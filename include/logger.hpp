#ifndef LOGGER_H
#define LOGGER_H

// #include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <stdexcept>

namespace vke_common
{
    class Logger
    {
    private:
        static Logger *instance;
        Logger() {}
        ~Logger() {}
        Logger(const Logger &);
        Logger &operator=(const Logger);

    public:
        static Logger *GetInstance()
        {
            if (instance == nullptr)
                throw std::runtime_error("Logger not initialized!");
            return instance;
        }

        static Logger *Init()
        {
            instance = new Logger();
            instance->init();
            return instance;
        }

        static void Dispose()
        {
            delete instance;
        }

        template <typename... Args>
        static void Trace(spdlog::format_string_t<Args...> fmt, Args &&...args)
        {
            instance->logger->trace(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Debug(spdlog::format_string_t<Args...> fmt, Args &&...args)
        {
            instance->logger->debug(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Info(spdlog::format_string_t<Args...> fmt, Args &&...args)
        {
            instance->logger->info(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Warn(spdlog::format_string_t<Args...> fmt, Args &&...args)
        {
            instance->logger->warn(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Error(spdlog::format_string_t<Args...> fmt, Args &&...args)
        {
            instance->logger->error(fmt, std::forward<Args>(args)...);
        }

        template <typename T>
        static void Trace(const T &msg)
        {
            instance->logger->trace(msg);
        }

        template <typename T>
        static void Debug(const T &msg)
        {
            instance->logger->debug(msg);
        }

        template <typename T>
        static void Info(const T &msg)
        {
            instance->logger->info(msg);
        }

        template <typename T>
        static void Warn(const T &msg)
        {
            instance->logger->warn(msg);
        }

        template <typename T>
        static void Error(const T &msg)
        {
            instance->logger->error(msg);
        }

    private:
        std::shared_ptr<spdlog::logger> logger;

        void init()
        {
            logger = spdlog::stdout_color_mt("vkEngine");
#ifdef VKE_DEBUG
            logger->set_level(spdlog::level::debug);
#endif
        }
    };

#define VKE_LOG_TRACE(...) vke_common::Logger::Trace(__VA_ARGS__);
#define VKE_LOG_DEBUG(...) vke_common::Logger::Debug(__VA_ARGS__);
#define VKE_LOG_INFO(...) vke_common::Logger::Info(__VA_ARGS__);
#define VKE_LOG_WARN(...) vke_common::Logger::Warn(__VA_ARGS__);
#define VKE_LOG_ERROR(...) vke_common::Logger::Error(__VA_ARGS__);
}

#endif