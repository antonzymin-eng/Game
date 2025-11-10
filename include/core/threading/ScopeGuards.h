// ============================================================================
// ScopeGuards.h - Reusable RAII guards for automatic resource management
// Location: include/core/threading/ScopeGuards.h
// ============================================================================

#pragma once

#include <atomic>
#include <functional>

namespace core::threading {

    // ============================================================================
    // AtomicCounterGuard - RAII guard for atomic counter increment/decrement
    // ============================================================================

    /// RAII guard that increments an atomic counter on construction and
    /// decrements it on destruction. Exception-safe.
    ///
    /// Usage:
    ///   std::atomic<size_t> counter{0};
    ///   {
    ///       AtomicCounterGuard guard(counter);
    ///       // counter == 1
    ///       DoWork();  // May throw
    ///   } // counter automatically decremented to 0, even if DoWork() throws
    class AtomicCounterGuard {
    public:
        /// Constructs guard and increments counter
        explicit AtomicCounterGuard(std::atomic<size_t>& counter)
            : m_counter(counter) {
            m_counter.fetch_add(1, std::memory_order_release);
        }

        /// Destructs guard and decrements counter
        ~AtomicCounterGuard() {
            m_counter.fetch_sub(1, std::memory_order_release);
        }

        // Non-copyable and non-movable (holds reference)
        AtomicCounterGuard(const AtomicCounterGuard&) = delete;
        AtomicCounterGuard& operator=(const AtomicCounterGuard&) = delete;
        AtomicCounterGuard(AtomicCounterGuard&&) = delete;
        AtomicCounterGuard& operator=(AtomicCounterGuard&&) = delete;

    private:
        std::atomic<size_t>& m_counter;
    };

    // ============================================================================
    // ScopeExit - Generic RAII guard that executes a function on scope exit
    // ============================================================================

    /// RAII guard that executes a function on destruction. Exception-safe.
    ///
    /// Usage:
    ///   {
    ///       ScopeExit cleanup([&]() { CleanupResources(); });
    ///       DoWork();  // May throw
    ///   } // CleanupResources() called automatically
    class ScopeExit {
    public:
        /// Constructs guard with cleanup function
        explicit ScopeExit(std::function<void()> cleanup)
            : m_cleanup(std::move(cleanup))
            , m_active(true) {
        }

        /// Executes cleanup function if still active
        ~ScopeExit() {
            if (m_active && m_cleanup) {
                try {
                    m_cleanup();
                } catch (...) {
                    // Suppress exceptions in destructor
                }
            }
        }

        // Non-copyable
        ScopeExit(const ScopeExit&) = delete;
        ScopeExit& operator=(const ScopeExit&) = delete;

        // Movable
        ScopeExit(ScopeExit&& other) noexcept
            : m_cleanup(std::move(other.m_cleanup))
            , m_active(other.m_active) {
            other.m_active = false;
        }

        ScopeExit& operator=(ScopeExit&& other) noexcept {
            if (this != &other) {
                // Execute current cleanup if active
                if (m_active && m_cleanup) {
                    try {
                        m_cleanup();
                    } catch (...) {
                        // Suppress exceptions
                    }
                }
                m_cleanup = std::move(other.m_cleanup);
                m_active = other.m_active;
                other.m_active = false;
            }
            return *this;
        }

        /// Manually release the guard without executing cleanup
        void Release() noexcept {
            m_active = false;
        }

    private:
        std::function<void()> m_cleanup;
        bool m_active;
    };

} // namespace core::threading
