// ============================================================================
// MessageBus.cpp - Type-safe event communication implementation
// Location: src/core/ECS/MessageBus.cpp
// ============================================================================

#include "MessageBus.h"

namespace core::ecs {

    // ============================================================================
    // Message Template Implementation
    // ============================================================================

    template<typename T>
    std::type_index Message<T>::GetTypeIndex() const {
        return std::type_index(typeid(T));
    }

    // ============================================================================
    // MessageHandler Implementation
    // ============================================================================

    template<typename MessageType>
    MessageHandler<MessageType>::MessageHandler(std::function<void(const MessageType&)> handler)
        : m_handler(std::move(handler)) {
    }

    template<typename MessageType>
    void MessageHandler<MessageType>::HandleMessage(const IMessage& message) {
        const MessageType& typed_message = static_cast<const MessageType&>(message);
        m_handler(typed_message);
    }

    template<typename MessageType>
    std::type_index MessageHandler<MessageType>::GetMessageType() const {
        return std::type_index(typeid(MessageType));
    }

    // ============================================================================
    // MessageBus Implementation
    // ============================================================================

    void MessageBus::ProcessQueuedMessages() {
        if (m_processing) {
            return; // Prevent recursive processing
        }

        m_processing = true;

        while (!m_message_queue.empty()) {
            auto message = std::move(m_message_queue.front());
            m_message_queue.pop();

            PublishImmediate(*message);
        }

        m_processing = false;
    }

    void MessageBus::Clear() {
        m_handlers.clear();

        // Clear message queue
        while (!m_message_queue.empty()) {
            m_message_queue.pop();
        }
    }

    size_t MessageBus::GetHandlerCount() const {
        size_t total = 0;
        for (const auto& [type_index, handlers] : m_handlers) {
            total += handlers.size();
        }
        return total;
    }

    size_t MessageBus::GetQueuedMessageCount() const {
        return m_message_queue.size();
    }

    void MessageBus::PublishImmediate(const IMessage& message) {
        auto type_index = message.GetTypeIndex();
        auto it = m_handlers.find(type_index);

        if (it != m_handlers.end()) {
            for (const auto& handler : it->second) {
                handler->HandleMessage(message);
            }
        }
    }

} // namespace core::ecs