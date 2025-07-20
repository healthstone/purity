#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <ctime>
#include <chrono>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <sstream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <fmt/format.h>

class MDC {
public:
    void put(const std::string& key, const std::string& value) {
        data_[key] = value;
    }

    [[nodiscard]] const std::unordered_map<std::string, std::string>& data() const {
        return data_;
    }

private:
    std::unordered_map<std::string, std::string> data_;
};

class Logger {
public:
    static void init_thread_pool() {
        constexpr size_t queue_size = 8192;
        constexpr size_t num_threads = 1;
        spdlog::init_thread_pool(queue_size, num_threads);
    }

    static std::shared_ptr<spdlog::logger> get() {
        static std::shared_ptr<spdlog::logger> logger = create_logger();
        return logger;
    }

    // ---------- MDC wrappers ----------
    static void trace_with_mdc(const std::string& message, const MDC& mdc) {
        get()->trace(R"({{"msg":"{}","mdc":{}}})", escape(message), mdc_to_json(mdc));
    }

    static void debug_with_mdc(const std::string& message, const MDC& mdc) {
        get()->debug(R"({{"msg":"{}","mdc":{}}})", escape(message), mdc_to_json(mdc));
    }

    static void info_with_mdc(const std::string& message, const MDC& mdc) {
        get()->info(R"({{"msg":"{}","mdc":{}}})", escape(message), mdc_to_json(mdc));
    }

    static void warn_with_mdc(const std::string& message, const MDC& mdc) {
        get()->warn(R"({{"msg":"{}","mdc":{}}})", escape(message), mdc_to_json(mdc));
    }

    static void error_with_mdc(const std::string& message, const MDC& mdc) {
        get()->error(R"({{"msg":"{}","mdc":{}}})", escape(message), mdc_to_json(mdc));
    }

    static void critical_with_mdc(const std::string& message, const MDC& mdc) {
        get()->critical(R"({{"msg":"{}","mdc":{}}})", escape(message), mdc_to_json(mdc));
    }

private:
    // JSON форматтер без цвета (файл)
    class JsonFormatter : public spdlog::formatter {
    public:
        void format(const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest) override {
            using namespace std::chrono;

            auto tp = msg.time;
            auto s = time_point_cast<seconds>(tp);
            auto ms = time_point_cast<milliseconds>(tp) - time_point_cast<milliseconds>(s);
            auto ns = time_point_cast<nanoseconds>(tp) - time_point_cast<nanoseconds>(time_point_cast<milliseconds>(tp));

            auto time_t = system_clock::to_time_t(s);
            std::tm tm;
#ifdef _WIN32
            localtime_s(&tm, &time_t);
#else
            localtime_r(&time_t, &tm);
#endif

            char time_buf[64];
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", &tm);

            fmt::format_to(
                    std::back_inserter(dest),
                    "{{\"timestamp\":\"{}.{:03}{:03}\",\"level\":\"{}\",\"thread_id\":{},\"message\":{}}}\n",
                    time_buf,
                    static_cast<int>(ms.count()),
                    static_cast<int>(ns.count()),
                    spdlog::level::to_string_view(msg.level),
                    msg.thread_id,
                    fmt::string_view(msg.payload.data(), msg.payload.size())
            );
        }

        std::unique_ptr<spdlog::formatter> clone() const override {
            return std::make_unique<JsonFormatter>();
        }
    };

    // JSON форматтер с цветным уровнем (консоль)
    class ColoredJsonFormatter : public spdlog::formatter {
    public:
        void format(const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest) override {
            using namespace std::chrono;

            auto tp = msg.time;
            auto s = time_point_cast<seconds>(tp);
            auto ms = time_point_cast<milliseconds>(tp) - time_point_cast<milliseconds>(s);
            auto ns = time_point_cast<nanoseconds>(tp) - time_point_cast<nanoseconds>(time_point_cast<milliseconds>(tp));

            auto time_t = system_clock::to_time_t(s);
            std::tm tm;
#ifdef _WIN32
            localtime_s(&tm, &time_t);
#else
            localtime_r(&time_t, &tm);
#endif

            char time_buf[64];
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", &tm);

            auto level_str = spdlog::level::to_string_view(msg.level);

            fmt::format_to(
                    std::back_inserter(dest),
                    "{{\"timestamp\":\"{}.{:03}{:03}\",\"level\":\"{}{}{}\",\"thread_id\":{},\"message\":{}}}\n",
                    time_buf,
                    static_cast<int>(ms.count()),
                    static_cast<int>(ns.count()),
                    color_start(msg.level),
                    level_str,
                    color_end(),
                    msg.thread_id,
                    fmt::string_view(msg.payload.data(), msg.payload.size())
            );
        }

        std::unique_ptr<spdlog::formatter> clone() const override {
            return std::make_unique<ColoredJsonFormatter>();
        }

    private:
        static const char* color_start(spdlog::level::level_enum lvl) {
            switch (lvl) {
                case spdlog::level::trace:    return "\033[37m";
                case spdlog::level::debug:    return "\033[36m";
                case spdlog::level::info:     return "\033[32m";
                case spdlog::level::warn:     return "\033[33m";
                case spdlog::level::err:      return "\033[31m";
                case spdlog::level::critical: return "\033[41m";
                default:                      return "";
            }
        }

        static const char* color_end() { return "\033[0m"; }
    };

    static std::shared_ptr<spdlog::logger> create_logger() {
        try {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::debug);
            console_sink->set_formatter(std::make_unique<ColoredJsonFormatter>());

            std::vector<spdlog::sink_ptr> sinks{ console_sink };

            const char* env_log_file = std::getenv("LOG_FILE");
            bool enable_file_log = false;
            if (env_log_file) {
                std::string val(env_log_file);
                std::transform(val.begin(), val.end(), val.begin(), ::tolower);
                if (val == "true" || val == "1" || val == "yes") {
                    enable_file_log = true;
                }
            }

            if (enable_file_log) {
                auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                        "logs/server.log", 1024 * 1024 * 5, 3);
                file_sink->set_level(spdlog::level::debug);
                file_sink->set_formatter(std::make_unique<JsonFormatter>());
                sinks.push_back(file_sink);
            }

            auto logger = std::make_shared<spdlog::async_logger>(
                    "async_logger",
                    sinks.begin(),
                    sinks.end(),
                    spdlog::thread_pool(),
                    spdlog::async_overflow_policy::block
            );

            logger->set_level(spdlog::level::debug);
            spdlog::register_logger(logger);
            spdlog::set_default_logger(logger);

            return logger;

        } catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Logger init failed: " << ex.what() << std::endl;
            throw;
        }
    }

    static std::string mdc_to_json(const MDC& mdc) {
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        for (const auto& [k, v] : mdc.data()) {
            if (!first) oss << ",";
            oss << "\"" << escape(k) << "\":\"" << escape(v) << "\"";
            first = false;
        }
        oss << "}";
        return oss.str();
    }

    static std::string escape(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else out += c;
        }
        return out;
    }
};
