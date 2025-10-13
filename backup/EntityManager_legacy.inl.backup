// ============================================================================
// EntityManager.inl - Template method implementations
// Created: October 10, 2025, 3:15 PM
// Location: include/core/ECS/EntityManager.inl
// FIXED: All signatures match EntityManager.h
// ============================================================================

#pragma once

#include <memory>
#include <vector>
#include <typeinfo>
#include <cassert>
#include <stdexcept>
#include <string>
#include <shared_mutex>

namespace core::ecs {

    // ============================================================================
    // Component Access Methods - FIXED SIGNATURES
    // ============================================================================

    template<typename ComponentType, typename... Args>
    std::shared_ptr<ComponentType> EntityManager::AddComponent(const EntityID& handle, Args&&... args) {
        // FIXED: Validate entity handle before adding components
        if (!ValidateEntityHandle(handle)) {
            throw std::invalid_argument("Invalid entity handle: " + handle.ToString());
        }

        size_t type_hash = typeid(ComponentType).hash_code();

        // Get or create storage
        ComponentStorage<ComponentType>* storage = nullptr;
        {
            std::unique_lock lock(m_storages_mutex);
            auto it = m_component_storages.find(type_hash);
            if (it == m_component_storages.end()) {
                m_component_storages[type_hash] = std::make_unique<ComponentStorage<ComponentType>>();
                storage = static_cast<ComponentStorage<ComponentType>*>(m_component_storages[type_hash].get());
            }
            else {
                storage = static_cast<ComponentStorage<ComponentType>*>(it->second.get());
            }
        }

        // Add component
        auto component = storage->AddComponent(handle.id, std::forward<Args>(args)...);

        // Update entity info
        {
            auto* entity_info = GetMutableEntityInfo(handle);
            if (entity_info) {
                entity_info->component_types.insert(type_hash);
                entity_info->UpdateLastModified();
            }
        }

        m_statistics_dirty = true;
        return component;
    }

    template<typename ComponentType>
    std::shared_ptr<ComponentType> EntityManager::GetComponent(const EntityID& handle) const {
        // FIXED: Validate entity handle before accessing components
        if (!ValidateEntityHandle(handle)) {
            return nullptr;
        }

        size_t type_hash = typeid(ComponentType).hash_code();

        std::shared_lock lock(m_storages_mutex);
        auto it = m_component_storages.find(type_hash);
        if (it != m_component_storages.end()) {
            auto storage = static_cast<ComponentStorage<ComponentType>*>(it->second.get());
            return storage->GetComponent(handle.id);
        }

        return nullptr;
    }

    template<typename ComponentType>
    bool EntityManager::HasComponent(const EntityID& handle) const {
        // FIXED: Validate entity handle before checking components
        if (!ValidateEntityHandle(handle)) {
            return false;
        }

        size_t type_hash = typeid(ComponentType).hash_code();

        std::shared_lock lock(m_storages_mutex);
        auto it = m_component_storages.find(type_hash);
        if (it != m_component_storages.end()) {
            return it->second->HasComponent(handle.id);
        }

        return false;
    }

    template<typename ComponentType>
    bool EntityManager::RemoveComponent(const EntityID& handle) {
        // FIXED: Validate entity handle before removing components
        if (!ValidateEntityHandle(handle)) {
            return false;
        }

        size_t type_hash = typeid(ComponentType).hash_code();

        bool removed = false;
        {
            std::shared_lock lock(m_storages_mutex);
            auto it = m_component_storages.find(type_hash);
            if (it != m_component_storages.end()) {
                removed = it->second->RemoveComponent(handle.id);
            }
        }

        if (removed) {
            // Update entity info
            auto* entity_info = GetMutableEntityInfo(handle);
            if (entity_info) {
                entity_info->component_types.erase(type_hash);
                entity_info->UpdateLastModified();
            }

            m_statistics_dirty = true;
        }

        return removed;
    }

    // ============================================================================
    // Bulk Query Methods - FIXED SIGNATURES
    // ============================================================================

    template<typename ComponentType>
    std::vector<EntityID> EntityManager::GetEntitiesWithComponent() const {
        std::vector<EntityID> result;

        size_t type_hash = typeid(ComponentType).hash_code();

        std::shared_lock storages_lock(m_storages_mutex);
        auto storage_it = m_component_storages.find(type_hash);
        if (storage_it == m_component_storages.end()) {
            return result;
        }

        auto entity_ids = storage_it->second->GetEntityIds();

        std::shared_lock entities_lock(m_entities_mutex);
        result.reserve(entity_ids.size());

        for (uint64_t entity_id : entity_ids) {
            auto it = m_entities.find(entity_id);
            if (it != m_entities.end() && it->second.active) {
                result.emplace_back(entity_id, it->second.version);
            }
        }

        return result;
    }

    template<typename ComponentType>
    void EntityManager::DestroyEntitiesWithComponent() {
        auto entities = GetEntitiesWithComponent<ComponentType>();
        for (const auto& handle : entities) {
            DestroyEntity(handle);
        }
    }

} // namespace core::ecs
