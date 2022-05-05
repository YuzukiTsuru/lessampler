#ifndef LESSAMPLER_LOG_H
#define LESSAMPLER_LOG_H

#include <ColorCout.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <mutex>
#include <iomanip>

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

class Yall_Instance {
public:
    explicit Yall_Instance(Yall_LEVEL logLevel) : logLevel(logLevel) {
        streams.push_back(&std::cout);
    }

    void operator<<(const std::string &msg) {
        std::lock_guard<std::mutex> lock(streamMtx);
        for (auto &stream: streams) {
            switch (logLevel) {
                case Yall_LEVEL::LOG_DEBUG:
                    *stream << cc::white << "[DEBUG]" << cc::reset;
                    break;
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
                case Yall_LEVEL::LOG_FILE:
                    *stream << cc::green << "[FILE]" << cc::reset;
                    break;
                case Yall_LEVEL::LOG_FUNC:
                    *stream << cc::magenta << "[FUNC]" << cc::reset;
                    break;
                case Yall_LEVEL::LOG_LINE:
                    *stream << cc::white << "[LINE]" << cc::reset;
                    break;
                default:
                    break;
            }
            *stream << " " << msg << " ";
            if (logLevel != Yall_LEVEL::LOG_FILE && logLevel != Yall_LEVEL::LOG_FUNC && logLevel != Yall_LEVEL::LOG_LINE)
                *stream << std::endl;
        }
    };

private:
    std::string name;
    Yall_LEVEL logLevel;
    std::vector<std::ostream *> streams;
    std::mutex streamMtx;
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

private:
    std::unordered_map<Yall_LEVEL, Yall_Instance *> yall_inst;

    Yall() = default;;

    static Yall &GetInstance() {
        static Yall inst;
        return inst;
    };
};


#define YALL_FILE_ Yall::GetYall(Yall_LEVEL::LOG_FILE) << __FILE__

#if __GNUC__
#define YALL_FUNC_ Yall::GetYall(Yall_LEVEL::LOG_FUNC) << __PRETTY_FUNCTION__
#else
#define YALL_FUNC_ Yall::GetYall(Yall_LEVEL::LOG_FUNC) << __func__
#endif

#define YALL_DEBUG_       YALL_FILE_; YALL_FUNC_; Yall::GetYall(Yall_LEVEL::LOG_DEBUG)
#define YALL_INFO_        YALL_FILE_; YALL_FUNC_; Yall::GetYall(Yall_LEVEL::LOG_INFO)
#define YALL_WARN_        YALL_FILE_; YALL_FUNC_; Yall::GetYall(Yall_LEVEL::LOG_WARN)
#define YALL_ERROR_       YALL_FILE_; YALL_FUNC_; Yall::GetYall(Yall_LEVEL::LOG_ERROR)
#define YALL_CRITICAL_    YALL_FILE_; YALL_FUNC_; Yall::GetYall(Yall_LEVEL::LOG_CRITICAL)

#endif //LESSAMPLER_LOG_H
