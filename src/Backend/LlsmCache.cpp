/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "LlsmCache.h"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <vector>

#include "Utils/LOG.h"
#include "Utils/exception.h"

namespace lessbackend {

    namespace {

        constexpr std::array<char, 4> kMagic{'L', 'S', 'L', 'L'};
        constexpr std::array<char, 4> kTrailer{'L', '1', 'E', 'N'};
        constexpr std::uint32_t kFormatVersion = 1;
        constexpr std::uint64_t kMaxFrames = 10'000'000;
        constexpr std::uint32_t kMaxSpectrum = 65536;
        constexpr std::uint32_t kMaxHarmonics = 65536;
        constexpr std::uint32_t kMaxChannels = 64;
        constexpr std::uint32_t kMaxPsd = 4096;
        constexpr std::uint32_t kMaxStringLength = 4096;

        static_assert(std::is_same_v<FP_TYPE, float>);

        struct SourceFingerprint {
            std::uint64_t size = 0;
            std::int64_t mtime = 0;
        };

        struct CacheHeader {
            std::string fingerprint;
            SourceFingerprint source;
            std::int32_t fs = 0;
            double thop = 0.0;
            std::uint32_t nfft = 0;
            std::uint64_t nfrm = 0;
            std::uint32_t npsd = 0;
            std::uint32_t nchannel = 0;
            std::uint32_t maxnhar = 0;
            std::uint32_t maxnhar_e = 0;
            double fnyq = 0.0;
            double noswarp = 0.0;
            double lipradius = 0.0;
            std::vector<FP_TYPE> chanfreq;
        };

        SourceFingerprint fingerprint_of(const std::filesystem::path &path) {
            std::error_code error;
            const auto size = std::filesystem::file_size(path, error);
            if (error) {
                return {};
            }

            const auto mtime = std::filesystem::last_write_time(path, error);
            if (error) {
                return {size, 0};
            }

            return {size, static_cast<std::int64_t>(mtime.time_since_epoch().count())};
        }

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
                throw audio_file_error("truncated .lessllsm file");
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

        void write_floats(std::ostream &out, std::span<const FP_TYPE> values) {
            if (values.empty()) {
                return;
            }
            if constexpr (std::endian::native == std::endian::little) {
                out.write(reinterpret_cast<const char *>(values.data()),
                          static_cast<std::streamsize>(values.size() * sizeof(FP_TYPE)));
            } else {
                for (FP_TYPE value: values) {
                    write_scalar<FP_TYPE>(out, value);
                }
            }
        }

        void read_floats(std::istream &in, std::span<FP_TYPE> values) {
            if (values.empty()) {
                return;
            }
            if constexpr (std::endian::native == std::endian::little) {
                in.read(reinterpret_cast<char *>(values.data()),
                        static_cast<std::streamsize>(values.size() * sizeof(FP_TYPE)));
                if (!in) {
                    throw audio_file_error("truncated .lessllsm file");
                }
            } else {
                for (FP_TYPE &value: values) {
                    value = read_scalar<FP_TYPE>(in);
                }
            }
        }

        void write_string(std::ostream &out, std::string_view value) {
            if (value.size() > kMaxStringLength) {
                throw parameter_error("String is too long for .lessllsm");
            }
            write_scalar<std::uint32_t>(out, static_cast<std::uint32_t>(value.size()));
            out.write(value.data(), static_cast<std::streamsize>(value.size()));
        }

        std::string read_string(std::istream &in) {
            const auto length = read_scalar<std::uint32_t>(in);
            if (length > kMaxStringLength) {
                throw audio_file_error("Invalid string length in .lessllsm file");
            }
            std::string value(length, '\0');
            in.read(value.data(), static_cast<std::streamsize>(length));
            if (!in) {
                throw audio_file_error("truncated .lessllsm file");
            }
            return value;
        }

        std::uint64_t remaining_bytes(std::istream &in) {
            const auto position = in.tellg();
            if (position < 0) {
                throw audio_file_error("Cannot determine .lessllsm file position");
            }
            in.seekg(0, std::ios::end);
            const auto end = in.tellg();
            in.seekg(position);
            if (end < position) {
                throw audio_file_error("Truncated .lessllsm file");
            }
            return static_cast<std::uint64_t>(end - position);
        }

