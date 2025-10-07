// ============================================================================
// ComponentAccessManager.inl - FIXED Template Implementation
// CRITICAL BUG FIX: Eliminated data race in vector operations
// Location: src/core/ecs/ComponentAccessManager.inl
// ============================================================================

#pragma once

namespace core::ecs {

    // ============================================================================
    // ComponentAccessResult Implementation
    // ============================================================================

    template<typename ComponentType>
    ComponentAccessResult<ComponentType>::ComponentAccessResult(ComponentType* component, std::shared_lock<std::shared_mutex>&& lock)
        : m_component(component), m_lock(std::move(lock)) {
    }

    template<typename ComponentType>
    ComponentAccessResult<ComponentType>::ComponentAccessResult(ComponentAccessResult&& other) noexcept
        : m_component(other.m_component), m_lock(std::move(other.m_lock)) {
        other.m_component = nullptr;
    }

    template<typename ComponentType>
    ComponentAccessResult<ComponentType>& ComponentAccessResult<ComponentType>::operator=(ComponentAccessResult&& other) noexcept {
        if (this != &other) {
            m_component = other.m_component;
            m_lock = std::move(other.m_lock);
            other.m_component = nullptr;
        }
        return *this;
    }

    template<typename ComponentType>
    bool ComponentAccessResult<ComponentType>::IsValid() const {
        return m_component != nullptr && m_lock.owns_lock();
    }

    template<typename ComponentType>
    const ComponentType* ComponentAccessResult<ComponentType>::Get() const {
        return m_component;
    }

    template<typename ComponentType>
    const ComponentType* ComponentAccessResult<ComponentType>::operator->() const {
        return m_component;
    }

    template<typename ComponentType>
    const ComponentType& ComponentAccessResult<ComponentType>::operator*() const {
        return *m_component;
    }

    template<typename ComponentType>
    ComponentAccessResult<ComponentType>::operator bool() const {
        return IsValid();
    }

    // ============================================================================
    // ComponentWriteGuard Implementation
    // ============================================================================

    template<typename ComponentType>
    ComponentWriteGuard<ComponentType>::ComponentWriteGuard(ComponentType* component, std::unique_lock<std::shared_mutex>&& lock)
        : m_component(component), m_lock(std::move(lock)) {
    }

    template<typename ComponentType>
    ComponentWriteGuard<ComponentType>::ComponentWriteGuard(ComponentWriteGuard&& other) noexcept
        : m_component(other.m_component), m_lock(std::move(other.m_lock)) {
        other.m_component = nullptr;
    }

    template<typename ComponentType>
    ComponentWriteGuard<ComponentType>& ComponentWriteGuard<ComponentType>::operator=(ComponentWriteGuard&& other) noexcept {
        if (this != &other) {
            m_component = other.m_component;
            m_lock = std::move(other.m_lock);
            other.m_component = nullptr;
        }
        return *this;
    }

    template<typename ComponentType>
    bool ComponentWriteGuard<ComponentType>::IsValid() const {
        return m_component != nullptr && m_lock.owns_lock();
    }

    template<typename ComponentType>
    ComponentType* ComponentWriteGuard<ComponentType>::Get() {
        return m_component;
    }

    template<typename ComponentType>
    ComponentType* ComponentWriteGuard<ComponentType>::operator->() {
        return m_component;
    }

    template<typename ComponentType>
    ComponentType& ComponentWriteGuard<ComponentType>::operator*() {
        return *m_component;
    }

    template<typename ComponentType>
    ComponentWriteGuard<ComponentType>::operator bool() const {
        return IsValid();
    }

    // ============================================================================
    // VectorAccessResult Implementation - SAFE VERSION
    // ============================================================================

    template<typename ComponentType>
    VectorAccessResult<ComponentType>::VectorAccessResult(std::shared_lock<std::shared_mutex>&& lock)
        : m_lock(std::move(lock)) {
    }

    template<typename ComponentType>
    VectorAccessResult<ComponentType>::VectorAccessResult(VectorAccessResult&& other) noexcept
        : m_lock(std::move(other.m_lock)) {
    }

    template<typename ComponentType>
    VectorAccessResult<ComponentType>& VectorAccessResult<ComponentType>::operator=(VectorAccessResult&& other) noexcept {
        if (this != &other) {
            m_lock = std::move(other.m_lock);
        }
        return *this;
    }

    template<typename ComponentType>
    const ComponentType* VectorAccessResult<ComponentType>::GetComponent(types::EntityID entity_id, EntityManager& entity_manager) const {
        return entity_manager.GetComponent<ComponentType>(entity_id);
    }

    template<typename ComponentType>
    ComponentType* VectorAccessResult<ComponentType>::GetComponentMutable(types::EntityID entity_id, EntityManager& entity_manager) {
        return entity_manager.GetComponent<ComponentType>(entity_id);
    }

