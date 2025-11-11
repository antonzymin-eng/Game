// ============================================================================
// Logger.h - Core logging infrastructure with level filtering & diagnostics
// Created: October 11, 2025
// Updated: January 18, 2026 - Level standardization, verbose diagnostics
// Location: include/core/logging/Logger.h
// ============================================================================
#pragma once

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace core {
namespace logging {

enum class LogLevel {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off
};

namespace detail {

inline std::atomic<LogLevel>& GlobalLogLevel() {
    static std::atomic<LogLevel> level{LogLevel::Info};
    return level;
}

inline std::mutex& GlobalLogMutex() {
    static std::mutex mutex;
    return mutex;
}

inline const char* ToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warn: return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        case LogLevel::Off: return "OFF";
        default: return "UNKNOWN";
    }
}

inline std::string MakeTimestamp() {
    using clock = std::chrono::system_clock;
    auto now = clock::now();
    auto time_t_now = clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time_t_now);
#else
    localtime_r(&time_t_now, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    oss << '.' << std::setw(6) << std::setfill('0') << micros.count();
    return oss.str();
}

template <typename T>
inline std::string ToLogString(T&& value) {
    using Decayed = std::decay_t<T>;
    if constexpr (std::is_same_v<Decayed, std::string>) {
        return std::forward<T>(value);
    } else if constexpr (std::is_same_v<Decayed, const char*>) {
        return value ? std::string(value) : std::string{};
    } else if constexpr (std::is_same_v<Decayed, char*>) {
        return value ? std::string(value) : std::string{};
    } else if constexpr (std::is_convertible_v<Decayed, std::string_view>) {
        std::string_view view = value;
        return std::string(view);
    } else if constexpr (std::is_arithmetic_v<Decayed>) {
        if constexpr (std::is_same_v<Decayed, bool>) {
            return value ? "true" : "false";
        } else {
            return std::to_string(value);
        }
    } else {
        std::ostringstream oss;
        oss << std::forward<T>(value);
        return oss.str();
    }
}

inline void WriteLogLine(LogLevel level, const std::string& system, const std::string& message) {
    std::lock_guard<std::mutex> lock(GlobalLogMutex());
    std::ostream& stream = (level >= LogLevel::Error) ? std::cerr : std::cout;
    stream << '[' << MakeTimestamp() << "][" << ToString(level) << "][" << system << "] " << message << std::endl;
}

inline std::string FormatMessageBusEvent(std::string_view event, std::string_view topic, std::string_view payload) {
    std::ostringstream oss;
    oss << event;
    if (!topic.empty()) {
        oss << " topic=" << topic;
    }
    if (!payload.empty()) {
        oss << " payload=" << payload;
    }
    return oss.str();
}

inline std::string FormatEcsLifecycleEvent(std::string_view action, uint64_t entity_id, std::string_view entity_name) {
    std::ostringstream oss;
    oss << action << " entity id=" << entity_id;
    if (!entity_name.empty()) {
        oss << " name=" << entity_name;
    }
    return oss.str();
}

} // namespace detail

inline void SetGlobalLogLevel(LogLevel level) {
    detail::GlobalLogLevel().store(level, std::memory_order_relaxed);
}

inline LogLevel GetGlobalLogLevel() {
    return detail::GlobalLogLevel().load(std::memory_order_relaxed);
}

inline bool IsLevelEnabled(LogLevel level) {
    return level >= detail::GlobalLogLevel().load(std::memory_order_relaxed) && level != LogLevel::Off;
}

inline void Log(LogLevel level, std::string system, std::string message) {
    if (!IsLevelEnabled(level)) {
        return;
    }
    detail::WriteLogLine(level, system, message);
}

template <typename System, typename Message>
inline void Log(LogLevel level, System&& system, Message&& message) {
    if (!IsLevelEnabled(level)) {
        return;
    }
    detail::WriteLogLine(level,
                         detail::ToLogString(std::forward<System>(system)),
                         detail::ToLogString(std::forward<Message>(message)));
}

#define CORE_LOG(level, system, message) \
    ::core::logging::Log(level, (system), (message))

#define CORE_LOG_TRACE(system, message) \
    CORE_LOG(::core::logging::LogLevel::Trace, ::core::logging::detail::ToLogString(system), ::core::logging::detail::ToLogString(message))

#define CORE_LOG_DEBUG(system, message) \
    CORE_LOG(::core::logging::LogLevel::Debug, ::core::logging::detail::ToLogString(system), ::core::logging::detail::ToLogString(message))

#define CORE_LOG_INFO(system, message) \
    CORE_LOG(::core::logging::LogLevel::Info, ::core::logging::detail::ToLogString(system), ::core::logging::detail::ToLogString(message))

#define CORE_LOG_WARN(system, message) \
    CORE_LOG(::core::logging::LogLevel::Warn, ::core::logging::detail::ToLogString(system), ::core::logging::detail::ToLogString(message))

#define CORE_LOG_ERROR(system, message) \
    CORE_LOG(::core::logging::LogLevel::Error, ::core::logging::detail::ToLogString(system), ::core::logging::detail::ToLogString(message))

#define CORE_LOG_CRITICAL(system, message) \
    CORE_LOG(::core::logging::LogLevel::Critical, ::core::logging::detail::ToLogString(system), ::core::logging::detail::ToLogString(message))

#define CORE_LOG_STREAM(level, system, expression) \
    do { \
        if (::core::logging::IsLevelEnabled(level)) { \
            std::ostringstream core_logging_stream__; \
            core_logging_stream__ << expression; \
            ::core::logging::Log(level, ::core::logging::detail::ToLogString(system), core_logging_stream__.str()); \
        } \
    } while (0)

