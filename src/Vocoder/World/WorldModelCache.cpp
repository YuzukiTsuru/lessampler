/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "WorldModelCache.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <span>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

#include "Core/Exception.h"
#include "Core/Log.h"
#include "Core/SourceFingerprint.h"

namespace lessampler {

    namespace {

        constexpr std::array<char, 4> kMagic{'L', 'S', 'A', 'M'};
        constexpr std::array<char, 4> kTrailer{'E', 'N', 'D', '2'};
        constexpr std::uint32_t kFormatVersion = 2;
        constexpr std::uint64_t kMaxFrames = 10'000'000;
        constexpr std::uint32_t kMaxStringLength = 4096;
        constexpr std::uint64_t kMaxSamples = 100'000'000;

        struct ModelMetadata {
            std::uint32_t version = 0;
            std::string checksum;
            SourceFingerprint source;
            std::uint64_t x_length = 0;
            std::int32_t fs = 0;
            double frame_period = 0.0;
            std::uint32_t w_length = 0;
            std::uint32_t fft_size = 0;
            std::uint64_t f0_length = 0;
        };

        template<typename T>
        void write_scalar(std::ostream &out, T value) {
            static_assert(std::is_trivially_copyable_v<T>);
            std::array<std::byte, sizeof(T)> bytes{};
            std::memcpy(bytes.data(), &value, sizeof(T));
            if constexpr (std::endian::native == std::endian::big) {
                std::ranges::reverse(bytes);
            }
            out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }

        template<typename T>
        T read_scalar(std::istream &in) {
            static_assert(std::is_trivially_copyable_v<T>);
            std::array<std::byte, sizeof(T)> bytes{};
            in.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            if (!in) {
                throw audio_file_error("truncated .lessaudio file");
            }
            if constexpr (std::endian::native == std::endian::big) {
                std::ranges::reverse(bytes);
            }
            T value{};
            std::memcpy(&value, bytes.data(), sizeof(T));
            return value;
        }

        void write_magic(std::ostream &out, const std::array<char, 4> &magic) {
            out.write(magic.data(), static_cast<std::streamsize>(magic.size()));
        }

        void read_magic(std::istream &in, const std::array<char, 4> &expected, std::string_view what) {
            std::array<char, 4> actual{};
            in.read(actual.data(), static_cast<std::streamsize>(actual.size()));
            if (!in || actual != expected) {
                throw header_check_error(std::string(what), std::string(expected.data(), expected.size()));
            }
        }

        void write_doubles(std::ostream &out, std::span<const double> values) {
            if constexpr (std::endian::native == std::endian::little) {
                out.write(reinterpret_cast<const char *>(values.data()),
                          static_cast<std::streamsize>(values.size() * sizeof(double)));
            } else {
                for (double value: values) {
                    write_scalar<double>(out, value);
                }
            }
        }

        void read_doubles(std::istream &in, std::span<double> values) {
            if constexpr (std::endian::native == std::endian::little) {
                in.read(reinterpret_cast<char *>(values.data()),
                        static_cast<std::streamsize>(values.size() * sizeof(double)));
                if (!in) {
                    throw audio_file_error("truncated .lessaudio file");
                }
            } else {
                for (double &value: values) {
                    value = read_scalar<double>(in);
                }
            }
        }

        void write_string(std::ostream &out, std::string_view value) {
            if (value.size() > kMaxStringLength) {
                throw parameter_error("String is too long for .lessaudio");
            }
            write_scalar<std::uint32_t>(out, static_cast<std::uint32_t>(value.size()));
            out.write(value.data(), static_cast<std::streamsize>(value.size()));
        }

        std::string read_string(std::istream &in) {
            const auto length = read_scalar<std::uint32_t>(in);
            if (length > kMaxStringLength) {
                throw audio_file_error("Invalid string length in .lessaudio file");
            }
            std::string value(length, '\0');
            in.read(value.data(), static_cast<std::streamsize>(length));
            if (!in) {
                throw audio_file_error("truncated .lessaudio file");
            }
            return value;
        }

