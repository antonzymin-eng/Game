// ============================================================================
// ComponentAccessManager.inl - Template Implementation (FIXED)
// Created: October 10, 2025, 6:00 PM
// Location: include/core/ECS/ComponentAccessManager.inl
// FIXED: Proper EntityID conversion and shared_ptr handling
// ============================================================================

// NOTE: This file is included at the end of ComponentAccessManager.h
// All necessary types and headers are already available

namespace core::ecs {

    // ============================================================================
    // ComponentAccessResult Implementation
    // ============================================================================

    template<typename ComponentType>
    inline ComponentAccessResult<ComponentType>::ComponentAccessResult(
        ComponentType* component, 
        std::shared_lock<std::shared_mutex>&& lock)
        : m_component(component)
        , m_lock(std::move(lock)) 
    {
    }

    template<typename ComponentType>
    inline ComponentAccessResult<ComponentType>::ComponentAccessResult(
        ComponentAccessResult&& other) noexcept
        : m_component(other.m_component)
        , m_lock(std::move(other.m_lock)) 
    {
        other.m_component = nullptr;
    }

    template<typename ComponentType>
    inline ComponentAccessResult<ComponentType>& ComponentAccessResult<ComponentType>::operator=(
        ComponentAccessResult&& other) noexcept 
    {
        if (this != &other) {
            m_component = other.m_component;
            m_lock = std::move(other.m_lock);
            other.m_component = nullptr;
        }
        return *this;
    }

    template<typename ComponentType>
    inline bool ComponentAccessResult<ComponentType>::IsValid() const {
        return m_component != nullptr && m_lock.owns_lock();
    }

    template<typename ComponentType>
    inline const ComponentType* ComponentAccessResult<ComponentType>::Get() const {
        return m_component;
    }

    template<typename ComponentType>
    inline const ComponentType* ComponentAccessResult<ComponentType>::operator->() const {
        #ifndef NDEBUG
            assert(m_component && "Dereferencing invalid ComponentAccessResult - component is null");
        #else
            if (!m_component) {
                throw std::runtime_error("Dereferencing invalid ComponentAccessResult - component is null");
            }
        #endif
        return m_component;
    }

    template<typename ComponentType>
    inline const ComponentType& ComponentAccessResult<ComponentType>::operator*() const {
        #ifndef NDEBUG
            assert(m_component && "Dereferencing invalid ComponentAccessResult - component is null");
        #else
            if (!m_component) {
                throw std::runtime_error("Dereferencing invalid ComponentAccessResult - component is null");
            }
        #endif
        return *m_component;
    }

    template<typename ComponentType>
    inline ComponentAccessResult<ComponentType>::operator bool() const {
        return IsValid();
    }

    // ============================================================================
    // ComponentWriteGuard Implementation
    // ============================================================================

    template<typename ComponentType>
    inline ComponentWriteGuard<ComponentType>::ComponentWriteGuard(
        ComponentType* component, 
        std::unique_lock<std::shared_mutex>&& lock)
        : m_component(component)
        , m_lock(std::move(lock)) 
    {
    }

    template<typename ComponentType>
    inline ComponentWriteGuard<ComponentType>::ComponentWriteGuard(
        ComponentWriteGuard&& other) noexcept
        : m_component(other.m_component)
        , m_lock(std::move(other.m_lock)) 
    {
        other.m_component = nullptr;
    }

    template<typename ComponentType>
    inline ComponentWriteGuard<ComponentType>& ComponentWriteGuard<ComponentType>::operator=(
        ComponentWriteGuard&& other) noexcept 
    {
        if (this != &other) {
            m_component = other.m_component;
            m_lock = std::move(other.m_lock);
            other.m_component = nullptr;
        }
        return *this;
    }

    template<typename ComponentType>
    inline bool ComponentWriteGuard<ComponentType>::IsValid() const {
        return m_component != nullptr && m_lock.owns_lock();
    }

    template<typename ComponentType>
    inline ComponentType* ComponentWriteGuard<ComponentType>::Get() {
        return m_component;
    }

    template<typename ComponentType>
    inline ComponentType* ComponentWriteGuard<ComponentType>::operator->() {
        #ifndef NDEBUG
            assert(m_component && "Dereferencing invalid ComponentWriteGuard - component is null");
        #else
            if (!m_component) {
                throw std::runtime_error("Dereferencing invalid ComponentWriteGuard - component is null");
            }
        #endif
        return m_component;
    }

