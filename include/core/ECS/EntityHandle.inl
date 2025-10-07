// ============================================================================
// EntityHandle.inl - Template Implementation for Version Safety
// CRITICAL MEMORY SAFETY: All operations validate entity versions
// Location: src/core/ecs/EntityHandle.inl
// ============================================================================

#pragma once

#include "EntityManager.h"
#include <stdexcept>
#include <typeinfo>

namespace core::ecs {

    // ============================================================================
    // EntityHandle Template Implementation
    // ============================================================================

    template<typename ComponentType>
    ComponentType* EntityHandle::GetComponent() const {
        if (!ValidateAccess()) {
            RecordInvalidAccess();
            return nullptr;
        }

        auto* safe_manager = dynamic_cast<SafeEntityManager*>(m_manager);
        if (!safe_manager) {
            // Fallback to unsafe access if not using SafeEntityManager
            return m_manager->GetComponent<ComponentType>(m_entity_id);
        }

        return safe_manager->GetComponent<ComponentType>(m_entity_id);
    }

    template<typename ComponentType>
    ComponentType* EntityHandle::AddComponent() const {
        if (!ValidateAccess()) {
            ThrowIfInvalid();
            return nullptr;
        }

        auto* safe_manager = dynamic_cast<SafeEntityManager*>(m_manager);
        if (!safe_manager) {
            return m_manager->AddComponent<ComponentType>(m_entity_id);
        }

        return safe_manager->AddComponent<ComponentType>(m_entity_id);
    }

    template<typename ComponentType>
    bool EntityHandle::RemoveComponent() const {
        if (!ValidateAccess()) {
            return false;
        }

        auto* safe_manager = dynamic_cast<SafeEntityManager*>(m_manager);
        if (!safe_manager) {
            return m_manager->RemoveComponent<ComponentType>(m_entity_id);
        }

        return safe_manager->RemoveComponent<ComponentType>(m_entity_id);
    }

    template<typename ComponentType>
    bool EntityHandle::HasComponent() const {
        if (!ValidateAccess()) {
            return false;
        }

        auto* safe_manager = dynamic_cast<SafeEntityManager*>(m_manager);
        if (!safe_manager) {
            return m_manager->HasComponent<ComponentType>(m_entity_id);
        }

        return safe_manager->HasComponent<ComponentType>(m_entity_id);
    }

    // ============================================================================
    // VersionedEntityManager Template Implementation
    // ============================================================================

    template<typename ComponentType>
    std::vector<EntityHandle> VersionedEntityManager::GetEntitiesWithComponentSafe() const {
        std::string component_type = typeid(ComponentType).name();
        return GetEntitiesWithComponentSafe(component_type);
    }

    // ============================================================================
    // SafeEntityManager Template Implementation
    // ============================================================================

    template<typename ComponentType>
    std::vector<EntityHandle> SafeEntityManager::GetEntitiesWithComponentSafe() const {
        std::vector<EntityHandle> handles;

        std::shared_lock<std::shared_mutex> lock(m_entities_mutex);

        for (types::EntityID id = 1; id < m_entity_records.size(); ++id) {
            if (m_entity_records[id].exists && HasComponent<ComponentType>(id)) {
                handles.emplace_back(id, m_entity_records[id].version, const_cast<SafeEntityManager*>(this));
            }
        }

        return handles;
    }

    template<typename ComponentType>
    ComponentType* SafeEntityManager::GetComponent(types::EntityID id) const {
        if (!IsValidEntityID(id) || !EntityExists(id)) {
            RecordInvalidAccess();
            return nullptr;
        }

        EnsureComponentArrayExists<ComponentType>();
        return GetComponentUnsafe<ComponentType>(id);
    }