        void write_header(std::ostream &out, const CacheHeader &header) {
            write_magic(out, kMagic);
            write_scalar<std::uint32_t>(out, kFormatVersion);
            write_string(out, header.fingerprint);
            write_scalar<std::uint64_t>(out, header.source.size);
            write_scalar<std::int64_t>(out, header.source.mtime);
            write_scalar<std::int32_t>(out, header.fs);
            write_scalar<double>(out, header.thop);
            write_scalar<std::uint32_t>(out, header.nfft);
            write_scalar<std::uint64_t>(out, header.nfrm);
            write_scalar<std::uint32_t>(out, header.npsd);
            write_scalar<std::uint32_t>(out, header.nchannel);
            write_scalar<std::uint32_t>(out, header.maxnhar);
            write_scalar<std::uint32_t>(out, header.maxnhar_e);
            write_scalar<double>(out, header.fnyq);
            write_scalar<double>(out, header.noswarp);
            write_scalar<double>(out, header.lipradius);
            write_scalar<std::uint32_t>(out, static_cast<std::uint32_t>(header.chanfreq.size()));
            write_floats(out, header.chanfreq);
        }

        CacheHeader read_header(std::istream &in) {
            read_magic(in, kMagic, "LLSM cache header");

            const auto version = read_scalar<std::uint32_t>(in);
            if (version != kFormatVersion) {
                throw file_version_error("Unsupported .lessllsm format");
            }

            CacheHeader header;
            header.fingerprint = read_string(in);
            header.source.size = read_scalar<std::uint64_t>(in);
            header.source.mtime = read_scalar<std::int64_t>(in);
            header.fs = read_scalar<std::int32_t>(in);
            header.thop = read_scalar<double>(in);
            header.nfft = read_scalar<std::uint32_t>(in);
            header.nfrm = read_scalar<std::uint64_t>(in);
            header.npsd = read_scalar<std::uint32_t>(in);
            header.nchannel = read_scalar<std::uint32_t>(in);
            header.maxnhar = read_scalar<std::uint32_t>(in);
            header.maxnhar_e = read_scalar<std::uint32_t>(in);
            header.fnyq = read_scalar<double>(in);
            header.noswarp = read_scalar<double>(in);
            header.lipradius = read_scalar<double>(in);
            const auto chanfreq_count = read_scalar<std::uint32_t>(in);

            if (header.fs <= 0 || !(header.thop > 0.0) || !(header.fnyq > 0.0) ||
                !(header.noswarp > 0.0) || !(header.lipradius > 0.0) ||
                header.nfft < 32 || header.nfft > kMaxSpectrum || header.nfft % 2 != 0 ||
                header.nfrm == 0 || header.nfrm > kMaxFrames ||
                header.npsd == 0 || header.npsd > kMaxPsd ||
                header.nchannel == 0 || header.nchannel > kMaxChannels ||
                header.maxnhar > kMaxHarmonics || header.maxnhar_e > kMaxHarmonics ||
                chanfreq_count != header.nchannel - 1) {
                throw audio_file_error("Invalid metadata in .lessllsm file");
            }

            header.chanfreq.resize(chanfreq_count);
            read_floats(in, header.chanfreq);
            return header;
        }

        template<typename T>
        const T *get_const_conf(const llsm_container *conf, int index, std::string_view name) {
            auto *value = static_cast<const T *>(llsm_container_get(const_cast<llsm_container *>(conf), index));
            if (value == nullptr) {
                throw audio_file_error("Missing " + std::string(name) + " in LLSM configuration");
            }
            return value;
        }