#define CORE_LOGF_TRACE(system, expression) \
    CORE_LOG_STREAM(::core::logging::LogLevel::Trace, system, expression)

#define CORE_LOGF_DEBUG(system, expression) \
    CORE_LOG_STREAM(::core::logging::LogLevel::Debug, system, expression)

#define CORE_LOGF_INFO(system, expression) \
    CORE_LOG_STREAM(::core::logging::LogLevel::Info, system, expression)

#define CORE_LOGF_WARN(system, expression) \
    CORE_LOG_STREAM(::core::logging::LogLevel::Warn, system, expression)

#define CORE_LOGF_ERROR(system, expression) \
    CORE_LOG_STREAM(::core::logging::LogLevel::Error, system, expression)

#define CORE_LOGF_CRITICAL(system, expression) \
    CORE_LOG_STREAM(::core::logging::LogLevel::Critical, system, expression)

class StreamLogBuilder {
public:
    StreamLogBuilder(LogLevel level, std::string system)
        : m_level(level), m_system(std::move(system)) {}

    StreamLogBuilder(StreamLogBuilder&& other) noexcept
        : m_level(other.m_level), m_system(std::move(other.m_system)), m_stream(std::move(other.m_stream)) {
        other.m_level = LogLevel::Off;
    }

    ~StreamLogBuilder() {
        if (m_level != LogLevel::Off) {
            ::core::logging::Log(m_level, m_system, m_stream.str());
        }
    }

    template <typename T>
    StreamLogBuilder& operator<<(T&& value) & {
        m_stream << std::forward<T>(value);
        return *this;
    }

    template <typename T>
    StreamLogBuilder&& operator<<(T&& value) && {
        m_stream << std::forward<T>(value);
        return std::move(*this);
    }

private:
    LogLevel m_level;
    std::string m_system;
    std::ostringstream m_stream;
};

inline StreamLogBuilder MakeStreamLogger(LogLevel level, std::string system) {
    if (!IsLevelEnabled(level)) {
        return StreamLogBuilder(LogLevel::Off, std::move(system));
    }
    return StreamLogBuilder(level, std::move(system));
}

#define CORE_STREAM_LOG(level, system) \
    ::core::logging::MakeStreamLogger(level, ::core::logging::detail::ToLogString(system))

#define CORE_STREAM_TRACE(system) CORE_STREAM_LOG(::core::logging::LogLevel::Trace, system)
#define CORE_STREAM_DEBUG(system) CORE_STREAM_LOG(::core::logging::LogLevel::Debug, system)
#define CORE_STREAM_INFO(system)  CORE_STREAM_LOG(::core::logging::LogLevel::Info, system)
#define CORE_STREAM_WARN(system)  CORE_STREAM_LOG(::core::logging::LogLevel::Warn, system)
#define CORE_STREAM_ERROR(system) CORE_STREAM_LOG(::core::logging::LogLevel::Error, system)
#define CORE_STREAM_CRITICAL(system) CORE_STREAM_LOG(::core::logging::LogLevel::Critical, system)

// Backwards-compatible helpers for legacy call sites
inline void LogInfo(const std::string& system, const std::string& msg) {
    CORE_LOG_INFO(system, msg);
}

inline void LogWarning(const std::string& system, const std::string& msg) {
    CORE_LOG_WARN(system, msg);
}

inline void LogError(const std::string& system, const std::string& msg) {
    CORE_LOG_ERROR(system, msg);
}

inline void LogDebug(const std::string& system, const std::string& msg) {
    CORE_LOG_DEBUG(system, msg);
}

namespace detail {

inline void LogMessageBus(std::string_view event, std::string_view topic, std::string payload) {
    ::core::logging::Log(LogLevel::Trace,
                         detail::ToLogString("MessageBus"),
                         FormatMessageBusEvent(event, topic, payload));
}

inline void LogEcsLifecycle(std::string_view action, uint64_t entity_id, std::string_view entity_name) {
    ::core::logging::Log(LogLevel::Trace,
                         detail::ToLogString("ECS.EntityLifecycle"),
                         FormatEcsLifecycleEvent(action, entity_id, entity_name));
}

} // namespace detail

#if defined(ENABLE_VERBOSE_DIAGNOSTICS)
#define CORE_VERBOSE_DIAGNOSTICS 1
#else
#define CORE_VERBOSE_DIAGNOSTICS 0
#endif

#if CORE_VERBOSE_DIAGNOSTICS && defined(ENABLE_MESSAGE_BUS_TRACE)
#define CORE_TRACE_MESSAGE_BUS(event, topic, payload) \
    ::core::logging::detail::LogMessageBus((event), (topic), ::core::logging::detail::ToLogString(payload))
#else
#define CORE_TRACE_MESSAGE_BUS(event, topic, payload) \
    do { (void)(event); (void)(topic); (void)(payload); } while (0)
#endif

#if CORE_VERBOSE_DIAGNOSTICS && defined(ENABLE_ECS_LIFECYCLE_TRACE)
#define CORE_TRACE_ECS_LIFECYCLE(action, entity_id, entity_name) \
    ::core::logging::detail::LogEcsLifecycle((action), static_cast<uint64_t>(entity_id), (entity_name))
#else
#define CORE_TRACE_ECS_LIFECYCLE(action, entity_id, entity_name) \
    do { (void)(action); (void)(entity_id); (void)(entity_name); } while (0)
#endif

} // namespace logging
} // namespace core