    template<typename ComponentType>
    ComponentType* SafeEntityManager::AddComponent(types::EntityID id) {
        if (!IsValidEntityID(id) || !EntityExists(id)) {
            RecordInvalidAccess();
            return nullptr;
        }

        std::unique_lock<std::shared_mutex> lock(m_components_mutex);

        EnsureComponentArrayExists<ComponentType>();

        std::string type_name = typeid(ComponentType).name();

        // Ensure entity capacity
        if (id >= m_component_masks[type_name].size()) {
            m_component_masks[type_name].resize(id + 1, false);
        }

        // Check if component already exists
        if (m_component_masks[type_name][id]) {
            return GetComponentUnsafe<ComponentType>(id);
        }

        // Allocate component storage if needed
        if (!m_component_arrays[type_name]) {
            m_component_arrays[type_name] = std::make_unique<void* []>(id + 1);
            for (size_t i = 0; i <= id; ++i) {
                static_cast<void**>(m_component_arrays[type_name].get())[i] = nullptr;
            }
        }

        // Create new component
        auto* component = new ComponentType();
        static_cast<void**>(m_component_arrays[type_name].get())[id] = component;
        m_component_masks[type_name][id] = true;

        return component;
    }

    template<typename ComponentType>
    bool SafeEntityManager::RemoveComponent(types::EntityID id) {
        if (!IsValidEntityID(id) || !EntityExists(id)) {
            RecordInvalidAccess();
            return false;
        }

        std::unique_lock<std::shared_mutex> lock(m_components_mutex);

        std::string type_name = typeid(ComponentType).name();

        auto mask_it = m_component_masks.find(type_name);
        if (mask_it == m_component_masks.end() || id >= mask_it->second.size() || !mask_it->second[id]) {
            return false; // Component doesn't exist
        }

        // Delete component
        auto array_it = m_component_arrays.find(type_name);
        if (array_it != m_component_arrays.end() && array_it->second) {
            void** array = static_cast<void**>(array_it->second.get());
            delete static_cast<ComponentType*>(array[id]);
            array[id] = nullptr;
        }

        mask_it->second[id] = false;
        return true;
    }

    template<typename ComponentType>
    bool SafeEntityManager::HasComponent(types::EntityID id) const {
        if (!IsValidEntityID(id) || !EntityExists(id)) {
            return false;
        }

        std::shared_lock<std::shared_mutex> lock(m_components_mutex);

        std::string type_name = typeid(ComponentType).name();

        auto it = m_component_masks.find(type_name);
        return it != m_component_masks.end() && id < it->second.size() && it->second[id];
    }

    template<typename ComponentType>
    std::vector<types::EntityID> SafeEntityManager::GetEntitiesWithComponent() const {
        std::vector<types::EntityID> entities;

        std::shared_lock<std::shared_mutex> entities_lock(m_entities_mutex);
        std::shared_lock<std::shared_mutex> components_lock(m_components_mutex);

        std::string type_name = typeid(ComponentType).name();

        auto it = m_component_masks.find(type_name);
        if (it == m_component_masks.end()) {
            return entities;
        }

        for (types::EntityID id = 1; id < std::min(m_entity_records.size(), it->second.size()); ++id) {
            if (m_entity_records[id].exists && it->second[id]) {
                entities.push_back(id);
            }
        }

        return entities;
    }

    template<typename ComponentType>
    void SafeEntityManager::EnsureComponentArrayExists() const {
        std::string type_name = typeid(ComponentType).name();

        if (m_component_masks.find(type_name) == m_component_masks.end()) {
            m_component_masks[type_name] = std::vector<bool>();
        }

        if (m_component_arrays.find(type_name) == m_component_arrays.end()) {
            m_component_arrays[type_name] = nullptr;
        }
    }

    template<typename ComponentType>
    ComponentType* SafeEntityManager::GetComponentUnsafe(types::EntityID id) const {
        std::string type_name = typeid(ComponentType).name();

        auto mask_it = m_component_masks.find(type_name);
        if (mask_it == m_component_masks.end() || id >= mask_it->second.size() || !mask_it->second[id]) {
            return nullptr;
        }

        auto array_it = m_component_arrays.find(type_name);
        if (array_it == m_component_arrays.end() || !array_it->second) {
            return nullptr;
        }

        void** array = static_cast<void**>(array_it->second.get());
        return static_cast<ComponentType*>(array[id]);
    }

} // namespace core::ecs