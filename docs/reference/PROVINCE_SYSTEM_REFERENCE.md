# Province Management System - Complete Reference

**Created:** January 2025  
**Status:** Consolidated from 5 source documents  
**Purpose:** Single authoritative reference for Province Management System

---

## Table of Contents

1. [Overview & Quick Facts](#overview--quick-facts)
2. [Architecture](#architecture)
3. [API Reference](#api-reference)
4. [Implementation Details](#implementation-details)
5. [Development Guide](#development-guide)

---

## Overview & Quick Facts

### Executive Summary

The Province Management System (PMS) is a player-facing UI layer that handles decision-making and order processing for province administration. It's designed as an ECS-compatible system that bridges player input with the underlying province simulation systems.

**Current Status:** 40% Complete - Core classes functional, integration incomplete

### File Locations

```
├── include/game/province/
│   └── ProvinceManagementSystem.h (430 lines)
├── src/game/province/
│   ├── ProvinceManagementSystem.cpp (882 lines)
│   └── ProvinceManagementUtils.cpp (438 lines)
```

**Total Lines of Code:** 1,750 lines

### Key Metrics

| Metric | Value | Status |
|--------|-------|--------|
| **Total Files** | 3 | ✅ Complete |
| **Total Classes** | 6 | ⚠️ Mixed (see below) |
| **Public Methods** | 62+ | ⚠️ Many TODOs |
| **Utility Functions** | 26 | ✅ 95% complete |
| **Enumerations** | 6 | ✅ Complete |
| **Data Structures** | 5 | ✅ Complete |

### Class Implementation Status

| Component | Implementation | Notes |
|-----------|----------------|-------|
| **PlayerDecision** | ✅ 100% | Fully functional |
| **DecisionQueue** | ✅ 100% | Priority-based queue working |
| **ProvinceOrderSystem** | ✅ 100% | Order lifecycle complete |
| **ProvinceManagementSystem** | ⚠️ 30% | Main coordinator - many TODOs |
| **ManagementComponent** | ❌ 0% | ECS component not implemented |
| **PlayerPolicyComponent** | ❌ 0% | ECS component not implemented |

### Critical Dependencies

**Implemented:**
- ✅ ECS Framework (EntityManager, ComponentAccessManager, MessageBus)
- ✅ Logging System
- ✅ Threading Strategy (MAIN_THREAD)

**Missing:**
- ❌ ProvinceSystem interface (HIGH PRIORITY - blocks functionality)
- ❌ Component integration (ManagementComponent, PlayerPolicyComponent)
- ❌ Message system subscriptions
- ❌ Integration with Economic, Population, Military systems

### Estimated Effort to Completion

| Task Category | Hours | Priority |
|---------------|-------|----------|
| Implement missing features | 50-70 | P1 |
| Fix design issues | 20-30 | P1 |
| Add testing | 15-20 | P2 |
| Optimization | 10-15 | P3 |
| **TOTAL** | **95-135** | - |

---

## Architecture

### System Hierarchy

```
ProvinceManagementSystem (implements ISystem)
├── DecisionQueue (decision management)
│   ├── PlayerDecision[] (pending decisions)
│   ├── PlayerDecision[] (completed decisions)
│   └── Priority-based queues
├── ProvinceOrderSystem (order management)
│   ├── ProvinceOrder[] (active orders)
│   └── ProvinceOrder[] (completed orders)
└── Game Integration
    ├── ECS Framework
    │   ├── ComponentAccessManager
    │   ├── MessageBus
    │   └── EntityManager
    ├── Province System (MISSING - not implemented)
    └── Logging System
```

### Design Patterns Used

#### 1. Strategy Pattern
**Location:** Decision generation strategies
```cpp
using DecisionGeneratorFunc = std::function<PlayerDecision(const Province&)>;
std::unordered_map<DecisionType, DecisionGeneratorFunc> decision_generators_;
```
**Purpose:** Flexible decision generation per type

#### 2. Factory Pattern
**Location:** Decision and order creation
```cpp
PlayerDecision CreateTaxDecision(const Province& province);
ProvinceOrder CreateConstructionOrder(EntityID province_id, Building building);
```

#### 3. Observer Pattern
**Location:** MessageBus integration (partially implemented)
- Events: ProvinceCreated, EconomicCrisis, ResourceShortage
- Subscriptions: Not yet implemented

#### 4. State Pattern
**Location:** Decision and order status management
```cpp
enum class DecisionStatus { PENDING, APPROVED, REJECTED, DELEGATED, EXECUTED, FAILED };
enum class OrderStatus { QUEUED, IN_PROGRESS, COMPLETED, FAILED, CANCELLED };
```

### Data Flow

```
Player Input (UI)
    ↓
GenerateDecision(province, type)
    ↓
DecisionQueue::AddDecision()
    ↓
[Priority sorting: CRITICAL > URGENT > IMPORTANT > ROUTINE]
    ↓
ProcessDecision(decision_id, option_id)
    ↓
ExecuteDecisionStrategy()
    ↓
IssueOrder() or DirectAction()
    ↓
ProvinceOrderSystem::AddOrder()
    ↓
UpdateOrders(delta_time) [progress tracking]
    ↓
CompleteOrder() → Apply effects to Province
    ↓
Publish events to MessageBus
```

### Integration Points

**Intended Connections:**
- **EconomicSystem** - Tax rates, budget allocation, trade policies
- **PopulationSystem** - Migration policy, social services
- **MilitarySystem** - Recruitment orders, garrison assignment
- **TechnologySystem** - Research funding, scholar patronage
- **AdministrativeSystem** - Official appointments, bureaucracy reform

**Current Status:** None implemented (awaiting ProvinceSystem interface)

---

## API Reference

### ProvinceManagementSystem

#### Lifecycle Methods (ISystem Interface)

```cpp
void Initialize();
```
**Purpose:** Setup system, register decision generators, initialize strategies  
**Status:** ⚠️ Partial - TODO: Subscribe to message events  
**Threading:** MAIN_THREAD

```cpp
void Update(float delta_time);
```
**Purpose:** Process decision queues and update orders  
**Status:** ✅ Implemented  
**Processing:**
1. Update decision queue (check escalations)
2. Update order system (progress tracking)
3. Check for completed orders

```cpp
void Shutdown();
```
**Purpose:** Cleanup resources  
**Status:** ✅ Implemented

```cpp
ThreadingStrategy GetThreadingStrategy() const;
```
**Returns:** `ThreadingStrategy::MAIN_THREAD`  
**Status:** ✅ Implemented

#### Decision Management

```cpp
DecisionQueue* GetDecisionQueue();
```
**Returns:** Pointer to decision queue  
**Status:** ✅ Implemented  
**Usage:** Access for UI display, manual decision processing

```cpp
bool GenerateDecision(types::EntityID province_id, ManagementDecisionType type);
```
**Purpose:** Create new decision for province  
**Parameters:**
- `province_id` - Province entity ID
- `type` - Decision type (TAX_RATE_ADJUSTMENT, BUILDING_CONSTRUCTION, etc.)

**Returns:** `true` if decision generated successfully  
**Status:** ⚠️ Partial - TODO: Fetch actual province data  
**Note:** Currently uses placeholder data

```cpp
bool ProcessDecision(const std::string& decision_id, const std::string& option_id);
```
**Purpose:** Execute player's decision choice  
**Parameters:**
- `decision_id` - Unique decision identifier
- `option_id` - Selected option identifier

**Returns:** `true` if decision processed successfully  
**Status:** ⚠️ Partial - TODO: Integrate with ProvinceSystem  
**Note:** Currently logs but doesn't apply effects

#### Order Management

```cpp
ProvinceOrderSystem* GetOrderSystem();
```
**Returns:** Pointer to order system  
**Status:** ✅ Implemented

```cpp
std::string IssueConstructionOrder(types::EntityID province_id, 
                                   ProductionBuilding building);
```
**Purpose:** Create construction order  
**Parameters:**
- `province_id` - Target province
- `building` - Building type (FARM, MARKET, SMITHY)

**Returns:** Order ID string  
**Status:** ⚠️ Partial - TODO: Validate province, check resources  
**Duration:** 30 days (hardcoded)

```cpp
std::string IssuePolicyOrder(types::EntityID province_id, 
                             const std::string& policy_name, 
                             double value);
```
**Purpose:** Create policy change order  
**Parameters:**
- `province_id` - Target province
- `policy_name` - Policy identifier ("tax_rate", "trade_policy", etc.)
- `value` - New policy value

**Returns:** Order ID string  
**Status:** ⚠️ Partial - TODO: Validate policy and value  
**Duration:** 7 days (hardcoded)

#### Province Management (TODO)

```cpp
bool CreateManagedProvince(types::EntityID province_id, 
                          const std::string& manager_name);
```
**Status:** ❌ Not implemented - returns false  
**TODO:** Create ManagementComponent for province

```cpp
bool DestroyManagedProvince(types::EntityID province_id);
```
**Status:** ❌ Not implemented - returns false  
**TODO:** Remove ManagementComponent from province

```cpp
bool SetProvinceAutomation(types::EntityID province_id, AutomationLevel level);
```
**Status:** ❌ Not implemented - returns false  
**TODO:** Set automation level in ManagementComponent

### DecisionQueue

```cpp
class DecisionQueue {
public:
    void AddDecision(const PlayerDecision& decision);
    bool RemoveDecision(const std::string& decision_id);
    bool MarkDecisionProcessed(const std::string& decision_id, DecisionStatus status);
    
    std::vector<PlayerDecision> GetPendingDecisions() const;
    std::vector<PlayerDecision> GetDecisionsByPriority(DecisionPriority priority) const;
    std::vector<PlayerDecision> GetCompletedDecisions() const;
    
    PlayerDecision* GetDecision(const std::string& decision_id);
    size_t GetPendingCount() const;
    void ClearCompletedDecisions();
    
    void Update(float delta_time);  // Check for escalations
};
```

**Features:**
- Priority-based sorting (CRITICAL → URGENT → IMPORTANT → ROUTINE)
- Automatic priority escalation for overdue decisions ⚠️ (TODO: Bug fix needed)
- Completed decision history
- Fast O(log n) priority queue operations

### ProvinceOrderSystem

```cpp
class ProvinceOrderSystem {
public:
    std::string AddOrder(const ProvinceOrder& order);
    bool CancelOrder(const std::string& order_id);
    bool CompleteOrder(const std::string& order_id);
    bool FailOrder(const std::string& order_id, const std::string& reason);
    
    std::vector<ProvinceOrder> GetActiveOrders() const;
    std::vector<ProvinceOrder> GetOrdersForProvince(types::EntityID province_id) const;
    std::vector<ProvinceOrder> GetCompletedOrders() const;
    
    ProvinceOrder* GetOrder(const std::string& order_id);
    void Update(float delta_time);  // Update progress
};
```

**Features:**
- Order lifecycle management (QUEUED → IN_PROGRESS → COMPLETED/FAILED)
- Progress tracking (0.0 → 1.0)
- Province-specific order queries
- Automatic status transitions based on time

### PlayerDecision

```cpp
class PlayerDecision {
public:
    // Getters
    std::string GetId() const;
    ManagementDecisionType GetType() const;
    types::EntityID GetProvinceId() const;
    DecisionPriority GetPriority() const;
    DecisionStatus GetStatus() const;
    std::string GetTitle() const;
    std::string GetDescription() const;
    std::vector<DecisionOption> GetOptions() const;
    int64_t GetCreatedTime() const;
    int64_t GetDeadline() const;
    
    // Setters
    void SetPriority(DecisionPriority priority);
    void SetStatus(DecisionStatus status);
    void SetProcessedTime(int64_t time);
    void SetSelectedOption(const std::string& option_id);
    
    // Queries
    bool IsOverdue(int64_t current_time) const;
    int64_t GetTimeRemaining(int64_t current_time) const;
    const DecisionOption* GetSelectedOption() const;
};
```

### ProvinceOrder

```cpp
class ProvinceOrder {
public:
    // Getters
    std::string GetId() const;
    OrderType GetType() const;
    OrderStatus GetStatus() const;
    types::EntityID GetProvinceId() const;
    std::string GetDescription() const;
    float GetProgress() const;
    int64_t GetStartTime() const;
    int64_t GetEstimatedCompletion() const;
    int GetDuration() const;
    
    // Progress
    void UpdateProgress(float delta);
    void SetStatus(OrderStatus status);
    void SetFailureReason(const std::string& reason);
    std::string GetFailureReason() const;
    
    // Queries
    bool IsComplete() const;
    bool IsFailed() const;
    int64_t GetTimeRemaining(int64_t current_time) const;
};
```

---

## Implementation Details

### Enumerations

#### ManagementDecisionType (13 types)
```cpp
enum class ManagementDecisionType : uint8_t {
    TAX_RATE_ADJUSTMENT = 0,      // Adjust taxation level
    BUDGET_ALLOCATION = 1,         // Allocate budget to sectors
    TRADE_POLICY_CHANGE = 2,       // Modify trade regulations
    BUILDING_CONSTRUCTION = 3,     // Build infrastructure
    INFRASTRUCTURE_DEVELOPMENT = 4,// Improve roads, ports
    MIGRATION_POLICY = 5,          // Immigration/emigration rules
    SOCIAL_SERVICES = 6,           // Healthcare, education
    RESEARCH_FUNDING = 7,          // Technology research budget
    SCHOLAR_PATRONAGE = 8,         // Support scholars
    OFFICIAL_APPOINTMENT = 9,      // Appoint administrators
    BUREAUCRACY_REFORM = 10,       // Administrative improvements
    RECRUITMENT_ORDER = 11,        // Military recruitment
    GARRISON_ASSIGNMENT = 12,      // Assign garrison units
    COUNT = 13,
    INVALID = 255
};
```

#### DecisionPriority (4 levels)
```cpp
enum class DecisionPriority : uint8_t {
    ROUTINE = 0,    // Can wait, no urgency
    IMPORTANT = 1,  // Should address soon
    URGENT = 2,     // Needs attention quickly
    CRITICAL = 3,   // Immediate action required
    COUNT = 4
};
```

#### DecisionStatus (6 states)
```cpp
enum class DecisionStatus : uint8_t {
    PENDING = 0,    // Awaiting player input
    APPROVED = 1,   // Player selected option
    REJECTED = 2,   // Player rejected decision
    DELEGATED = 3,  // Delegated to AI/official
    EXECUTED = 4,   // Successfully applied
    FAILED = 5,     // Failed to execute
    COUNT = 6
};
```

#### AutomationLevel (4 levels)
```cpp
enum class AutomationLevel : uint8_t {
    MANUAL = 0,     // Never automate, always ask player
    ASSISTED = 1,   // Show AI recommendations
    GUIDED = 2,     // Automate ROUTINE, ask for others
    AUTOMATED = 3,  // Automate all except CRITICAL
    COUNT = 4
};
```

#### OrderType (4 types)
```cpp
enum class OrderType : uint8_t {
    CONSTRUCTION_ORDER = 0,   // Building construction
    POLICY_CHANGE = 1,        // Policy modification
    RESOURCE_ALLOCATION = 2,  // Resource distribution
    RESEARCH_ORDER = 3,       // Research directive
    COUNT = 4
};
```

#### OrderStatus (5 states)
```cpp
enum class OrderStatus : uint8_t {
    QUEUED = 0,        // Waiting to start
    IN_PROGRESS = 1,   // Currently executing
    COMPLETED = 2,     // Successfully finished
    FAILED = 3,        // Execution failed
    CANCELLED = 4,     // Manually cancelled
    COUNT = 5
};
```

### Data Structures

#### DecisionOption
```cpp
struct DecisionOption {
    std::string option_id;          // Unique identifier
    std::string description;        // What this option does
    std::string tooltip;            // Detailed explanation
    double cost = 0.0;             // Economic cost
    std::unordered_map<std::string, double> effects;  // Predicted effects
    std::unordered_map<std::string, std::string> requirements;  // Prerequisites
    bool is_available = true;       // Can player select this?
};
```

**Example:**
```cpp
DecisionOption option;
option.option_id = "tax_increase_10";
option.description = "Increase tax rate by 10%";
option.tooltip = "Higher revenue but lower happiness";
option.cost = 0.0;
option.effects["tax_income"] = 1.1;
option.effects["happiness"] = -0.05;
option.requirements["authority"] = "50";
option.is_available = true;
```

### Utility Functions (26 total)

#### String Conversions
```cpp
std::string DecisionTypeToString(ManagementDecisionType type);
std::string DecisionPriorityToString(DecisionPriority priority);
std::string DecisionStatusToString(DecisionStatus status);
std::string AutomationLevelToString(AutomationLevel level);
std::string OrderTypeToString(OrderType type);
std::string OrderStatusToString(OrderStatus status);

ManagementDecisionType StringToDecisionType(const std::string& str);
DecisionPriority StringToDecisionPriority(const std::string& str);
// ... etc for all enums
```

#### Decision Generation Helpers
```cpp
PlayerDecision CreateTaxDecision(const Province& province);
PlayerDecision CreateBudgetDecision(const Province& province);
PlayerDecision CreateTradePolicyDecision(const Province& province);
PlayerDecision CreateBuildingDecision(const Province& province);
// ... 9 more decision generators
```

#### Analysis Functions
```cpp
DecisionPriority AnalyzeDecisionUrgency(ManagementDecisionType type, 
                                       const Province& province);
double EstimateDecisionImpact(const PlayerDecision& decision, 
                              const DecisionOption& option);
std::vector<DecisionOption> GenerateDecisionOptions(ManagementDecisionType type,
                                                     const Province& province);
```

---

## Development Guide

### Critical Issues (9 total)

#### Issue #1: ProvinceSystem Not Implemented (HIGH)
**Problem:** Core dependency missing - can't fetch/modify province data  
**Impact:** All province-related operations are stubs  
**Location:** Throughout ProvinceManagementSystem.cpp  
**Solution:** Implement ProvinceSystem interface with methods:
```cpp
class ProvinceSystem {
    Province* GetProvince(types::EntityID id);
    bool UpdateProvinceTaxRate(types::EntityID id, double rate);
    bool ConstructBuilding(types::EntityID id, Building type);
    // ... etc
};
```
**Estimated Effort:** 30-40 hours

#### Issue #2: 29 TODO Items Blocking Functionality (HIGH)
**Problem:** Many methods are placeholder stubs  
**Locations:**
- `GenerateDecision()` - Uses placeholder province data
- `ProcessDecision()` - Doesn't apply effects
- `IssueConstructionOrder()` - No validation
- `CreateManagedProvince()` - Not implemented
- All decision generators - Use fake data

**Solution:** Implement each TODO with proper ProvinceSystem integration  
**Estimated Effort:** 20-30 hours

#### Issue #3: Component Integration Incomplete (HIGH)
**Problem:** ManagementComponent and PlayerPolicyComponent not created  
**Impact:** Can't persist management state in ECS  
**Solution:** 
1. Define components in header
2. Add component creation in CreateManagedProvince()
3. Use components to store automation level, policies, etc.

**Estimated Effort:** 8-12 hours

#### Issue #4: No Thread Safety (MEDIUM)
**Problem:** Concurrent access to DecisionQueue and OrderSystem not protected  
**Impact:** Potential race conditions if called from multiple threads  
**Solution:** Add `std::mutex` to both classes, guard all public methods  
**Estimated Effort:** 4-6 hours

#### Issue #5: Null Pointer Return Risks (MEDIUM)
**Problem:** Methods return raw pointers that can be nullptr  
**Locations:**
- `GetDecisionQueue()` returns raw pointer
- `GetOrderSystem()` returns raw pointer
- `GetDecision()` returns nullable pointer

**Solution:** 
- Use `std::optional<>` or `std::shared_ptr<>` for safer returns
- Add null checks before dereferencing
- Document nullable returns clearly

**Estimated Effort:** 2-3 hours

#### Issue #6: Memory Management Issues (MEDIUM)
**Problem:** ProvinceOrder stores heap-allocated data without cleanup  
**Impact:** Potential memory leaks  
**Solution:** Use RAII patterns, smart pointers, or proper destructors  
**Estimated Effort:** 3-4 hours

#### Issue #7: Priority Escalation Bug (MEDIUM)
**Problem:** Escalation logic doesn't handle already-CRITICAL decisions  
**Location:** `DecisionQueue::Update()`  
**Current Code:**
```cpp
if (decision.IsOverdue(current_time)) {
    decision.SetPriority(static_cast<DecisionPriority>(
        static_cast<int>(decision.GetPriority()) + 1
    ));
}
```
**Issue:** Can increment beyond CRITICAL (value 3) to undefined behavior  
**Solution:**
```cpp
if (decision.IsOverdue(current_time)) {
    auto current = static_cast<int>(decision.GetPriority());
    if (current < static_cast<int>(DecisionPriority::CRITICAL)) {
        decision.SetPriority(static_cast<DecisionPriority>(current + 1));
    }
}
```
**Estimated Effort:** 30 minutes

#### Issue #8: Performance - O(n) Lookups (LOW)
**Problem:** Linear search for decisions/orders by ID  
**Impact:** Slow with many pending items  
**Solution:** Add `std::unordered_map<std::string, size_t>` for O(1) lookup  
**Estimated Effort:** 2-3 hours

#### Issue #9: Duplicate Code - String Functions (LOW)
**Problem:** Similar enum-to-string conversion code repeated 6 times  
**Solution:** Use template metaprogramming or macro to reduce duplication  
**Estimated Effort:** 2-3 hours

### Development Priorities

#### Priority 1 - CRITICAL (Do First)
1. **Implement ProvinceSystem interface** (30-40 hours)
   - Define core Province class
   - Implement get/update methods
   - Integrate with EntityManager
   
2. **Add thread safety** (4-6 hours)
   - Add mutexes to DecisionQueue and OrderSystem
   - Guard all public methods
   - Test concurrent access

3. **Complete component integration** (8-12 hours)
   - Define ManagementComponent
   - Define PlayerPolicyComponent
   - Implement CreateManagedProvince()
   - Add component serialization

4. **Remove all TODO stubs** (20-30 hours)
   - Implement decision generation with real data
   - Implement ProcessDecision() effects
   - Add validation to order creation
   - Complete all placeholder methods

**Subtotal: 62-88 hours**

#### Priority 2 - IMPORTANT (Do Soon)
1. **Implement message subscriptions** (4-6 hours)
   - Subscribe to ProvinceCreated events
   - Subscribe to EconomicCrisis events
   - Subscribe to ResourceShortage events
   - Trigger automatic decision generation

2. **Fix priority escalation bug** (30 minutes)
   - Add bounds checking
   - Test edge cases

3. **Add input validation** (6-8 hours)
   - Validate province IDs
   - Check resource availability
   - Verify policy ranges
   - Add error messages

4. **Consolidate duplicate code** (2-3 hours)
   - Template string conversion functions
   - Extract common patterns

5. **Optimize data structures** (2-3 hours)
   - Add hash maps for fast lookup
   - Profile performance
   - Optimize hot paths

**Subtotal: 15-21 hours**

#### Priority 3 - NICE TO HAVE (Do Later)
1. **Add serialization support** (8-10 hours)
   - Serialize DecisionQueue state
   - Serialize OrderSystem state
   - Save/load management component data

2. **Improve error handling** (4-6 hours)
   - Add error codes
   - Improve error messages
   - Add recovery mechanisms

3. **Performance profiling** (4-6 hours)
   - Profile Update() calls
   - Identify bottlenecks
   - Optimize if needed

4. **Machine learning recommendations** (10-15 hours)
   - Track player decision patterns
   - Generate AI recommendations
   - Improve automation

5. **Complete testing coverage** (8-10 hours)
   - Unit tests for all classes
   - Integration tests
   - Edge case validation

**Subtotal: 34-47 hours**

### Integration Checklist

Before declaring system complete, verify:

- [ ] ProvinceSystem interface defined and implemented
- [ ] All 29 TODO items resolved
- [ ] ManagementComponent and PlayerPolicyComponent created
- [ ] Thread safety added (mutexes in place)
- [ ] Message subscriptions implemented
- [ ] Integration with EconomicSystem working
- [ ] Integration with PopulationSystem working
- [ ] Integration with MilitarySystem working
- [ ] Integration with TechnologySystem working
- [ ] Integration with AdministrativeSystem working
- [ ] All decision generators use real province data
- [ ] ProcessDecision() applies effects to provinces
- [ ] Order execution modifies province state
- [ ] Serialization saves/loads complete state
- [ ] Unit tests pass
- [ ] Integration tests pass
- [ ] Performance meets requirements (< 1ms per Update())
- [ ] Memory leaks checked and fixed
- [ ] Documentation updated

### Testing Strategy

#### Unit Tests
```cpp
TEST(DecisionQueueTest, PriorityOrdering) {
    // Test that CRITICAL decisions come before ROUTINE
}

TEST(DecisionQueueTest, EscalationLogic) {
    // Test overdue decisions escalate priority
}

TEST(ProvinceOrderSystemTest, OrderLifecycle) {
    // Test QUEUED → IN_PROGRESS → COMPLETED flow
}

TEST(ProvinceOrderSystemTest, ProgressTracking) {
    // Test progress updates correctly
}

TEST(PlayerDecisionTest, OptionSelection) {
    // Test selecting decision options
}
```

#### Integration Tests
```cpp
TEST(ProvinceManagementIntegration, TaxDecisionFlow) {
    // 1. Generate tax decision
    // 2. Process with option
    // 3. Verify province tax rate changed
}

TEST(ProvinceManagementIntegration, ConstructionFlow) {
    // 1. Issue construction order
    // 2. Update until complete
    // 3. Verify building exists in province
}

TEST(ProvinceManagementIntegration, AutomationFlow) {
    // 1. Set automation level
    // 2. Generate ROUTINE decision
    // 3. Verify auto-processed
}
```

### Code Quality Guidelines

**When modifying this system:**
1. Remove TODOs as you implement features
2. Add unit tests for new functionality
3. Update this documentation
4. Follow existing naming conventions
5. Use smart pointers for heap allocations
6. Add thread safety if accessing from multiple threads
7. Validate all inputs
8. Add proper error handling
9. Document public APIs
10. Profile performance impacts

---

## Revision History

| Date | Change | Author |
|------|--------|--------|
| Oct 30, 2025 | Initial analysis and documentation | AI Assistant |
| Jan 2025 | Consolidated from 5 source documents | AI Assistant |

---

## Source Documents Consolidated

This reference was created by merging content from:
1. PROVINCE_MANAGEMENT_ANALYSIS.md (833 lines)
2. PROVINCE_MANAGEMENT_CODE_STRUCTURE.md (886 lines)
3. PROVINCE_MANAGEMENT_QUICK_REFERENCE.md (290 lines)
4. PROVINCE_MANAGEMENT_ANALYSIS_SUMMARY.txt (summary)
5. PROVINCE_MANAGEMENT_INDEX.txt (navigation guide)

All original files archived to: `archive/2025-01-docs-cleanup/superseded/`

---

**For Questions:** Refer to source code in `include/game/province/` and `src/game/province/`  
**For Updates:** Modify this file and update "Last Updated" date  
**For Implementation:** Follow Development Guide priorities
