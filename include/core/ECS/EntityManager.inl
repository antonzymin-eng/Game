// ============================================================================
// EntityManager.inl - Template method implementations
// Location: include/core/ECS/EntityManager.inl
// ============================================================================

#pragma once

namespace core::ecs {

    // ============================================================================
    // EntityManager Template Methods
    // ============================================================================

    template<typename T, typename... Args>
    T* EntityManager::AddComponent(EntityID entity, Args&&... args) {
        assert(IsValidEntity(entity) && "Entity must be valid");

        auto* pool = GetOrCreateComponentPool<T>();
        return pool->AddComponent(entity, std::forward<Args>(args)...);
    }

    template<typename T>
    T* EntityManager::GetComponent(EntityID entity) {
        if (!IsValidEntity(entity)) {
            return nullptr;
        }

        auto* pool = GetComponentPool<T>();
        return pool ? pool->GetComponent(entity) : nullptr;
    }

    template<typename T>
    const T* EntityManager::GetComponent(EntityID entity) const {
        if (!IsValidEntity(entity)) {
            return nullptr;
        }

        const auto* pool = GetComponentPool<T>();
        return pool ? pool->GetComponent(entity) : nullptr;
    }

    template<typename T>
    bool EntityManager::HasComponent(EntityID entity) const {
        if (!IsValidEntity(entity)) {
            return false;
        }

        const auto* pool = GetComponentPool<T>();
        return pool ? pool->HasComponent(entity) : false;
    }

    template<typename T>
    bool EntityManager::RemoveComponent(EntityID entity) {
        if (!IsValidEntity(entity)) {
            return false;
        }

        auto* pool = GetComponentPool<T>();
        return pool ? pool->RemoveComponent(entity) : false;
    }

    template<typename... Components>
    std::vector<EntityID> EntityManager::GetEntitiesWith() const {
        std::vector<EntityID> result;

        // Get entities that have all required components
        for (EntityID entity : m_entities) {
            bool has_all_components = (HasComponent<Components>(entity) && ...);
            if (has_all_components) {
                result.push_back(entity);
            }
        }

        return result;
    }

    template<typename T>
    TypedComponentPool<T>* EntityManager::GetComponentPool() {
        auto type_index = std::type_index(typeid(T));
        auto it = m_component_pools.find(type_index);

        if (it != m_component_pools.end()) {
            return static_cast<TypedComponentPool<T>*>(it->second.get());
        }

        return nullptr;
    }

    template<typename T>
    const TypedComponentPool<T>* EntityManager::GetComponentPool() const {
        auto type_index = std::type_index(typeid(T));
        auto it = m_component_pools.find(type_index);

        if (it != m_component_pools.end()) {
            return static_cast<const TypedComponentPool<T>*>(it->second.get());
        }

        return nullptr;
    }

    template<typename T>
    size_t EntityManager::GetComponentCount() const {
        const auto* pool = GetComponentPool<T>();
        return pool ? pool->Size() : 0;
    }

    template<typename T>
    TypedComponentPool<T>* EntityManager::GetOrCreateComponentPool() {
        auto type_index = std::type_index(typeid(T));
        auto it = m_component_pools.find(type_index);

        if (it == m_component_pools.end()) {
            auto pool = std::make_unique<TypedComponentPool<T>>();
            auto* pool_ptr = pool.get();
            m_component_pools[type_index] = std::move(pool);
            return pool_ptr;
        }

        return static_cast<TypedComponentPool<T>*>(it->second.get());
    }

} // namespace core::ecs