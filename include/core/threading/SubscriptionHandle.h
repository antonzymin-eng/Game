// ============================================================================
// SubscriptionHandle.h - RAII wrapper for event subscriptions
// Created: December 4, 2025
// Purpose: Automatically unsubscribe from events when handle is destroyed
// ============================================================================

#pragma once

#include <functional>
#include <utility>

namespace core {
namespace threading {

/**
 * @brief RAII handle for message bus subscriptions
 *
 * Automatically unsubscribes when destroyed, preventing use-after-free
 * when a subscriber object is destroyed while events are still queued.
 *
 * Usage:
 *   class MySystem {
 *       std::vector<SubscriptionHandle> m_subscriptions;
 *   public:
 *       MySystem(MessageBus& bus) {
 *           m_subscriptions.push_back(
 *               bus.Subscribe<MyEvent>([this](const MyEvent& e) { OnMyEvent(e); })
 *           );
 *       }
 *       // Destructor automatically unsubscribes via RAII
 *   };
 */
class SubscriptionHandle {
public:
    using UnsubscribeFn = std::function<void()>;

    /**
     * Construct a subscription handle with an unsubscribe callback
     * @param unsubscribe Function to call when handle is destroyed
     */
    explicit SubscriptionHandle(UnsubscribeFn unsubscribe)
        : m_unsubscribe(std::move(unsubscribe)) {}

    /**
     * Destructor - automatically unsubscribes
     */
    ~SubscriptionHandle() {
        if (m_unsubscribe) {
            m_unsubscribe();
        }
    }

    /**
     * Move constructor - transfers ownership
     */
    SubscriptionHandle(SubscriptionHandle&& other) noexcept
        : m_unsubscribe(std::move(other.m_unsubscribe)) {
        other.m_unsubscribe = nullptr;
    }

    /**
     * Move assignment - transfers ownership
     */
    SubscriptionHandle& operator=(SubscriptionHandle&& other) noexcept {
        if (this != &other) {
            // Unsubscribe current subscription
            if (m_unsubscribe) {
                m_unsubscribe();
            }

            // Transfer ownership
            m_unsubscribe = std::move(other.m_unsubscribe);
            other.m_unsubscribe = nullptr;
        }
        return *this;
    }

    // Non-copyable (subscription should not be duplicated)
    SubscriptionHandle(const SubscriptionHandle&) = delete;
    SubscriptionHandle& operator=(const SubscriptionHandle&) = delete;

    /**
     * Manually unsubscribe before destruction
     */
    void Unsubscribe() {
        if (m_unsubscribe) {
            m_unsubscribe();
            m_unsubscribe = nullptr;
        }
    }

    /**
     * Check if this handle is active (has an unsubscribe function)
     */
    bool IsActive() const {
        return m_unsubscribe != nullptr;
    }

private:
    UnsubscribeFn m_unsubscribe;
};

} // namespace threading
} // namespace core
