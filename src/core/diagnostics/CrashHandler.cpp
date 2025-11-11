// ============================================================================
// CrashHandler.cpp - Cross-platform crash dump collection utilities
// ============================================================================

#include "core/diagnostics/CrashHandler.h"
#include "core/logging/Logger.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <vector>

#if defined(_WIN32)
#    include <Windows.h>
#    include <DbgHelp.h>
#    pragma comment(lib, "DbgHelp.lib")
#else
#    include <unistd.h>
#    if defined(__linux__)
#        include <execinfo.h>
#    endif
#endif

namespace core::diagnostics {

namespace {

std::atomic<bool> g_initialized{false};
CrashHandlerConfig g_config{};
std::mutex g_breadcrumb_mutex;
std::vector<std::string> g_breadcrumbs;

std::filesystem::path EnsureDumpDirectory() {
    if (g_config.dump_directory.empty()) {
        g_config.dump_directory = std::filesystem::current_path() / "crash_dumps";
    }
    std::error_code ec;
    std::filesystem::create_directories(g_config.dump_directory, ec);
    if (ec) {
        CORE_LOGF_WARN("CrashHandler", "Failed to create dump directory '" << g_config.dump_directory.string()
                                        << "': " << ec.message());
    }
    return g_config.dump_directory;
}

std::string CurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time_t_now);
#else
    localtime_r(&time_t_now, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

std::filesystem::path WriteBreadcrumbFile(const std::filesystem::path& dump_path) {
    if (g_breadcrumbs.empty()) {
        return {};
    }

    auto breadcrumb_path = dump_path;
    breadcrumb_path += ".breadcrumbs.txt";
    std::ofstream ofs(breadcrumb_path, std::ios::out | std::ios::trunc);
    if (!ofs) {
        CORE_LOGF_WARN("CrashHandler", "Failed to write breadcrumb file '" << breadcrumb_path.string() << "'");
        return {};
    }

    ofs << "Mechanica Imperii Crash Breadcrumbs\n";
    ofs << "---------------------------------\n";
    for (const auto& entry : g_breadcrumbs) {
        ofs << entry << '\n';
    }
    ofs.flush();
    return breadcrumb_path;
}

#if defined(_WIN32)

LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* exception_info) {
    const auto dump_dir = EnsureDumpDirectory();
    const auto dump_path = dump_dir / ("mechanica_crash_" + CurrentTimestamp() + ".dmp");

    HANDLE file = CreateFileW(dump_path.wstring().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION dump_info{};
        dump_info.ThreadId = GetCurrentThreadId();
        dump_info.ExceptionPointers = exception_info;
        dump_info.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpWithFullMemory,
                          &dump_info, nullptr, nullptr);
        CloseHandle(file);
        CORE_LOGF_ERROR("CrashHandler", "Wrote crash dump to " << dump_path.string());
    } else {
        CORE_LOGF_ERROR("CrashHandler", "Failed to open dump file '" << dump_path.string() << "'");
    }

    {
        std::lock_guard<std::mutex> lock(g_breadcrumb_mutex);
        WriteBreadcrumbFile(dump_path);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

#else

void SignalHandler(int signal_number, siginfo_t* info, void* context) {
    (void)info;
    (void)context;
    const auto dump_dir = EnsureDumpDirectory();
    const auto dump_path = dump_dir / ("mechanica_crash_" + CurrentTimestamp() + ".log");
    std::ofstream ofs(dump_path, std::ios::out | std::ios::trunc);
    if (ofs) {
        ofs << "Signal: " << signal_number << '\n';
#if defined(__linux__)
        if (g_config.capture_backtraces) {
            void* callstack[64];
            const int frames = backtrace(callstack, 64);
            char** symbols = backtrace_symbols(callstack, frames);
            ofs << "Backtrace (" << frames << " frames):\n";
            for (int i = 0; i < frames; ++i) {
                ofs << symbols[i] << '\n';
            }
            free(symbols);
        }
#endif
        ofs.flush();
        CORE_LOGF_ERROR("CrashHandler", "Wrote crash log to " << dump_path.string());
    }

    {
        std::lock_guard<std::mutex> lock(g_breadcrumb_mutex);
        WriteBreadcrumbFile(dump_path);
    }

    // Re-raise with default handler to terminate the process as expected
    signal(signal_number, SIG_DFL);
    raise(signal_number);
}

void InstallSignalHandler(int signal_number) {
    struct sigaction action{};
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = SignalHandler;
    sigemptyset(&action.sa_mask);
    sigaction(signal_number, &action, nullptr);
}

#endif

} // namespace

void InitializeCrashHandling(const CrashHandlerConfig& config) {
    if (g_initialized.exchange(true)) {
        return;
    }

    g_config = config;
    EnsureDumpDirectory();

#if defined(_WIN32)
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
#else
    InstallSignalHandler(SIGSEGV);
    InstallSignalHandler(SIGABRT);
    InstallSignalHandler(SIGILL);
    InstallSignalHandler(SIGFPE);
#endif

    if (config.enable_symbol_linkage) {
        CORE_LOG_INFO("CrashHandler", "Crash handler initialized with symbol linkage enabled");
    } else {
        CORE_LOG_INFO("CrashHandler", "Crash handler initialized");
    }
}

std::filesystem::path GetCrashDumpDirectory() {
    return EnsureDumpDirectory();
}

void AppendCrashBreadcrumb(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_breadcrumb_mutex);
    g_breadcrumbs.push_back(message);
}

} // namespace core::diagnostics

