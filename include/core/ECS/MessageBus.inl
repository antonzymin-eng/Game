// ============================================================================
// Date/Time Created: Sunday, October 26, 2025 - 3:15 PM PST
// Intended Folder Location: include/core/ECS/MessageBus.inl
// MessageBus.inl - Template method implementations
// ============================================================================

#pragma once

namespace core::ecs {

    // ============================================================================
    // Message Template Implementation
    // ============================================================================

    template<typename T>
    std::type_index Message<T>::GetTypeIndex() const {
        return std::type_index(typeid(T));
    }

    // ============================================================================
    // MessageHandler Template Implementation
    // ============================================================================

    template<typename MessageType>
    MessageHandler<MessageType>::MessageHandler(std::function<void(const MessageType&)> handler)
        : m_handler(std::move(handler)) {
    }

    template<typename MessageType>
    void MessageHandler<MessageType>::HandleMessage(const IMessage& message) {
        // Use dynamic_cast to safely cast from IMessage to Message<MessageType>
        const Message<MessageType>* msg_wrapper = dynamic_cast<const Message<MessageType>*>(&message);
        if (msg_wrapper) {
            // Extract the actual message data from the wrapper
            m_handler(msg_wrapper->GetData());
        }
    }

    template<typename MessageType>
    std::type_index MessageHandler<MessageType>::GetMessageType() const {
        return std::type_index(typeid(MessageType));
    }

    // ============================================================================
    // MessageBus Template Implementation
    // ============================================================================

    template<typename MessageType>
    void MessageBus::Subscribe(std::function<void(const MessageType&)> handler) {
        auto type_index = std::type_index(typeid(MessageType));
        auto message_handler = std::make_unique<MessageHandler<MessageType>>(std::move(handler));
        m_handlers[type_index].push_back(std::move(message_handler));
    }

    template<typename MessageType, typename... Args>
    void MessageBus::Publish(Args&&... args) {
        auto message = std::make_unique<Message<MessageType>>(std::forward<Args>(args)...);
        m_message_queue.push(std::move(message));
    }

    template<typename MessageType>
    void MessageBus::PublishMessage(const MessageType& message) {
        auto message_wrapper = std::make_unique<Message<MessageType>>(message);
        m_message_queue.push(std::move(message_wrapper));
    }

    template<typename MessageType>
    void MessageBus::Unsubscribe() {
        auto type_index = std::type_index(typeid(MessageType));
        m_handlers.erase(type_index);
    }

} // namespace core::ecs
