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

namespace core::ecs {

    // ============================================================================
    // Message System Core
    // ============================================================================

    class IMessage {
    public:
        virtual ~IMessage() = default;
        virtual std::type_index GetTypeIndex() const = 0;
    };

    template<typename T>
    class Message : public IMessage {
    public:
        T data;

        // Constructor to forward arguments to T's constructor
        template<typename... Args>
        explicit Message(Args&&... args) : data(std::forward<Args>(args)...) {}

        // Copy constructor
        explicit Message(const T& d) : data(d) {}

        // Move constructor
        explicit Message(T&& d) : data(std::move(d)) {}

        std::type_index GetTypeIndex() const override;

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
        std::unordered_map<std::type_index, std::vector<std::unique_ptr<IMessageHandler>>> m_handlers;
        std::queue<std::unique_ptr<IMessage>> m_message_queue;
        bool m_processing = false;

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

        // Message publishing
        template<typename MessageType, typename... Args>
        void Publish(Args&&... args);

        template<typename MessageType>
        void PublishMessage(const MessageType& message);

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