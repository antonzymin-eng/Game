// ============================================================================
// MessageBus.inl - Template method implementations
// Location: src/core/ECS/MessageBus.inl
// ============================================================================

#pragma once

namespace core::ecs {

    // ============================================================================
    // MessageBus Template Methods
    // ============================================================================

    template<typename MessageType>
    void MessageBus::Subscribe(std::function<void(const MessageType&)> handler) {
        auto type_index = std::type_index(typeid(MessageType));
        auto message_handler = std::make_unique<MessageHandler<MessageType>>(std::move(handler));
        m_handlers[type_index].push_back(std::move(message_handler));
    }

    template<typename MessageType, typename... Args>
    void MessageBus::Publish(Args&&... args) {
        auto message = std::make_unique<MessageType>(std::forward<Args>(args)...);

        if (m_processing) {
            // Queue message to avoid recursive processing issues
            m_message_queue.push(std::move(message));
        }
        else {
            // Process immediately
            PublishImmediate(*message);
        }
    }

    template<typename MessageType>
    void MessageBus::PublishMessage(const MessageType& message) {
        auto message_copy = std::make_unique<MessageType>(message);

        if (m_processing) {
            // Queue message to avoid recursive processing issues
            m_message_queue.push(std::move(message_copy));
        }
        else {
            // Process immediately
            PublishImmediate(*message_copy);
        }
    }

    template<typename MessageType>
    void MessageBus::Unsubscribe() {
        auto type_index = std::type_index(typeid(MessageType));
        m_handlers.erase(type_index);
    }

} // namespace core::ecs