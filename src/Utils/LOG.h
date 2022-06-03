/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_LOG_H
#define LESSAMPLER_LOG_H

#include <ColorCout.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <mutex>
#include <iomanip>
#include <string_view>

enum Yall_LEVEL {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_CRITICAL,
};

class Yall_Inst {
public:
    explicit Yall_Inst(Yall_LEVEL logLevel) {
        this->logLevel = logLevel;
    };

    virtual void operator<<(const std::string &msg) {};

private:
protected:
    std::string name;
    Yall_LEVEL logLevel;
    std::mutex streamMtx;
    std::vector<std::ostream *> streams;
};

class Yall_Instance : Yall_Inst {
public:
    explicit Yall_Instance(Yall_LEVEL logLevel) : Yall_Inst(logLevel) {
        streams.push_back(&std::cout);
    };

    void operator<<(const std::string &msg) override {
        std::lock_guard<std::mutex> lock(streamMtx);
        for (auto &stream: streams) {
            switch (logLevel) {
                case Yall_LEVEL::LOG_INFO:
                    *stream << cc::cyan << "[INFO]" << cc::reset;
                    break;
                case Yall_LEVEL::LOG_WARN:
                    *stream << cc::yellow << "[WARNING]" << cc::reset;
                    break;
                case Yall_LEVEL::LOG_ERROR:
                    *stream << cc::red << "[ERROR]" << cc::reset;
                    break;
                case Yall_LEVEL::LOG_CRITICAL:
                    *stream << cc::on_red << "[CRITICAL]" << cc::reset;
                    break;
                default:
                    break;
            }
            *stream << " " << msg << " " << std::endl;
        }
    };
};

class Yall_Debug_Instance : Yall_Inst {
public:
    explicit Yall_Debug_Instance(Yall_LEVEL logLevel) : Yall_Inst(logLevel) {}

    void SetDebugInfo(const std::string &file, const std::string &func, int line) {
        this->FILE = file;
        this->FUNC = func;
        this->LINE = line;
    }

    [[maybe_unused]] void EnableDebug() {
        auto it = std::find(streams.begin(), streams.end(), &std::cout);
        if (it == streams.end())
            streams.push_back(&std::cout);
    }

    [[maybe_unused]] void DisableDebug() {
        auto it = std::find(streams.begin(), streams.end(), &std::cout);
        if (it == streams.end())
            return;
        streams.erase(it);
    }

    void operator<<(const std::string &msg) override {
        std::lock_guard<std::mutex> lock(streamMtx);
        for (auto &stream: streams) {
            *stream << cc::cyan << "[FUNC] " << std::left << std::setw(23) << cc::reset << fmt(this->FUNC) << " "
                    << cc::yellow << "[FILE] " << std::setw(23) << cc::reset << fmt(this->FILE) << " "
                    << cc::green << "[LINE] " << std::setw(4) << cc::reset << this->LINE << " "
                    << cc::white << "[DEBUG] " << cc::reset << msg << " " << std::endl;
        }
    }

private:
    std::string FILE = {};
    std::string FUNC = {};
    int LINE = {};
private:
    // Get the last 20 char
    static std::string fmt(std::string_view sv) {
        if (sv.length() > 20) {
            return std::string("...") + sv.substr(sv.length() - 20, sv.length()).data();
        } else {
            return sv.data();
        }
    }
};

class Yall {
public:
    Yall(Yall const &) = delete;

    void operator=(Yall const &) = delete;

    static Yall_Instance &GetYall(Yall_LEVEL logLevel) {
        auto it = GetInstance().yall_inst.find(logLevel);
        if (it == GetInstance().yall_inst.end()) {
            auto *logger = new Yall_Instance(logLevel);
            GetInstance().yall_inst[logLevel] = logger;
            return *logger;
        }
        return *it->second;
    };

    static Yall_Debug_Instance &GetDebugYall(Yall_LEVEL logLevel, const std::string &FILE, const std::string &FUNC, int LINE) {
        auto it = GetDebugInstance().yall_debug_inst.find(logLevel);
        if (it == GetDebugInstance().yall_debug_inst.end()) {
            auto *logger = new Yall_Debug_Instance(logLevel);
            GetDebugInstance().yall_debug_inst[logLevel] = logger;
            logger->SetDebugInfo(FILE, FUNC, LINE);
            return *logger;
        }
        it->second->SetDebugInfo(FILE, FUNC, LINE);
        return *it->second;
    };

private:
    std::unordered_map<Yall_LEVEL, Yall_Instance *> yall_inst;
    std::unordered_map<Yall_LEVEL, Yall_Debug_Instance *> yall_debug_inst;

    Yall() = default;;

    static Yall &GetInstance() {
        static Yall inst;
        return inst;
    };

    static Yall &GetDebugInstance() {
        static Yall inst_d;
        return inst_d;
    };
};


#if __GNUC__
#define YALL_FUNC_        __PRETTY_FUNCTION__
#else
#define YALL_FUNC_        __func__
#endif

#define YALL_DEBUG_       Yall::GetDebugYall(Yall_LEVEL::LOG_DEBUG, __FILE__, YALL_FUNC_, __LINE__)
#define YALL_INFO_        Yall::GetYall(Yall_LEVEL::LOG_INFO)
#define YALL_WARN_        Yall::GetYall(Yall_LEVEL::LOG_WARN)
#define YALL_ERROR_       Yall::GetYall(Yall_LEVEL::LOG_ERROR)
#define YALL_CRITICAL_    Yall::GetYall(Yall_LEVEL::LOG_CRITICAL)

#endif //LESSAMPLER_LOG_H