    // ConstIterator implementation
    template<typename ComponentType>
    VectorAccessResult<ComponentType>::ConstIterator::ConstIterator(EntityManager& em, const std::vector<types::EntityID>& entities, size_t index)
        : m_entity_manager(em), m_entities(entities), m_index(index) {
    }

    template<typename ComponentType>
    const ComponentType* VectorAccessResult<ComponentType>::ConstIterator::operator*() const {
        if (m_index >= m_entities.size()) return nullptr;
        return m_entity_manager.GetComponent<ComponentType>(m_entities[m_index]);
    }

    template<typename ComponentType>
    const ComponentType* VectorAccessResult<ComponentType>::ConstIterator::operator->() const {
        if (m_index >= m_entities.size()) return nullptr;
        return m_entity_manager.GetComponent<ComponentType>(m_entities[m_index]);
    }

    template<typename ComponentType>
    typename VectorAccessResult<ComponentType>::ConstIterator& VectorAccessResult<ComponentType>::ConstIterator::operator++() {
        ++m_index;
        return *this;
    }

    template<typename ComponentType>
    bool VectorAccessResult<ComponentType>::ConstIterator::operator!=(const ConstIterator& other) const {
        return m_index != other.m_index;
    }

    template<typename ComponentType>
    bool VectorAccessResult<ComponentType>::ConstIterator::operator==(const ConstIterator& other) const {
        return m_index == other.m_index;
    }

    template<typename ComponentType>
    typename VectorAccessResult<ComponentType>::ConstIterator VectorAccessResult<ComponentType>::begin(EntityManager& entity_manager, const std::vector<types::EntityID>& entities) const {
        return ConstIterator(entity_manager, entities, 0);
    }

    template<typename ComponentType>
    typename VectorAccessResult<ComponentType>::ConstIterator VectorAccessResult<ComponentType>::end(EntityManager& entity_manager, const std::vector<types::EntityID>& entities) const {
        return ConstIterator(entity_manager, entities, entities.size());
    }

    // ============================================================================
    // VectorWriteResult Implementation - SAFE VERSION  
    // ============================================================================

    template<typename ComponentType>
    VectorWriteResult<ComponentType>::VectorWriteResult(std::unique_lock<std::shared_mutex>&& lock)
        : m_lock(std::move(lock)) {
    }

    template<typename ComponentType>
    VectorWriteResult<ComponentType>::VectorWriteResult(VectorWriteResult&& other) noexcept
        : m_lock(std::move(other.m_lock)) {
    }

    template<typename ComponentType>
    VectorWriteResult<ComponentType>& VectorWriteResult<ComponentType>::operator=(VectorWriteResult&& other) noexcept {
        if (this != &other) {
            m_lock = std::move(other.m_lock);
        }
        return *this;
    }

    template<typename ComponentType>
    const ComponentType* VectorWriteResult<ComponentType>::GetComponent(types::EntityID entity_id, EntityManager& entity_manager) const {
        return entity_manager.GetComponent<ComponentType>(entity_id);
    }

    template<typename ComponentType>
    ComponentType* VectorWriteResult<ComponentType>::GetComponentMutable(types::EntityID entity_id, EntityManager& entity_manager) {
        return entity_manager.GetComponent<ComponentType>(entity_id);
    }

    // Iterator implementation
    template<typename ComponentType>
    VectorWriteResult<ComponentType>::Iterator::Iterator(EntityManager& em, const std::vector<types::EntityID>& entities, size_t index)
        : m_entity_manager(em), m_entities(entities), m_index(index) {
    }

    template<typename ComponentType>
    ComponentType* VectorWriteResult<ComponentType>::Iterator::operator*() {
        if (m_index >= m_entities.size()) return nullptr;
        return m_entity_manager.GetComponent<ComponentType>(m_entities[m_index]);
    }

    template<typename ComponentType>
    ComponentType* VectorWriteResult<ComponentType>::Iterator::operator->() {
        if (m_index >= m_entities.size()) return nullptr;
        return m_entity_manager.GetComponent<ComponentType>(m_entities[m_index]);
    }

    template<typename ComponentType>
    typename VectorWriteResult<ComponentType>::Iterator& VectorWriteResult<ComponentType>::Iterator::operator++() {
        ++m_index;
        return *this;
    }

    template<typename ComponentType>
    bool VectorWriteResult<ComponentType>::Iterator::operator!=(const Iterator& other) const {
        return m_index != other.m_index;
    }

    template<typename ComponentType>
    bool VectorWriteResult<ComponentType>::Iterator::operator==(const Iterator& other) const {
        return m_index == other.m_index;
    }

    template<typename ComponentType>
    typename VectorWriteResult<ComponentType>::Iterator VectorWriteResult<ComponentType>::begin(EntityManager& entity_manager, const std::vector<types::EntityID>& entities) {
        return Iterator(entity_manager, entities, 0);
    }

    template<typename ComponentType>
    typename VectorWriteResult<ComponentType>::Iterator VectorWriteResult<ComponentType>::end(EntityManager& entity_manager, const std::vector<types::EntityID>& entities) {
        return Iterator(entity_manager, entities, entities.size());
    }