        ModelMetadata read_metadata(std::istream &in) {
            read_magic(in, kMagic, "model header");

            ModelMetadata metadata;
            metadata.version = read_scalar<std::uint32_t>(in);
            if (metadata.version != kFormatVersion) {
                throw file_version_error("Unsupported .lessaudio format");
            }

            metadata.checksum = read_string(in);
            metadata.source.size = read_scalar<std::uint64_t>(in);
            metadata.source.mtime = read_scalar<std::int64_t>(in);
            metadata.x_length = read_scalar<std::uint64_t>(in);
            metadata.fs = read_scalar<std::int32_t>(in);
            metadata.frame_period = read_scalar<double>(in);
            metadata.w_length = read_scalar<std::uint32_t>(in);
            metadata.fft_size = read_scalar<std::uint32_t>(in);
            metadata.f0_length = read_scalar<std::uint64_t>(in);

            if (metadata.fs <= 0 || metadata.frame_period <= 0.0 || metadata.fft_size == 0 || metadata.w_length != metadata.fft_size / 2 + 1 || metadata.f0_length == 0 || metadata.f0_length > kMaxFrames || metadata.x_length > kMaxSamples) {
                throw audio_file_error("Invalid metadata in .lessaudio file");
            }

            return metadata;
        }

        void validate_against(const ModelMetadata &metadata, const Config &configure,
                              const SourceFingerprint &source) {
            if (metadata.checksum != configure.fingerprint()) {
                throw file_version_error("Configuration does not match .lessaudio checksum");
            }
            if (metadata.source.size != source.size || metadata.source.mtime != source.mtime) {
                throw file_version_error("Source WAV changed since .lessaudio was generated");
            }
        }

        std::uint64_t remaining_bytes(std::istream &in) {
            const auto position = in.tellg();
            if (position < 0) {
                throw audio_file_error("Cannot determine .lessaudio file position");
            }
            in.seekg(0, std::ios::end);
            const auto end = in.tellg();
            in.seekg(position);
            if (end < position) {
                throw audio_file_error("Truncated .lessaudio file");
            }
            return static_cast<std::uint64_t>(end - position);
        }

        Matrix read_frames(std::istream &in, std::uint64_t frame_count, std::uint32_t w_length) {
            const auto row_bytes = static_cast<std::uint64_t>(sizeof(std::uint32_t)) + static_cast<std::uint64_t>(w_length) * sizeof(double);
            if (frame_count > kMaxFrames || row_bytes == 0 || frame_count > remaining_bytes(in) / row_bytes) {
                throw audio_file_error("Too many frames in .lessaudio file");
            }

            Matrix frames(static_cast<std::size_t>(frame_count), w_length);
            for (std::size_t i = 0; i < frames.rows(); ++i) {
                const auto row_length = read_scalar<std::uint32_t>(in);
                if (row_length != w_length) {
                    throw audio_file_error("Invalid frame width in .lessaudio file");
                }
                read_doubles(in, frames[i]);
            }
            return frames;
        }

        void write_frames(std::ostream &out, const Matrix &frames) {
            write_scalar<std::uint64_t>(out, static_cast<std::uint64_t>(frames.rows()));
            for (std::size_t i = 0; i < frames.rows(); ++i) {
                write_scalar<std::uint32_t>(out, static_cast<std::uint32_t>(frames.cols()));
                write_doubles(out, frames[i]);
            }
        }

    }// namespace

    WorldModelCache::WorldModelCache(std::filesystem::path Path, const WorldModel &audioModel, Config configure)
        : _audioModel(&audioModel), _configure(std::move(configure)), in_file_path(std::move(Path)) {
        GenerateFilePath();
    }

    WorldModelCache::WorldModelCache(std::filesystem::path Path) : in_file_path(std::move(Path)) {
        _audioModel = &_ownedModel;
        GenerateFilePath();
    }

    WorldModelCache::~WorldModelCache() = default;

    const WorldModel &WorldModelCache::GetAudioModel() const {
        return *_audioModel;
    }