        llsm_container *create_conf(const CacheHeader &header) {
            llsm_container *conf = llsm_create_container(12);
            llsm_container_attach(conf, LLSM_CONF_NFRM, llsm_create_int(static_cast<int>(header.nfrm)),
                                  llsm_delete_int, llsm_copy_int);
            llsm_container_attach(conf, LLSM_CONF_THOP, llsm_create_fp(static_cast<FP_TYPE>(header.thop)),
                                  llsm_delete_fp, llsm_copy_fp);
            llsm_container_attach(conf, LLSM_CONF_MAXNHAR, llsm_create_int(static_cast<int>(header.maxnhar)),
                                  llsm_delete_int, llsm_copy_int);
            llsm_container_attach(conf, LLSM_CONF_MAXNHAR_E, llsm_create_int(static_cast<int>(header.maxnhar_e)),
                                  llsm_delete_int, llsm_copy_int);
            llsm_container_attach(conf, LLSM_CONF_NPSD, llsm_create_int(static_cast<int>(header.npsd)),
                                  llsm_delete_int, llsm_copy_int);
            llsm_container_attach(conf, LLSM_CONF_NOSWARP, llsm_create_fp(static_cast<FP_TYPE>(header.noswarp)),
                                  llsm_delete_fp, llsm_copy_fp);
            llsm_container_attach(conf, LLSM_CONF_FNYQ, llsm_create_fp(static_cast<FP_TYPE>(header.fnyq)),
                                  llsm_delete_fp, llsm_copy_fp);
            llsm_container_attach(conf, LLSM_CONF_NCHANNEL, llsm_create_int(static_cast<int>(header.nchannel)),
                                  llsm_delete_int, llsm_copy_int);
            llsm_container_attach(conf, LLSM_CONF_NSPEC, llsm_create_int(static_cast<int>(header.nfft / 2 + 1)),
                                  llsm_delete_int, llsm_copy_int);
            llsm_container_attach(conf, LLSM_CONF_LIPRADIUS,
                                  llsm_create_fp(static_cast<FP_TYPE>(header.lipradius)),
                                  llsm_delete_fp, llsm_copy_fp);

            auto *chanfreq = llsm_create_fparray(static_cast<int>(header.chanfreq.size()));
            std::ranges::copy(header.chanfreq, chanfreq);
            llsm_container_attach(conf, LLSM_CONF_CHANFREQ, chanfreq,
                                  llsm_delete_fparray, llsm_copy_fparray);
            return conf;
        }

        void attach_fp(llsm_container *frame, int index, FP_TYPE value) {
            llsm_container_attach(frame, index, llsm_create_fp(value), llsm_delete_fp, llsm_copy_fp);
        }

        void attach_fparray(llsm_container *frame, int index, std::span<const FP_TYPE> values) {
            auto *array = llsm_create_fparray(static_cast<int>(values.size()));
            std::ranges::copy(values, array);
            llsm_container_attach(frame, index, array, llsm_delete_fparray, llsm_copy_fparray);
        }

