// ============================================================================
// ThreadSafeMessageBus.h - Thread-safe message bus for multi-threaded systems
// Location: include/core/threading/ThreadSafeMessageBus.h
// 
// Purpose: Provides thread-safe wrapper around MessageBus for use in
//          multi-threaded environments like ThreadedSystemManager
// ============================================================================

#pragma once

#include "core/ECS/MessageBus.h"
#include <mutex>
#include <shared_mutex>

namespace core::threading {

    /**
     * @brief Thread-safe wrapper around core::ecs::MessageBus
     * 
     * Provides thread-safe message publishing and handling for multi-threaded
     * system environments. Uses shared_mutex for reader-writer optimization
     * where multiple threads can read handlers simultaneously but writes are exclusive.
     */
    class ThreadSafeMessageBus {
    public:
        ThreadSafeMessageBus() = default;
        ~ThreadSafeMessageBus() = default;

        // Non-copyable, non-movable for thread safety
        ThreadSafeMessageBus(const ThreadSafeMessageBus&) = delete;
        ThreadSafeMessageBus& operator=(const ThreadSafeMessageBus&) = delete;
        ThreadSafeMessageBus(ThreadSafeMessageBus&&) = delete;
        ThreadSafeMessageBus& operator=(ThreadSafeMessageBus&&) = delete;

        /**
         * @brief Subscribe to messages of a specific type (thread-safe)
         * @tparam MessageType The type of message to subscribe to
         * @param handler Function to call when message is received
         */
        template<typename MessageType>
        void Subscribe(std::function<void(const MessageType&)> handler) {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            m_message_bus.Subscribe<MessageType>(std::move(handler));
        }

        /**
         * @brief Unsubscribe from messages of a specific type (thread-safe)
         * @tparam MessageType The type of message to unsubscribe from
         */
        template<typename MessageType>
        void Unsubscribe() {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            m_message_bus.Unsubscribe<MessageType>();
        }

        /**
         * @brief Publish message for processing (thread-safe)
         * @tparam MessageType The type of message to publish
         * @param message The message to publish
         */
        template<typename MessageType>
        void Publish(const MessageType& message) {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            m_message_bus.PublishMessage(message);
        }

        /**
         * @brief Publish message immediately without queuing (thread-safe)
         * @tparam MessageType The type of message to publish
         * @param message The message to publish immediately
         */
        template<typename MessageType>
        void PublishImmediate(const MessageType& message) {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            m_message_bus.PublishImmediate(message);
        }

        /**
         * @brief Process all queued messages (thread-safe)
         * Should be called from main thread or designated message processing thread
         */
        void ProcessQueuedMessages() {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            m_message_bus.ProcessQueuedMessages();
        }

        /**
         * @brief Clear all handlers and queued messages (thread-safe)
         */
        void Clear() {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            m_message_bus.Clear();
        }

        /**
         * @brief Get total number of registered handlers (thread-safe)
         * @return Number of handlers across all message types
         */
        size_t GetHandlerCount() const {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            return m_message_bus.GetHandlerCount();
        }

        /**
         * @brief Get number of queued messages (thread-safe)
         * @return Number of messages waiting to be processed
         */
        size_t GetQueuedMessageCount() const {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            return m_message_bus.GetQueuedMessageCount();
        }

        /**
         * @brief Get underlying MessageBus (for advanced usage, not thread-safe)
         * @warning Direct access bypasses thread safety - use with caution
         * @return Reference to underlying MessageBus
         */
        core::ecs::MessageBus& GetUnsafeMessageBus() {
            return m_message_bus;
        }

    private:
        mutable std::shared_mutex m_mutex;  ///< Reader-writer mutex for thread safety
        core::ecs::MessageBus m_message_bus; ///< Underlying message bus
    };

} // namespace core::threading