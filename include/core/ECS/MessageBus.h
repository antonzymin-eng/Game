// ============================================================================
// MessageBus.h - Type-safe event communication system
// Location: include/core/ECS/MessageBus.h
// ============================================================================

#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <typeindex>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <cstdint>

namespace core::ecs {

    // ============================================================================
    // Message Priority System
    // ============================================================================

    enum class MessagePriority : uint8_t {
        LOW = 0,      // Background tasks, statistics updates
        NORMAL = 1,   // Default priority for most messages
        HIGH = 2,     // Important events (diplomatic proposals, tech unlocks)
        CRITICAL = 3  // Game-critical events (war declarations, battles, succession)
    };

    // ============================================================================
    // Message System Core
    // ============================================================================

    class IMessage {
    public:
        virtual ~IMessage() = default;
        virtual std::type_index GetTypeIndex() const = 0;
        virtual MessagePriority GetPriority() const = 0;
    };

    template<typename T>
    class Message : public IMessage {
    public:
        T data;
        MessagePriority priority;

        // Constructor to forward arguments to T's constructor
        template<typename... Args>
        explicit Message(Args&&... args)
            : data(std::forward<Args>(args)...), priority(MessagePriority::NORMAL) {}

        // Constructor with explicit priority
        template<typename... Args>
        explicit Message(MessagePriority prio, Args&&... args)
            : data(std::forward<Args>(args)...), priority(prio) {}

        // Copy constructor
        explicit Message(const T& d, MessagePriority prio = MessagePriority::NORMAL)
            : data(d), priority(prio) {}

        // Move constructor
        explicit Message(T&& d, MessagePriority prio = MessagePriority::NORMAL)
            : data(std::move(d)), priority(prio) {}

        std::type_index GetTypeIndex() const override;
        MessagePriority GetPriority() const override { return priority; }

        // Access to the underlying data
        const T& GetData() const { return data; }
        T& GetData() { return data; }
    };

    // ============================================================================
    // Message Handler System
    // ============================================================================

    class IMessageHandler {
    public:
        virtual ~IMessageHandler() = default;
        virtual void HandleMessage(const IMessage& message) = 0;
        virtual std::type_index GetMessageType() const = 0;
    };

    template<typename MessageType>
    class MessageHandler : public IMessageHandler {
    private:
        std::function<void(const MessageType&)> m_handler;

    public:
        explicit MessageHandler(std::function<void(const MessageType&)> handler);

        void HandleMessage(const IMessage& message) override;
        std::type_index GetMessageType() const override;
    };

    // ============================================================================
    // Message Bus - Decoupled communication
    // ============================================================================

    class MessageBus {
    private:
        // Message wrapper for priority queue
        struct PrioritizedMessage {
            std::unique_ptr<IMessage> message;
            MessagePriority priority;
            uint64_t sequence;  // For FIFO within same priority

            bool operator<(const PrioritizedMessage& other) const {
                // Higher priority first; if equal, earlier sequence first
                if (priority != other.priority) {
                    return priority < other.priority;  // Lower value = lower priority
                }
                return sequence > other.sequence;  // Higher sequence = lower priority
            }
        };

        std::unordered_map<std::type_index, std::vector<std::unique_ptr<IMessageHandler>>> m_handlers;
        std::priority_queue<PrioritizedMessage> m_message_queue;
        std::atomic<bool> m_processing{false};  // FIXED: Use atomic instead of mutex
        std::atomic<uint64_t> m_sequence{0};    // For message ordering

        // Thread safety
        mutable std::shared_mutex m_handlers_mutex;  // For handler map (allows concurrent reads)
        mutable std::mutex m_queue_mutex;            // For message queue

    public:
        MessageBus() = default;
        ~MessageBus() = default;

        // Non-copyable
        MessageBus(const MessageBus&) = delete;
        MessageBus& operator=(const MessageBus&) = delete;

        // Movable
        MessageBus(MessageBus&&) = default;
        MessageBus& operator=(MessageBus&&) = default;

        // Message subscription
        template<typename MessageType>
        void Subscribe(std::function<void(const MessageType&)> handler);

        // Message publishing with priority support
        template<typename MessageType, typename... Args>
        void Publish(Args&&... args);

        template<typename MessageType, typename... Args>
        void PublishWithPriority(MessagePriority priority, Args&&... args);

        template<typename MessageType>
        void PublishMessage(const MessageType& message, MessagePriority priority = MessagePriority::NORMAL);

        // Queue management
        void ProcessQueuedMessages();
        void Clear();

        // Unsubscription
        template<typename MessageType>
        void Unsubscribe();

        // Statistics
        size_t GetHandlerCount() const;
        size_t GetQueuedMessageCount() const;

    private:
        void PublishImmediate(const IMessage& message);
    };

} // namespace core::ecs

// Template definitions
#include "MessageBus.inl"