    bool WorldModelCache::CheckAudioModel(const Config &configure) const {
        if (!std::filesystem::exists(audio_model_file_path)) {
            return false;
        }

        try {
            std::ifstream in(audio_model_file_path, std::ios::binary);
            if (!in) {
                return false;
            }

            const auto metadata = read_metadata(in);
            validate_against(metadata, configure, FingerprintSource(in_file_path));
            return true;
        } catch (const std::exception &error) {
            YALL_WARN_ << std::format("Audio model is not reusable: {}", error.what());
            return false;
        }
    }

    void WorldModelCache::SaveAudioModel() const {
        const auto &model = GetAudioModel();
        const auto temporary_path = audio_model_file_path.string() + ".tmp";
        std::ofstream out(temporary_path, std::ios::binary | std::ios::trunc);
        if (!out) {
            throw file_open_error(temporary_path);
        }

        write_magic(out, kMagic);
        write_scalar<std::uint32_t>(out, kFormatVersion);
        write_string(out, _configure.fingerprint());

        const auto source = FingerprintSource(in_file_path);
        write_scalar<std::uint64_t>(out, source.size);
        write_scalar<std::int64_t>(out, source.mtime);
        write_scalar<std::uint64_t>(out, static_cast<std::uint64_t>(model.x_length));
        write_scalar<std::int32_t>(out, static_cast<std::int32_t>(model.fs));
        write_scalar<double>(out, model.frame_period);
        write_scalar<std::uint32_t>(out, static_cast<std::uint32_t>(model.w_length));
        write_scalar<std::uint32_t>(out, static_cast<std::uint32_t>(model.fft_size));

        write_scalar<std::uint64_t>(out, static_cast<std::uint64_t>(model.f0.size()));
        write_doubles(out, model.f0);

        write_frames(out, model.spectrogram);
        write_frames(out, model.aperiodicity);

        write_magic(out, kTrailer);
        out.close();
        if (!out) {
            throw file_open_error(temporary_path);
        }

        std::error_code error;
        std::filesystem::remove(audio_model_file_path, error);
        error.clear();
        std::filesystem::rename(temporary_path, audio_model_file_path, error);
        if (error) {
            throw file_open_error(audio_model_file_path.string());
        }
    }

    WorldModel WorldModelCache::ReadAudioModel(const Config &configure) {
        std::ifstream in(audio_model_file_path, std::ios::binary);
        if (!in) {
            throw file_open_error(audio_model_file_path.string());
        }

        const auto metadata = read_metadata(in);
        validate_against(metadata, configure, FingerprintSource(in_file_path));

        if (metadata.f0_length > remaining_bytes(in) / sizeof(double)) {
            throw audio_file_error("Truncated f0 data in .lessaudio file");
        }

        _ownedModel = WorldModel{};
        _ownedModel.x_length = static_cast<std::size_t>(metadata.x_length);
        _ownedModel.fs = metadata.fs;
        _ownedModel.frame_period = metadata.frame_period;
        _ownedModel.w_length = static_cast<int>(metadata.w_length);
        _ownedModel.fft_size = static_cast<int>(metadata.fft_size);

        _ownedModel.f0.resize(static_cast<std::size_t>(metadata.f0_length));
        read_doubles(in, _ownedModel.f0);

        const auto frame_count = read_scalar<std::uint64_t>(in);
        if (frame_count != metadata.f0_length) {
            throw audio_file_error("Spectrogram frame count does not match f0 length");
        }
        _ownedModel.spectrogram = read_frames(in, frame_count, metadata.w_length);

        const auto aperiodicity_count = read_scalar<std::uint64_t>(in);
        if (aperiodicity_count != metadata.f0_length) {
            throw audio_file_error("Aperiodicity frame count does not match f0 length");
        }
        _ownedModel.aperiodicity = read_frames(in, aperiodicity_count, metadata.w_length);

        read_magic(in, kTrailer, "model trailer");
        return std::move(_ownedModel);
    }

    void WorldModelCache::GenerateFilePath() {
        audio_model_file_path = in_file_path;
        audio_model_file_path.replace_extension(std::string(audio_model_file_ext));
    }

}// namespace lessampler
