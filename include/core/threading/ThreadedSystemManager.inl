// ============================================================================
// ThreadedSystemManager.inl - Template Implementation
// Location: src/core/Threading/ThreadedSystemManager.inl
// ============================================================================

#pragma once

namespace core::threading {

    // ============================================================================
    // ThreadPool Template Implementation
    // ============================================================================

    template<typename F, typename... Args>
    auto ThreadPool::Submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);

            if (!m_running) {
                throw std::runtime_error("Cannot submit task to stopped ThreadPool");
            }

            m_tasks.emplace([task]() { (*task)(); });
        }

        m_condition.notify_one();
        return result;
    }

    // ============================================================================
    // ThreadedSystemManager Template Implementation
    // ============================================================================

    template<typename SystemType, typename... Args>
    SystemType* ThreadedSystemManager::AddSystem(ThreadingStrategy strategy, Args&&... args) {
        static_assert(std::is_base_of_v<core::ecs::ISystem, SystemType>,
            "SystemType must inherit from ISystem");

        // Create the system instance
        auto system = std::make_unique<SystemType>(std::forward<Args>(args)...);
        SystemType* system_ptr = system.get();

        // Add the system using the runtime version
        AddSystem(std::move(system), strategy);

        return system_ptr;
    }

    template<typename SystemType>
    SystemType* ThreadedSystemManager::GetSystem() {
        static_assert(std::is_base_of_v<core::ecs::ISystem, SystemType>,
            "SystemType must inherit from ISystem");

        std::lock_guard<std::mutex> lock(m_systems_mutex);

        for (const auto& system : m_systems) {
            // Use dynamic_cast to check if this system is of the requested type
            if (auto typed_system = dynamic_cast<SystemType*>(system.get())) {
                return typed_system;
            }
        }

        return nullptr;
    }

} // namespace core::threading