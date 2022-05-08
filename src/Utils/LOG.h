#ifndef LESSAMPLER_LOG_H
#define LESSAMPLER_LOG_H

#include <ColorCout.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <mutex>
#include <iomanip>

#include "lessConfigure.h"

enum Yall_LEVEL {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_CRITICAL,
    // Private usage
    LOG_FILE,
    LOG_FUNC,
    LOG_LINE,
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
            *stream << " " << msg << " ";
            if (logLevel != Yall_LEVEL::LOG_FILE && logLevel != Yall_LEVEL::LOG_FUNC && logLevel != Yall_LEVEL::LOG_LINE)
                *stream << std::endl;
        }
    };
};

class Yall_Debug_Instance : Yall_Inst {
public:
    explicit Yall_Debug_Instance(Yall_LEVEL logLevel) : Yall_Inst(logLevel) {
#ifdef DEBUG_MODE
        streams.push_back(&std::cout);
#endif
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
            switch (logLevel) {
                case Yall_LEVEL::LOG_DEBUG:
                    *stream << cc::white << "[DEBUG]" << cc::reset;
                    break;
                case Yall_LEVEL::LOG_FUNC:
                    *stream << cc::cyan << "[FUNC]" << cc::reset;
                    break;
                case Yall_LEVEL::LOG_FILE:
                    *stream << cc::yellow << "[FILE]" << cc::reset;
                    break;
                case Yall_LEVEL::LOG_LINE:
                    *stream << cc::green << "[LINE]" << cc::reset;
                    break;
                default:
                    break;
            }
            *stream << " " << msg << " ";
            if (logLevel != Yall_LEVEL::LOG_FILE && logLevel != Yall_LEVEL::LOG_FUNC && logLevel != Yall_LEVEL::LOG_LINE)
                *stream << std::endl;
        }
    };
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

    static Yall_Debug_Instance &GetDebugYall(Yall_LEVEL logLevel) {
        auto it = GetDebugInstance().yall_debug_inst.find(logLevel);
        if (it == GetDebugInstance().yall_debug_inst.end()) {
            auto *logger = new Yall_Debug_Instance(logLevel);
            GetDebugInstance().yall_debug_inst[logLevel] = logger;
            return *logger;
        }
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


#ifdef DEBUG_MODE
#define YALL_FILE_ Yall::GetDebugYall(Yall_LEVEL::LOG_FILE) << __FILE__

#if __GNUC__
#define YALL_FUNC_ Yall::GetDebugYall(Yall_LEVEL::LOG_FUNC) << __PRETTY_FUNCTION__
#else
#define YALL_FUNC_ Yall::GetDebugYall(Yall_LEVEL::LOG_FUNC) << __func__
#endif

#ifdef __LINE__
#define YALL_LINE_ Yall::GetDebugYall(Yall_LEVEL::LOG_LINE) << std::to_string(__LINE__)
#define YALL_DEBUG_       YALL_FILE_; YALL_FUNC_;YALL_LINE_; Yall::GetDebugYall(Yall_LEVEL::LOG_DEBUG)
#else
#define YALL_DEBUG_       YALL_FILE_; YALL_FUNC_; Yall::GetDebugYall(Yall_LEVEL::LOG_DEBUG)
#endif

#else
#define YALL_DEBUG_       Yall::GetDebugYall(Yall_LEVEL::LOG_DEBUG)
#endif


#define YALL_INFO_        Yall::GetYall(Yall_LEVEL::LOG_INFO)
#define YALL_WARN_        Yall::GetYall(Yall_LEVEL::LOG_WARN)
#define YALL_ERROR_       Yall::GetYall(Yall_LEVEL::LOG_ERROR)
#define YALL_CRITICAL_    Yall::GetYall(Yall_LEVEL::LOG_CRITICAL)

#endif //LESSAMPLER_LOG_H