    template<typename ComponentType>
    inline ComponentType& ComponentWriteGuard<ComponentType>::operator*() {
        #ifndef NDEBUG
            assert(m_component && "Dereferencing invalid ComponentWriteGuard - component is null");
        #else
            if (!m_component) {
                throw std::runtime_error("Dereferencing invalid ComponentWriteGuard - component is null");
            }
        #endif
        return *m_component;
    }

    template<typename ComponentType>
    inline ComponentWriteGuard<ComponentType>::operator bool() const {
        return IsValid();
    }

    // ============================================================================
    // VectorAccessResult Implementation - FIXED
    // ============================================================================

    template<typename ComponentType>
    inline VectorAccessResult<ComponentType>::VectorAccessResult(
        std::shared_lock<std::shared_mutex>&& lock)
        : m_lock(std::move(lock)) 
    {
    }

    template<typename ComponentType>
    inline VectorAccessResult<ComponentType>::VectorAccessResult(
        VectorAccessResult&& other) noexcept
        : m_lock(std::move(other.m_lock)) 
    {
    }

    template<typename ComponentType>
    inline VectorAccessResult<ComponentType>& VectorAccessResult<ComponentType>::operator=(
        VectorAccessResult&& other) noexcept 
    {
        if (this != &other) {
            m_lock = std::move(other.m_lock);
        }
        return *this;
    }

    template<typename ComponentType>
    inline const ComponentType* VectorAccessResult<ComponentType>::GetComponent(
        game::types::EntityID entity_id, 
        EntityManager& entity_manager) const 
    {
        // FIXED: Convert game::types::EntityID to core::ecs::EntityID
        auto shared = entity_manager.GetComponent<ComponentType>(
            ::core::ecs::EntityID(entity_id));
        return shared ? shared.get() : nullptr;
    }

    template<typename ComponentType>
    inline ComponentType* VectorAccessResult<ComponentType>::GetComponentMutable(
        game::types::EntityID entity_id, 
        EntityManager& entity_manager) 
    {
        // FIXED: Convert game::types::EntityID to core::ecs::EntityID
        auto shared = entity_manager.GetComponent<ComponentType>(
            ::core::ecs::EntityID(entity_id));
        return shared ? shared.get() : nullptr;
    }

    // ConstIterator implementation - FIXED
    template<typename ComponentType>
    inline VectorAccessResult<ComponentType>::ConstIterator::ConstIterator(
        EntityManager& em, 
        const std::vector<game::types::EntityID>& entities, 
        size_t index)
        : m_entity_manager(em)
        , m_entities(entities)
        , m_index(index) 
    {
    }

    template<typename ComponentType>
    inline const ComponentType* VectorAccessResult<ComponentType>::ConstIterator::operator*() const {
        if (m_index >= m_entities.size()) return nullptr;
        
        // FIXED: Convert EntityID and extract raw pointer from shared_ptr
        auto shared = m_entity_manager.GetComponent<ComponentType>(
            ::core::ecs::EntityID(m_entities[m_index]));
        return shared ? shared.get() : nullptr;
    }

    template<typename ComponentType>
    inline const ComponentType* VectorAccessResult<ComponentType>::ConstIterator::operator->() const {
        if (m_index >= m_entities.size()) return nullptr;
        
        // FIXED: Convert EntityID and extract raw pointer from shared_ptr
        auto shared = m_entity_manager.GetComponent<ComponentType>(
            ::core::ecs::EntityID(m_entities[m_index]));
        return shared ? shared.get() : nullptr;
    }

    template<typename ComponentType>
    inline typename VectorAccessResult<ComponentType>::ConstIterator& 
    VectorAccessResult<ComponentType>::ConstIterator::operator++() {
        ++m_index;
        return *this;
    }

    template<typename ComponentType>
    inline bool VectorAccessResult<ComponentType>::ConstIterator::operator!=(
        const ConstIterator& other) const 
    {
        return m_index != other.m_index;
    }

    template<typename ComponentType>
    inline bool VectorAccessResult<ComponentType>::ConstIterator::operator==(
        const ConstIterator& other) const 
    {
        return m_index == other.m_index;
    }

    template<typename ComponentType>
    inline typename VectorAccessResult<ComponentType>::ConstIterator 
    VectorAccessResult<ComponentType>::begin(
        EntityManager& entity_manager, 
        const std::vector<game::types::EntityID>& entities) const 
    {
        return ConstIterator(entity_manager, entities, 0);
    }

    template<typename ComponentType>
    inline typename VectorAccessResult<ComponentType>::ConstIterator 
    VectorAccessResult<ComponentType>::end(
        EntityManager& entity_manager, 
        const std::vector<game::types::EntityID>& entities) const 
    {
        return ConstIterator(entity_manager, entities, entities.size());
    }