        LlsmChunkPtr read_chunk(std::istream &in, const CacheHeader &header) {
            // Reject a header whose frame count cannot possibly fit in the
            // remaining bytes before allocating the frame-pointer array.
            const auto min_frame_bytes = 2 * sizeof(FP_TYPE) + 1 +
                                         static_cast<std::uint64_t>(header.npsd) * sizeof(FP_TYPE) +
                                         static_cast<std::uint64_t>(header.nchannel) * sizeof(FP_TYPE) +
                                         static_cast<std::uint64_t>(header.nchannel) * sizeof(std::uint32_t);
            if (min_frame_bytes == 0 || header.nfrm > remaining_bytes(in) / min_frame_bytes) {
                throw audio_file_error("Truncated .lessllsm frame data");
            }

            llsm_container *conf = create_conf(header);
            LlsmChunkPtr chunk(llsm_create_chunk(conf, 0));
            llsm_delete_container(conf);
            if (!chunk || chunk->frames == nullptr) {
                throw audio_file_error("Cannot allocate LLSM cache chunk");
            }

            const auto nspec = static_cast<std::size_t>(header.nfft / 2 + 1);
            std::vector<FP_TYPE> vtmagn(nspec);
            std::vector<FP_TYPE> vsphere;
            std::vector<FP_TYPE> psd(header.npsd);
            std::vector<FP_TYPE> edc(header.nchannel);
            std::vector<FP_TYPE> envelope_ampl;
            std::vector<FP_TYPE> envelope_phse;

            for (std::uint64_t i = 0; i < header.nfrm; ++i) {
                const auto f0 = read_scalar<FP_TYPE>(in);
                const auto rd = read_scalar<FP_TYPE>(in);
                const auto has_envelope = read_scalar<std::uint8_t>(in);
                if (has_envelope > 1 || (has_envelope == 1) != (f0 > 0.0f) ||
                    !std::isfinite(f0) || !std::isfinite(rd)) {
                    throw audio_file_error("Invalid LLSM frame in cache");
                }

                std::uint32_t nhar = 0;
                if (has_envelope) {
                    nhar = read_scalar<std::uint32_t>(in);
                    if (nhar > header.maxnhar) {
                        throw audio_file_error("Invalid LLSM harmonic count in cache");
                    }
                    vsphere.resize(nhar);
                    read_floats(in, vsphere);
                    read_floats(in, vtmagn);
                }

                read_floats(in, psd);
                read_floats(in, edc);

                llsm_container *frame = llsm_create_frame(0, static_cast<int>(header.nchannel),
                                                          static_cast<int>(header.maxnhar_e),
                                                          static_cast<int>(header.npsd));
                // Own the frame immediately so a truncated cache cannot leak it.
                chunk->frames[i] = frame;
                auto *frame_f0 = static_cast<FP_TYPE *>(llsm_container_get(frame, LLSM_FRAME_F0));
                frame_f0[0] = f0;
                attach_fp(frame, LLSM_FRAME_RD, rd);

                auto *nm = static_cast<llsm_nmframe *>(llsm_container_get(frame, LLSM_FRAME_NM));
                std::ranges::copy(psd, nm->psd);
                std::ranges::copy(edc, nm->edc);
                for (std::uint32_t channel = 0; channel < header.nchannel; ++channel) {
                    const auto envelope_harmonics = read_scalar<std::uint32_t>(in);
                    if (envelope_harmonics > header.maxnhar_e) {
                        throw audio_file_error("Invalid LLSM noise envelope in cache");
                    }
                    envelope_ampl.resize(envelope_harmonics);
                    envelope_phse.resize(envelope_harmonics);
                    read_floats(in, envelope_ampl);
                    read_floats(in, envelope_phse);
                    auto *envelope = nm->eenv[channel];
                    std::ranges::copy(envelope_ampl, envelope->ampl);
                    std::ranges::copy(envelope_phse, envelope->phse);
                    envelope->nhar = static_cast<int>(envelope_harmonics);
                }

                if (has_envelope) {
                    attach_fparray(frame, LLSM_FRAME_VSPHSE, vsphere);
                    attach_fparray(frame, LLSM_FRAME_VTMAGN, vtmagn);
                }
            }

            read_magic(in, kTrailer, "LLSM cache trailer");
            return chunk;
        }

        bool same_source(const SourceFingerprint &cached, const std::filesystem::path &path) {
            const auto current = fingerprint_of(path);
            return current.size != 0 && current.size == cached.size && current.mtime == cached.mtime;
        }

    }// namespace

    void LlsmChunkDeleter::operator()(llsm_chunk *value) const {
        llsm_delete_chunk(value);
    }

    std::filesystem::path LlsmCachePath(const std::filesystem::path &input_path) {
        auto path = input_path;
        path.replace_extension("lessllsm");
        return path;
    }

    LlsmChunkPtr LoadLlsmCache(const std::filesystem::path &input_path, const lessConfigure &configure) {
        const auto path = LlsmCachePath(input_path);
        std::error_code exists_error;
        if (!std::filesystem::exists(path, exists_error) || exists_error) {
            return nullptr;
        }

        try {
            std::ifstream in(path, std::ios::binary);
            if (!in) {
                return nullptr;
            }

            const auto header = read_header(in);
            if (header.fingerprint != configure.fingerprint()) {
                throw file_version_error("Configuration does not match .lessllsm checksum");
            }
            if (!same_source(header.source, input_path)) {
                throw file_version_error("Source WAV changed since .lessllsm was generated");
            }

            return read_chunk(in, header);
        } catch (const std::exception &error) {
            YALL_DEBUG_ << std::format("LLSM cache is not reusable: {}", error.what());
            return nullptr;
        }
    }

