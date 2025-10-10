// ============================================================================
// EntityManager.cpp - Entity lifecycle management implementation
// Location: src/core/ECS/EntityManager.cpp
// ============================================================================

#include "core/ECS/EntityManager.h"
#include <algorithm>

namespace core::ecs {

    // ============================================================================
    // Component Template Static Members
    // ============================================================================

    template<typename T>
    ComponentTypeID Component<T>::s_type_id = 0;

    template<typename T>
    ComponentTypeID Component<T>::s_next_id = 1;

    template<typename T>
    ComponentTypeID Component<T>::GetStaticTypeID() {
        if (s_type_id == 0) {
            s_type_id = s_next_id++;
        }
        return s_type_id;
    }

    template<typename T>
    ComponentTypeID Component<T>::GetTypeID() const {
        return GetStaticTypeID();
    }

    template<typename T>
    std::unique_ptr<IComponent> Component<T>::Clone() const {
        return std::make_unique<T>(static_cast<const T&>(*this));
    }

    // ============================================================================
    // TypedComponentPool Implementation
    // ============================================================================

    template<typename T>
    bool TypedComponentPool<T>::HasComponent(EntityID entity) const {
        return m_components.find(entity) != m_components.end();
    }

    template<typename T>
    bool TypedComponentPool<T>::RemoveComponent(EntityID entity) {
        auto it = m_components.find(entity);
        if (it != m_components.end()) {
            m_components.erase(it);
            return true;
        }
        return false;
    }

    template<typename T>
    void TypedComponentPool<T>::Clear() {
        m_components.clear();
    }

    template<typename T>
    size_t TypedComponentPool<T>::Size() const {
        return m_components.size();
    }

    template<typename T>
    std::vector<EntityID> TypedComponentPool<T>::GetEntities() const {
        std::vector<EntityID> entities;
        entities.reserve(m_components.size());

        for (const auto& pair : m_components) {
            entities.push_back(pair.first);
        }

        return entities;
    }

    template<typename T>
    template<typename... Args>
    T* TypedComponentPool<T>::AddComponent(EntityID entity, Args&&... args) {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* component_ptr = component.get();
        m_components[entity] = std::move(component);
        return component_ptr;
    }

    template<typename T>
    T* TypedComponentPool<T>::GetComponent(EntityID entity) {
        auto it = m_components.find(entity);
        return (it != m_components.end()) ? it->second.get() : nullptr;
    }

    template<typename T>
    const T* TypedComponentPool<T>::GetComponent(EntityID entity) const {
        auto it = m_components.find(entity);
        return (it != m_components.end()) ? it->second.get() : nullptr;
    }

    // ============================================================================
    // EntityManager Implementation
    // ============================================================================

    EntityID EntityManager::CreateEntity() {
        EntityID entity = m_next_entity_id++;
        m_entities.insert(entity);
        return entity;
    }

    void EntityManager::DestroyEntity(EntityID entity) {
        if (!IsValidEntity(entity)) {
            return;
        }

        // Remove all components for this entity
        for (auto& [type_index, pool] : m_component_pools) {
            pool->RemoveComponent(entity);
        }

        // Remove entity from set
        m_entities.erase(entity);
    }

    bool EntityManager::IsValidEntity(EntityID entity) const {
        return m_entities.find(entity) != m_entities.end();
    }

    size_t EntityManager::GetEntityCount() const {
        return m_entities.size();
    }

    size_t EntityManager::GetComponentPoolCount() const {
        return m_component_pools.size();
    }

    void EntityManager::Clear() {
        for (auto& [type_index, pool] : m_component_pools) {
            pool->Clear();
        }
        m_entities.clear();
        m_next_entity_id = 1;
    }

    // ============================================================================
    // Template Method Implementations (in EntityManager.inl)
    // ============================================================================

} // namespace core::ecs