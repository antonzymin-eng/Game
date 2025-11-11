// ============================================================================
// MessageBus.cpp - Type-safe event communication implementation
// Location: src/core/ECS/MessageBus.cpp
// ============================================================================

#include "core/ECS/MessageBus.h"
#include "core/logging/Logger.h"

#include <string>

namespace core::ecs {

    // ============================================================================
    // MessageBus Implementation (Non-template methods only)
    // ============================================================================

    void MessageBus::ProcessQueuedMessages() {
        if (m_processing) {
            CORE_TRACE_MESSAGE_BUS("reentry_guard", "queue", "already processing");
            return; // Prevent recursive processing
        }

        CORE_TRACE_MESSAGE_BUS("process_start", "queue", std::to_string(m_message_queue.size()));
        m_processing = true;

        while (!m_message_queue.empty()) {
            auto message = std::move(m_message_queue.front());
            m_message_queue.pop();

            CORE_TRACE_MESSAGE_BUS("dispatch", message->GetTypeIndex().name(), "queued message");
            PublishImmediate(*message);
        }

        m_processing = false;
        CORE_TRACE_MESSAGE_BUS("process_end", "queue", std::to_string(m_message_queue.size()));
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
            CORE_TRACE_MESSAGE_BUS("publish_immediate", message.GetTypeIndex().name(),
                                   "handlers=" + std::to_string(it->second.size()));
            for (const auto& handler : it->second) {
                handler->HandleMessage(message);
            }
        }
    }

} // namespace core::ecs