    void SaveLlsmCache(const std::filesystem::path &input_path,
                       const lessConfigure &configure, const llsm_chunk *chunk) {
        if (chunk == nullptr || chunk->conf == nullptr || chunk->frames == nullptr) {
            throw parameter_error("Cannot save an empty LLSM chunk");
        }

        const auto source = fingerprint_of(input_path);
        if (source.size == 0) {
            throw file_open_error(input_path.string());
        }

        auto *conf = chunk->conf;
        const auto nfrm = static_cast<std::uint64_t>(*get_const_conf<int>(conf, LLSM_CONF_NFRM, "NFRM"));
        const auto thop = static_cast<double>(*get_const_conf<FP_TYPE>(conf, LLSM_CONF_THOP, "THOP"));
        const auto npsd = static_cast<std::uint32_t>(*get_const_conf<int>(conf, LLSM_CONF_NPSD, "NPSD"));
        const auto nchannel = static_cast<std::uint32_t>(*get_const_conf<int>(conf, LLSM_CONF_NCHANNEL, "NCHANNEL"));
        const auto nspec = static_cast<std::uint32_t>(*get_const_conf<int>(conf, LLSM_CONF_NSPEC, "NSPEC"));
        const auto fnyq = static_cast<double>(*get_const_conf<FP_TYPE>(conf, LLSM_CONF_FNYQ, "FNYQ"));
        const auto noswarp = static_cast<double>(*get_const_conf<FP_TYPE>(conf, LLSM_CONF_NOSWARP, "NOSWARP"));
        const auto lipradius = static_cast<double>(*get_const_conf<FP_TYPE>(conf, LLSM_CONF_LIPRADIUS, "LIPRADIUS"));
        const auto maxnhar_conf = static_cast<std::uint32_t>(*get_const_conf<int>(conf, LLSM_CONF_MAXNHAR, "MAXNHAR"));
        const auto maxnhar_e_conf = static_cast<std::uint32_t>(*get_const_conf<int>(conf, LLSM_CONF_MAXNHAR_E, "MAXNHAR_E"));

        auto *chanfreq = static_cast<FP_TYPE *>(llsm_container_get(conf, LLSM_CONF_CHANFREQ));
        if (chanfreq == nullptr || nfrm == 0 || nfrm > kMaxFrames || npsd == 0 ||
            npsd > kMaxPsd || nchannel == 0 || nchannel > kMaxChannels ||
            llsm_fparray_length(chanfreq) != static_cast<int>(nchannel) - 1 ||
            nspec < 16 || nspec > kMaxSpectrum / 2 + 1 || !(fnyq > 0.0) ||
            !(noswarp > 0.0) || !(lipradius > 0.0) || !(thop > 0.0)) {
            throw parameter_error("Invalid LLSM chunk for cache");
        }

        CacheHeader header;
        header.fingerprint = configure.fingerprint();
        header.source = source;
        header.fs = static_cast<std::int32_t>(std::lround(fnyq * 2.0));
        header.thop = thop;
        header.nfft = (nspec - 1) * 2;
        header.nfrm = nfrm;
        header.npsd = npsd;
        header.nchannel = nchannel;
        header.maxnhar = maxnhar_conf;
        header.maxnhar_e = maxnhar_e_conf;
        header.fnyq = fnyq;
        header.noswarp = noswarp;
        header.lipradius = lipradius;
        header.chanfreq.assign(chanfreq, chanfreq + (nchannel - 1));

        // Include every actual frame requirement so a small analysis option
        // change cannot make a cached frame unreadable.
        for (std::uint64_t i = 0; i < nfrm; ++i) {
            auto *frame = chunk->frames[i];
            if (frame == nullptr) {
                throw parameter_error("Missing LLSM frame for cache");
            }
            if (const auto *f0 = get_const_conf<FP_TYPE>(frame, LLSM_FRAME_F0, "F0"); f0[0] > 0.0f) {
                const auto *vsphere = get_const_conf<FP_TYPE>(frame, LLSM_FRAME_VSPHSE, "VSPHSE");
                header.maxnhar = std::max(header.maxnhar,
                                          static_cast<std::uint32_t>(llsm_fparray_length(
                                                  const_cast<FP_TYPE *>(vsphere))));
            }
            const auto *nm = get_const_conf<llsm_nmframe>(frame, LLSM_FRAME_NM, "NM");
            for (std::uint32_t channel = 0; channel < nchannel; ++channel) {
                header.maxnhar_e = std::max(header.maxnhar_e,
                                            static_cast<std::uint32_t>(nm->eenv[channel]->nhar));
            }
        }
        if (header.maxnhar > kMaxHarmonics || header.maxnhar_e > kMaxHarmonics) {
            throw parameter_error("LLSM chunk has too many harmonics for the cache");
        }

        const auto path = LlsmCachePath(input_path);
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto temporary_path = std::filesystem::path(
                std::format("{}.tmp.{}.{}", path.string(), stamp,
                            std::hash<std::thread::id>{}(std::this_thread::get_id())));

        std::error_code cleanup_error;
        try {
            std::ofstream out(temporary_path, std::ios::binary | std::ios::trunc);
            if (!out) {
                throw file_open_error(temporary_path.string());
            }

            write_header(out, header);
            for (std::uint64_t i = 0; i < nfrm; ++i) {
                auto *frame = chunk->frames[i];
                const auto *f0 = get_const_conf<FP_TYPE>(frame, LLSM_FRAME_F0, "F0");
                const auto *rd = get_const_conf<FP_TYPE>(frame, LLSM_FRAME_RD, "RD");
                const auto *nm = get_const_conf<llsm_nmframe>(frame, LLSM_FRAME_NM, "NM");
                if (nm->npsd != static_cast<int>(npsd) || nm->nchannel != static_cast<int>(nchannel)) {
                    throw parameter_error("Inconsistent LLSM noise model size");
                }
                const auto *vsphere = static_cast<FP_TYPE *>(llsm_container_get(frame, LLSM_FRAME_VSPHSE));
                const auto *vtmagn = static_cast<FP_TYPE *>(llsm_container_get(frame, LLSM_FRAME_VTMAGN));
                const bool has_envelope = f0[0] > 0.0f;

                if (has_envelope != (vsphere != nullptr && vtmagn != nullptr)) {
                    throw parameter_error("Inconsistent LLSM frame layer-1 data");
                }

                write_scalar<FP_TYPE>(out, f0[0]);
                write_scalar<FP_TYPE>(out, rd[0]);
                write_scalar<std::uint8_t>(out, has_envelope ? 1 : 0);

                if (has_envelope) {
                    const auto nhar = static_cast<std::uint32_t>(llsm_fparray_length(const_cast<FP_TYPE *>(vsphere)));
                    const auto vtmagn_length = static_cast<std::uint32_t>(llsm_fparray_length(const_cast<FP_TYPE *>(vtmagn)));
                    if (vtmagn_length != nspec) {
                        throw parameter_error("Invalid LLSM spectrum size");
                    }
                    write_scalar<std::uint32_t>(out, nhar);
                    write_floats(out, std::span<const FP_TYPE>(vsphere, nhar));
                    write_floats(out, std::span<const FP_TYPE>(vtmagn, vtmagn_length));
                }

                write_floats(out, std::span<const FP_TYPE>(nm->psd, npsd));
                write_floats(out, std::span<const FP_TYPE>(nm->edc, nchannel));
                for (std::uint32_t channel = 0; channel < nchannel; ++channel) {
                    const auto *envelope = nm->eenv[channel];
                    const auto envelope_harmonics = static_cast<std::uint32_t>(envelope->nhar);
                    write_scalar<std::uint32_t>(out, envelope_harmonics);
                    write_floats(out, std::span<const FP_TYPE>(envelope->ampl, envelope_harmonics));
                    write_floats(out, std::span<const FP_TYPE>(envelope->phse, envelope_harmonics));
                }
            }
            write_magic(out, kTrailer);

            out.close();
            if (!out) {
                throw file_open_error(temporary_path.string());
            }

            std::error_code error;
            std::filesystem::rename(temporary_path, path, error);
            if (error) {
                // Windows does not replace an existing file with rename().
                error.clear();
                std::filesystem::remove(path, error);
                error.clear();
                std::filesystem::rename(temporary_path, path, error);
            }
            if (error) {
                throw file_open_error(path.string());
            }
        } catch (...) {
            std::filesystem::remove(temporary_path, cleanup_error);
            throw;
        }
    }

}// namespace lessbackend