    // ============================================================================
    // VectorWriteResult Implementation - FIXED
    // ============================================================================

    template<typename ComponentType>
    inline VectorWriteResult<ComponentType>::VectorWriteResult(
        std::unique_lock<std::shared_mutex>&& lock)
        : m_lock(std::move(lock)) 
    {
    }

    template<typename ComponentType>
    inline VectorWriteResult<ComponentType>::VectorWriteResult(
        VectorWriteResult&& other) noexcept
        : m_lock(std::move(other.m_lock)) 
    {
    }

    template<typename ComponentType>
    inline VectorWriteResult<ComponentType>& VectorWriteResult<ComponentType>::operator=(
        VectorWriteResult&& other) noexcept 
    {
        if (this != &other) {
            m_lock = std::move(other.m_lock);
        }
        return *this;
    }

    template<typename ComponentType>
    inline const ComponentType* VectorWriteResult<ComponentType>::GetComponent(
        game::types::EntityID entity_id, 
        EntityManager& entity_manager) const 
    {
        // FIXED: Convert game::types::EntityID to core::ecs::EntityID
        auto shared = entity_manager.GetComponent<ComponentType>(
            ::core::ecs::EntityID(entity_id));
        return shared ? shared.get() : nullptr;
    }

    template<typename ComponentType>
    inline ComponentType* VectorWriteResult<ComponentType>::GetComponentMutable(
        game::types::EntityID entity_id, 
        EntityManager& entity_manager) 
    {
        // FIXED: Convert game::types::EntityID to core::ecs::EntityID
        auto shared = entity_manager.GetComponent<ComponentType>(
            ::core::ecs::EntityID(entity_id));
        return shared ? shared.get() : nullptr;
    }

    // Iterator implementation - FIXED
    template<typename ComponentType>
    inline VectorWriteResult<ComponentType>::Iterator::Iterator(
        EntityManager& em, 
        const std::vector<game::types::EntityID>& entities, 
        size_t index)
        : m_entity_manager(em)
        , m_entities(entities)
        , m_index(index) 
    {
    }

    template<typename ComponentType>
    inline ComponentType* VectorWriteResult<ComponentType>::Iterator::operator*() {
        if (m_index >= m_entities.size()) return nullptr;
        
        // FIXED: Convert EntityID and extract raw pointer from shared_ptr
        auto shared = m_entity_manager.GetComponent<ComponentType>(
            ::core::ecs::EntityID(m_entities[m_index]));
        return shared ? shared.get() : nullptr;
    }

    template<typename ComponentType>
    inline ComponentType* VectorWriteResult<ComponentType>::Iterator::operator->() {
        if (m_index >= m_entities.size()) return nullptr;
        
        // FIXED: Convert EntityID and extract raw pointer from shared_ptr
        auto shared = m_entity_manager.GetComponent<ComponentType>(
            ::core::ecs::EntityID(m_entities[m_index]));
        return shared ? shared.get() : nullptr;
    }

    template<typename ComponentType>
    inline typename VectorWriteResult<ComponentType>::Iterator& 
    VectorWriteResult<ComponentType>::Iterator::operator++() {
        ++m_index;
        return *this;
    }

    template<typename ComponentType>
    inline bool VectorWriteResult<ComponentType>::Iterator::operator!=(
        const Iterator& other) const 
    {
        return m_index != other.m_index;
    }

    template<typename ComponentType>
    inline bool VectorWriteResult<ComponentType>::Iterator::operator==(
        const Iterator& other) const 
    {
        return m_index == other.m_index;
    }

    template<typename ComponentType>
    inline typename VectorWriteResult<ComponentType>::Iterator 
    VectorWriteResult<ComponentType>::begin(
        EntityManager& entity_manager, 
        const std::vector<game::types::EntityID>& entities) 
    {
        return Iterator(entity_manager, entities, 0);
    }

    template<typename ComponentType>
    inline typename VectorWriteResult<ComponentType>::Iterator 
    VectorWriteResult<ComponentType>::end(
        EntityManager& entity_manager, 
        const std::vector<game::types::EntityID>& entities) 
    {
        return Iterator(entity_manager, entities, entities.size());
    }

    // ============================================================================
    // ComponentAccessManager Template Methods - FIXED
    // ============================================================================