    // ============================================================================
    // ComponentAccessManager Template Implementation - FIXED
    // ============================================================================

    template<typename ComponentType>
    ComponentAccessResult<ComponentType> ComponentAccessManager::GetComponent(types::EntityID entity_id) {
        std::string type_name = typeid(ComponentType).name();
        RegisterComponentType(type_name);

        auto& mutex = GetComponentMutex<ComponentType>();
        std::shared_lock<std::shared_mutex> lock(mutex);

        ComponentType* component = m_entity_manager->GetComponent<ComponentType>(entity_id);

        if (m_performance_monitoring_enabled.load()) {
            RecordAccess(type_name, false);
        }

        return ComponentAccessResult<ComponentType>(component, std::move(lock));
    }

    template<typename ComponentType>
    ComponentWriteGuard<ComponentType> ComponentAccessManager::GetComponentForWrite(types::EntityID entity_id) {
        std::string type_name = typeid(ComponentType).name();
        RegisterComponentType(type_name);

        auto& mutex = GetComponentMutex<ComponentType>();
        std::unique_lock<std::shared_mutex> lock(mutex);

        ComponentType* component = m_entity_manager->GetComponent<ComponentType>(entity_id);

        if (m_performance_monitoring_enabled.load()) {
            RecordAccess(type_name, true);
        }

        return ComponentWriteGuard<ComponentType>(component, std::move(lock));
    }

    // ============================================================================
    // FIXED: Safe vector operations - NO MORE DATA RACES
    // ============================================================================

    template<typename ComponentType>
    VectorAccessResult<ComponentType> ComponentAccessManager::GetAllComponentsForRead() {
        std::string type_name = typeid(ComponentType).name();
        RegisterComponentType(type_name);

        auto& mutex = GetComponentMutex<ComponentType>();
        std::shared_lock<std::shared_mutex> lock(mutex);

        if (m_performance_monitoring_enabled.load()) {
            RecordAccess(type_name, false);
        }

        // Return a SINGLE lock that protects ALL access
        return VectorAccessResult<ComponentType>(std::move(lock));
    }

    template<typename ComponentType>
    VectorWriteResult<ComponentType> ComponentAccessManager::GetAllComponentsForWrite() {
        std::string type_name = typeid(ComponentType).name();
        RegisterComponentType(type_name);

        auto& mutex = GetComponentMutex<ComponentType>();
        std::unique_lock<std::shared_mutex> lock(mutex);

        if (m_performance_monitoring_enabled.load()) {
            RecordAccess(type_name, true);
        }

        // Return a SINGLE lock that protects ALL access
        return VectorWriteResult<ComponentType>(std::move(lock));
    }

    template<typename ComponentType>
    VectorAccessResult<ComponentType> ComponentAccessManager::GetComponentsBatchForRead(const std::vector<types::EntityID>& entity_ids) {
        std::string type_name = typeid(ComponentType).name();
        RegisterComponentType(type_name);

        auto& mutex = GetComponentMutex<ComponentType>();
        std::shared_lock<std::shared_mutex> lock(mutex);

        if (m_performance_monitoring_enabled.load()) {
            RecordAccess(type_name, false);
        }

        // Return a SINGLE lock that protects access to the specified entities
        return VectorAccessResult<ComponentType>(std::move(lock));
    }

    template<typename ComponentType>
    VectorWriteResult<ComponentType> ComponentAccessManager::GetComponentsBatchForWrite(const std::vector<types::EntityID>& entity_ids) {
        std::string type_name = typeid(ComponentType).name();
        RegisterComponentType(type_name);

        auto& mutex = GetComponentMutex<ComponentType>();
        std::unique_lock<std::shared_mutex> lock(mutex);

        if (m_performance_monitoring_enabled.load()) {
            RecordAccess(type_name, true);
        }

        // Return a SINGLE lock that protects access to the specified entities
        return VectorWriteResult<ComponentType>(std::move(lock));
    }

    template<typename ComponentType>
    std::shared_mutex& ComponentAccessManager::GetComponentMutex() {
        std::string type_name = typeid(ComponentType).name();
        RegisterComponentType(type_name);

        std::shared_lock<std::shared_mutex> lock(m_mutex_map_mutex);
        auto it = m_component_mutexes.find(type_name);
        if (it != m_component_mutexes.end()) {
            return *it->second;
        }

        // Need to upgrade to write lock
        lock.unlock();
        std::unique_lock<std::shared_mutex> write_lock(m_mutex_map_mutex);

        // Double-check pattern
        auto it2 = m_component_mutexes.find(type_name);
        if (it2 != m_component_mutexes.end()) {
            return *it2->second;
        }

        // Create new mutex
        auto new_mutex = std::make_unique<std::shared_mutex>();
        auto& mutex_ref = *new_mutex;
        m_component_mutexes[type_name] = std::move(new_mutex);

        return mutex_ref;
    }

} // namespace core::ecs