#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

namespace ui {

// Simple thread-safe ring buffer for debug HUD logging
static std::mutex g_log_mtx;
static std::vector<std::string> g_log;
static constexpr size_t kMaxLog = 256;

void HUDLog(const char* fmt, ...) {
    if (!fmt) return;
    char buf[1024];
    va_list args;
    va_start(args, fmt);
#if defined(_MSC_VER)
    _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
#else
    vsnprintf(buf, sizeof(buf), fmt, args);
#endif
    va_end(args);

    std::lock_guard<std::mutex> lock(g_log_mtx);
    g_log.emplace_back(buf);
    if (g_log.size() > kMaxLog) g_log.erase(g_log.begin(), g_log.begin() + (g_log.size() - kMaxLog));
}

} // namespace ui