    template<typename ComponentType>
    inline ComponentAccessResult<ComponentType> ComponentAccessManager::GetComponent(
        game::types::EntityID entity_id) 
    {
        std::string type_name = typeid(ComponentType).name();
        RegisterComponentType(type_name);

        auto& mutex = GetComponentMutex<ComponentType>();
        std::shared_lock<std::shared_mutex> lock(mutex);

        // FIXED: Convert EntityID and extract raw pointer
        auto shared = m_entity_manager->GetComponent<ComponentType>(
            ::core::ecs::EntityID(entity_id));
        ComponentType* component = shared ? shared.get() : nullptr;

        if (m_performance_monitoring_enabled.load(std::memory_order_acquire)) {
            RecordAccess(type_name, false);
        }

        return ComponentAccessResult<ComponentType>(component, std::move(lock));
    }

    template<typename ComponentType>
    inline ComponentWriteGuard<ComponentType> ComponentAccessManager::GetComponentForWrite(
        game::types::EntityID entity_id) 
    {
        std::string type_name = typeid(ComponentType).name();
        RegisterComponentType(type_name);

        auto& mutex = GetComponentMutex<ComponentType>();
        std::unique_lock<std::shared_mutex> lock(mutex);

        // FIXED: Convert EntityID and extract raw pointer
        auto shared = m_entity_manager->GetComponent<ComponentType>(
            ::core::ecs::EntityID(entity_id));
        ComponentType* component = shared ? shared.get() : nullptr;

        if (m_performance_monitoring_enabled.load(std::memory_order_acquire)) {
            RecordAccess(type_name, true);
        }

        return ComponentWriteGuard<ComponentType>(component, std::move(lock));
    }

    template<typename ComponentType>
    inline VectorAccessResult<ComponentType> ComponentAccessManager::GetAllComponentsForRead() {
        std::string type_name = typeid(ComponentType).name();
        RegisterComponentType(type_name);

        auto& mutex = GetComponentMutex<ComponentType>();
        std::shared_lock<std::shared_mutex> lock(mutex);

        if (m_performance_monitoring_enabled.load(std::memory_order_acquire)) {
            RecordAccess(type_name, false);
        }

        return VectorAccessResult<ComponentType>(std::move(lock));
    }

    template<typename ComponentType>
    inline VectorWriteResult<ComponentType> ComponentAccessManager::GetAllComponentsForWrite() {
        std::string type_name = typeid(ComponentType).name();
        RegisterComponentType(type_name);

        auto& mutex = GetComponentMutex<ComponentType>();
        std::unique_lock<std::shared_mutex> lock(mutex);

        if (m_performance_monitoring_enabled.load(std::memory_order_acquire)) {
            RecordAccess(type_name, true);
        }

        return VectorWriteResult<ComponentType>(std::move(lock));
    }

    template<typename ComponentType>
    inline VectorAccessResult<ComponentType> ComponentAccessManager::GetComponentsBatchForRead(
        const std::vector<game::types::EntityID>& entity_ids) 
    {
        std::string type_name = typeid(ComponentType).name();
        RegisterComponentType(type_name);

        auto& mutex = GetComponentMutex<ComponentType>();
        std::shared_lock<std::shared_mutex> lock(mutex);

        if (m_performance_monitoring_enabled.load(std::memory_order_acquire)) {
            RecordAccess(type_name, false);
        }

        return VectorAccessResult<ComponentType>(std::move(lock));
    }

    template<typename ComponentType>
    inline VectorWriteResult<ComponentType> ComponentAccessManager::GetComponentsBatchForWrite(
        const std::vector<game::types::EntityID>& entity_ids) 
    {
        std::string type_name = typeid(ComponentType).name();
        RegisterComponentType(type_name);

        auto& mutex = GetComponentMutex<ComponentType>();
        std::unique_lock<std::shared_mutex> lock(mutex);

        if (m_performance_monitoring_enabled.load(std::memory_order_acquire)) {
            RecordAccess(type_name, true);
        }

        return VectorWriteResult<ComponentType>(std::move(lock));
    }

    template<typename ComponentType>
    inline std::shared_mutex& ComponentAccessManager::GetComponentMutex() {
        std::string type_name = typeid(ComponentType).name();
        
        // First try with read lock
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex_map_mutex);
            auto it = m_component_mutexes.find(type_name);
            if (it != m_component_mutexes.end()) {
                return *it->second;
            }
        }

        // Need to create new mutex - upgrade to write lock
        std::unique_lock<std::shared_mutex> write_lock(m_mutex_map_mutex);

        // Double-check pattern - might have been created while upgrading lock
        auto it = m_component_mutexes.find(type_name);
        if (it != m_component_mutexes.end()) {
            return *it->second;
        }

        // Create new mutex
        auto new_mutex = std::make_unique<std::shared_mutex>();
        auto& mutex_ref = *new_mutex;
        m_component_mutexes[type_name] = std::move(new_mutex);

        return mutex_ref;
    }

} // namespace core::ecs
