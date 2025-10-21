#pragma once

#include <cstdint>

namespace core::threading {
    enum class ThreadingStrategy {
        MAIN_THREAD,        // Run on main thread only
        THREAD_POOL,        // Use shared thread pool
        DEDICATED_THREAD,   // Each system gets own thread
        BACKGROUND_THREAD,  // Run on background threads
        HYBRID              // Mix of strategies
    };
}