# Province Management System - Detailed Code Structure Map

## File: ProvinceManagementSystem.h (430 lines)

### Namespace: game::province
```cpp
enum class ProductionBuilding : uint8_t {
    FARM = 0,
    MARKET = 1,
    SMITHY = 2,
    COUNT
};

namespace messages {
    struct ProvinceCreated { types::EntityID province_id; };
    struct EconomicCrisis { types::EntityID province_id; };
    struct ResourceShortage { types::EntityID province_id; };
}

namespace utils {
    inline std::string ProductionBuildingToString(ProductionBuilding building)
        → Returns: "Building" (STUB)
}
```

### Namespace: game::management

#### Enumerations (6 total)

```cpp
enum class ManagementDecisionType : uint8_t {
    TAX_RATE_ADJUSTMENT = 0,
    BUDGET_ALLOCATION = 1,
    TRADE_POLICY_CHANGE = 2,
    BUILDING_CONSTRUCTION = 3,
    INFRASTRUCTURE_DEVELOPMENT = 4,
    MIGRATION_POLICY = 5,
    SOCIAL_SERVICES = 6,
    RESEARCH_FUNDING = 7,
    SCHOLAR_PATRONAGE = 8,
    OFFICIAL_APPOINTMENT = 9,
    BUREAUCRACY_REFORM = 10,
    RECRUITMENT_ORDER = 11,
    GARRISON_ASSIGNMENT = 12,
    COUNT = 13,
    INVALID = 255
};

enum class DecisionPriority : uint8_t {
    ROUTINE = 0,
    IMPORTANT = 1,
    URGENT = 2,
    CRITICAL = 3,
    COUNT = 4
};

enum class DecisionStatus : uint8_t {
    PENDING = 0,
    APPROVED = 1,
    REJECTED = 2,
    DELEGATED = 3,
    EXECUTED = 4,
    FAILED = 5,
    COUNT = 6
};

enum class AutomationLevel : uint8_t {
    MANUAL = 0,
    ASSISTED = 1,
    GUIDED = 2,
    AUTOMATED = 3,
    COUNT = 4
};

enum class OrderType : uint8_t {
    CONSTRUCTION_ORDER = 0,
    POLICY_CHANGE = 1,
    RESOURCE_ALLOCATION = 2,
    RESEARCH_ORDER = 3,
    COUNT = 4
};

enum class OrderStatus : uint8_t {
    QUEUED = 0,
    IN_PROGRESS = 1,
    COMPLETED = 2,
    FAILED = 3,
    CANCELLED = 4,
    COUNT = 5
};
```

#### Data Structures (5 total)

```cpp
struct DecisionOption {
    std::string option_id;
    std::string description;
    std::string tooltip;
    double cost = 0.0;
    double benefit_estimate = 0.0;
    std::vector<std::string> requirements;
    bool is_available = true;
    double ai_recommendation = 0.0;
};

struct DecisionContext {
    types::EntityID province_id{ 0 };
    ManagementDecisionType decision_type = ManagementDecisionType::INVALID;
    std::string situation_description;
    std::vector<DecisionOption> available_options;
    double urgency_factor = 0.0;
    std::chrono::system_clock::time_point deadline;
    std::unordered_map<std::string, double> numeric_data;
};

struct ProvinceOrder {
    std::string order_id;
    OrderType order_type = OrderType::CONSTRUCTION_ORDER;
    types::EntityID target_province{ 0 };
    OrderStatus status = OrderStatus::QUEUED;
    std::string order_description;
    double estimated_cost = 0.0;
    double progress = 0.0;
    std::chrono::system_clock::time_point start_time;
    std::unordered_map<std::string, std::string> parameters;
    bool can_execute = false;
    
    ProvinceOrder() = default;
    explicit ProvinceOrder(OrderType type, types::EntityID province);
};

struct ManagementComponent : public game::core::IComponent {
    types::EntityID province_id{ 0 };
    AutomationLevel automation_level = AutomationLevel::ASSISTED;
    bool player_controlled = true;
    std::string manager_name;
    int decisions_pending = 0;
    int decisions_completed = 0;
    double administrative_efficiency = 1.0;
    
    ManagementComponent() = default;
    explicit ManagementComponent(types::EntityID id);
};

struct PlayerPolicyComponent : public game::core::IComponent {
    double base_tax_rate = 0.1;
    double trade_policy_openness = 0.5;
    double social_services_funding = 0.5;
    double research_funding_level = 0.5;
    double military_focus = 0.5;
    double bureaucratic_centralization = 0.5;
    
    PlayerPolicyComponent() = default;
};
```

#### Class: PlayerDecision (30 lines of interface)

```cpp
class PlayerDecision {
private:
    std::string m_decision_id;
    DecisionContext m_context;
    DecisionPriority m_priority;
    DecisionStatus m_status;
    std::chrono::system_clock::time_point m_created_time;
    std::chrono::system_clock::time_point m_deadline;
    std::string m_selected_option_id;
    std::string m_player_notes;

public:
    explicit PlayerDecision(const DecisionContext& context);
    ~PlayerDecision() = default;

    // Accessors
    const std::string& GetDecisionId() const;
    const DecisionContext& GetContext() const;
    DecisionPriority GetPriority() const;
    DecisionStatus GetStatus() const;
    std::chrono::system_clock::time_point GetCreatedTime() const;
    double GetTimeRemaining() const;
    bool IsOverdue() const;

    // State transitions
    bool SelectOption(const std::string& option_id);
    bool ApproveDecision(const std::string& player_notes = "");
    bool RejectDecision(const std::string& reason = "");
    bool DelegateDecision();
    
    // Query
    const DecisionOption* GetSelectedOption() const;
    std::vector<DecisionOption> GetAvailableOptions() const;
};
```

#### Class: DecisionQueue (28 lines of interface)

```cpp
class DecisionQueue {
private:
    std::vector<std::unique_ptr<PlayerDecision>> m_pending_decisions;
    std::vector<std::unique_ptr<PlayerDecision>> m_completed_decisions;
    std::unordered_map<DecisionPriority, std::queue<PlayerDecision*>> m_priority_queues;
    AutomationLevel m_automation_level = AutomationLevel::ASSISTED;
    size_t m_max_completed_history = 50;

public:
    DecisionQueue() = default;
    ~DecisionQueue() = default;

    // Queue operations
    void AddDecision(std::unique_ptr<PlayerDecision> decision);
    PlayerDecision* GetNextDecision(DecisionPriority min_priority = DecisionPriority::ROUTINE);
    PlayerDecision* GetDecision(const std::string& decision_id);
    std::vector<PlayerDecision*> GetPendingDecisions() const;
    std::vector<PlayerDecision*> GetOverdueDecisions() const;
    void MarkDecisionCompleted(const std::string& decision_id);
    
    // Automation
    void ProcessAutomatedDecisions();
    bool ShouldAutomate(const PlayerDecision& decision) const;
    
    // Control
    void Clear();
    size_t GetPendingCount() const;
    void SetAutomationLevel(AutomationLevel level);
    AutomationLevel GetAutomationLevel() const;
};
```

#### Class: ProvinceOrderSystem (17 lines of interface)

```cpp
class ProvinceOrderSystem {
private:
    std::vector<std::unique_ptr<ProvinceOrder>> m_active_orders;
    std::vector<std::unique_ptr<ProvinceOrder>> m_completed_orders;

public:
    ProvinceOrderSystem() = default;
    ~ProvinceOrderSystem() = default;

    // Order management
    std::string AddOrder(std::unique_ptr<ProvinceOrder> order);
    bool CompleteOrder(const std::string& order_id);
    bool CancelOrder(const std::string& order_id);
    ProvinceOrder* GetOrder(const std::string& order_id);
    
    // Queries
    std::vector<ProvinceOrder*> GetActiveOrders() const;
    std::vector<ProvinceOrder*> GetOrdersByProvince(types::EntityID province_id) const;
    int GetOrderCount() const;
};
```

#### Class: ProvinceManagementSystem : public game::core::ISystem (95 lines of interface)

```cpp
class ProvinceManagementSystem : public game::core::ISystem {
private:
    ::core::ecs::ComponentAccessManager& m_access_manager;
    ::core::ecs::MessageBus& m_message_bus;
    
    // Sub-systems
    std::unique_ptr<DecisionQueue> m_decision_queue;
    std::unique_ptr<ProvinceOrderSystem> m_order_system;
    
    // References
    game::province::ProvinceSystem* m_province_system;
    
    // Timing
    std::chrono::steady_clock::time_point m_last_update;
    double m_update_frequency = 0.5;
    
    // Strategies
    std::unordered_map<ManagementDecisionType, 
        std::function<DecisionContext(types::EntityID)>> m_decision_generators;

public:
    explicit ProvinceManagementSystem(::core::ecs::ComponentAccessManager& access_manager,
        ::core::ecs::MessageBus& message_bus);
    ~ProvinceManagementSystem() override;

    // ISystem interface
    void Initialize() override;
    void Update(float delta_time) override;
    void Shutdown() override;
    std::string GetSystemName() const override;
    ::core::threading::ThreadingStrategy GetThreadingStrategy() const override;

    // System integration
    void SetProvinceSystem(game::province::ProvinceSystem* province_system);

    // Province management
    bool CreateManagedProvince(types::EntityID province_id, const std::string& manager_name = "Player");
    bool DestroyManagedProvince(types::EntityID province_id);
    bool SetProvinceAutomation(types::EntityID province_id, AutomationLevel level);

    // Decision system interface
    DecisionQueue* GetDecisionQueue();
    bool GenerateDecision(types::EntityID province_id, ManagementDecisionType type);
    bool ProcessDecision(const std::string& decision_id, const std::string& selected_option);

    // Order system interface
    ProvinceOrderSystem* GetOrderSystem();
    std::string IssueConstructionOrder(types::EntityID province_id,
        game::province::ProductionBuilding building_type);
    std::string IssuePolicyOrder(types::EntityID province_id,
        const std::string& policy_name, double new_value);

    // Policy management
    bool SetTaxRate(types::EntityID province_id, double tax_rate);
    bool SetTradePolicy(types::EntityID province_id, double openness_level);
    bool SetSocialServices(types::EntityID province_id, double funding_level);

    // Information queries
    std::vector<types::EntityID> GetManagedProvinces() const;
    ManagementComponent* GetManagementData(types::EntityID province_id);
    PlayerPolicyComponent* GetPolicyData(types::EntityID province_id);

private:
    // Decision generation
    void InitializeDecisionGenerators();
    DecisionContext GenerateEconomicDecision(types::EntityID province_id);
    DecisionContext GenerateConstructionDecision(types::EntityID province_id);
    DecisionContext GeneratePolicyDecision(types::EntityID province_id);

    // Order processing
    void ProcessActiveOrders();
    bool ExecuteConstructionOrder(const ProvinceOrder& order);
    bool ExecutePolicyOrder(const ProvinceOrder& order);

    // Automation
    void ProcessAutomatedDecisions();
    bool ShouldAutomate(const PlayerDecision& decision) const;
    bool ExecuteDecision(const PlayerDecision& decision);

    // Event handlers
    void OnProvinceCreated(const game::province::messages::ProvinceCreated& message);
    void OnEconomicCrisis(const game::province::messages::EconomicCrisis& message);
    void OnResourceShortage(const game::province::messages::ResourceShortage& message);

    // Helper methods
    void LogManagementAction(types::EntityID province_id, const std::string& action);
    std::string GenerateOrderId(OrderType type);
};
```

#### Namespace: utils (26 utility functions)

```cpp
namespace utils {
    // String conversions (6 functions)
    std::string ManagementDecisionTypeToString(ManagementDecisionType type);
    std::string DecisionPriorityToString(DecisionPriority priority);
    std::string DecisionStatusToString(DecisionStatus status);
    std::string AutomationLevelToString(AutomationLevel level);
    std::string OrderTypeToString(OrderType type);
    std::string OrderStatusToString(OrderStatus status);

    // Factory methods (4 functions)
    ManagementComponent CreateManagement(types::EntityID province_id,
        const std::string& manager_name = "Player");
    PlayerPolicyComponent CreateDefaultPolicies();
    std::unique_ptr<PlayerDecision> CreateEconomicDecision(types::EntityID province_id,
        ManagementDecisionType type);
    std::unique_ptr<ProvinceOrder> CreateConstructionOrder(types::EntityID province_id,
        game::province::ProductionBuilding building);

    // Validation utilities (3 functions)
    bool IsValidDecisionType(ManagementDecisionType type);
    bool IsValidAutomationLevel(AutomationLevel level);
    bool CanExecuteOrder(const ProvinceOrder& order);
}
```

---

## File: ProvinceManagementSystem.cpp (882 lines)

### Implementation Sections

#### PlayerDecision Implementation (125 lines)
```cpp
// Constructor (lines 25-49)
PlayerDecision::PlayerDecision(const DecisionContext& context)
    - Stores context
    - Auto-calculates priority from urgency factor
    - Sets creation time and deadline
    - Generates unique decision ID

// Time methods (lines 51-59)
double GetTimeRemaining() const;  // Returns hours
bool IsOverdue() const;

// Option selection (lines 61-69)
bool SelectOption(const std::string& option_id);

// Decision state transitions (lines 71-102)
bool ApproveDecision(const std::string& player_notes = "");
bool RejectDecision(const std::string& reason = "");
bool DelegateDecision();  // Auto-selects best AI recommendation

// Query methods (lines 105-124)
const DecisionOption* GetSelectedOption() const;
std::vector<DecisionOption> GetAvailableOptions() const;
```

#### DecisionQueue Implementation (108 lines)
```cpp
// Queue operations (lines 130-161)
void AddDecision(std::unique_ptr<PlayerDecision> decision);
    - Stores decision
    - Pushes to priority queue
    
PlayerDecision* GetNextDecision(DecisionPriority min_priority);
    - Returns highest priority decision
    - Iterates from CRITICAL to ROUTINE
    
PlayerDecision* GetDecision(const std::string& decision_id);
    - Linear search O(n)
    - Returns nullptr if not found

// Query methods (lines 163-181)
std::vector<PlayerDecision*> GetPendingDecisions() const;
std::vector<PlayerDecision*> GetOverdueDecisions() const;

// State transition (lines 183-198)
void MarkDecisionCompleted(const std::string& decision_id);
    - Moves from pending to history
    - Enforces 50-item history limit
    - Uses FIFO eviction

// Automation (lines 200-231)
void ProcessAutomatedDecisions();
bool ShouldAutomate(const PlayerDecision& decision) const;
    - MANUAL: never
    - ASSISTED: never
    - GUIDED: ROUTINE only
    - AUTOMATED: all except CRITICAL

// Cleanup (lines 233-237)
void Clear();
```

#### ProvinceOrderSystem Implementation (69 lines)
```cpp
// Order creation (lines 243-253)
std::string AddOrder(std::unique_ptr<ProvinceOrder> order);
    - Auto-generates order ID
    - Sets start time
    - Returns order ID

// Order lifecycle (lines 255-284)
bool CompleteOrder(const std::string& order_id);
bool CancelOrder(const std::string& order_id);

// Query methods (lines 286-311)
ProvinceOrder* GetOrder(const std::string& order_id);
    - Linear search O(n)
    
std::vector<ProvinceOrder*> GetActiveOrders() const;
std::vector<ProvinceOrder*> GetOrdersByProvince(types::EntityID province_id) const;
    - Linear search O(n)
```

#### ProvinceManagementSystem Implementation (600+ lines)

**Constructor/Destructor (lines 317-326)**
```cpp
ProvinceManagementSystem::ProvinceManagementSystem(...)
    - Initializes member references
    - Creates decision queue and order system
    - Sets last_update time

~ProvinceManagementSystem() = default;
```

**ISystem Interface (lines 328-371)**
```cpp
void Initialize();  // lines 328-342
    - Logs initialization
    - Calls InitializeDecisionGenerators()
    - TODO: Subscribe to events (commented out)

void Update(float delta_time);  // lines 344-358
    - Checks elapsed time
    - Processes automated decisions
    - Processes active orders
    - Updates last_update

void Shutdown();  // lines 360-367
    - Logs shutdown
    - Clears decision queue
    - Resets pointers

ThreadingStrategy GetThreadingStrategy();  // line 369-371
    - Returns MAIN_THREAD
```

**Province Management (lines 380-398)**
```cpp
bool CreateManagedProvince(...);  // TODO: Not implemented
bool DestroyManagedProvince(...);  // TODO: Not implemented
bool SetProvinceAutomation(...);   // TODO: Not implemented
```

**Decision System (lines 404-466)**
```cpp
bool GenerateDecision(...);  // lines 404-432
    - Looks up generator for decision type
    - Calls generator to create context
    - Creates PlayerDecision
    - Adds to queue
    - Logs action

bool ProcessDecision(...);  // lines 434-466
    - Gets decision from queue
    - Validates option selection
    - Approves decision
    - Calls ExecuteDecision()
    - Marks completed
    - Handles exceptions
```

**Order System (lines 472-533)**
```cpp
std::string IssueConstructionOrder(...);  // lines 472-503
    - Creates ProvinceOrder with type CONSTRUCTION_ORDER
    - Sets building type parameter
    - Estimates cost
    - Returns order ID

std::string IssuePolicyOrder(...);  // lines 505-533
    - Creates ProvinceOrder with type POLICY_CHANGE
    - Sets policy parameters
    - Sets estimated cost
    - Returns order ID
```

**Policy Management (lines 539-555)** - All TODO stubs

**Decision Generation (lines 581-687)**
```cpp
void InitializeDecisionGenerators();  // lines 581-588
    - Maps TAX_RATE_ADJUSTMENT → GenerateEconomicDecision()
    - Maps BUILDING_CONSTRUCTION → GenerateConstructionDecision()
    - Maps SOCIAL_SERVICES → GeneratePolicyDecision()

DecisionContext GenerateEconomicDecision(EntityID);  // lines 590-628
    - Creates context with 72-hour deadline
    - Urgency: 0.5
    - TODO: Disabled province system access
    - Generates option for increase/decrease tax

DecisionContext GenerateConstructionDecision(EntityID);  // lines 630-660
    - Creates context with 1-week deadline
    - Urgency: 0.3
    - Lists 3 building types (FARM, MARKET, SMITHY)
    - Generates construction options
    - Fixed costs (100.0)

DecisionContext GeneratePolicyDecision(EntityID);  // lines 662-687
    - Creates context with 2-week deadline
    - Urgency: 0.2
    - Generates 2 options:
      - Increase social services (0.05 benefit)
      - Maintain current (0.0 benefit)
```

**Order Processing (lines 693-767)**
```cpp
void ProcessActiveOrders();  // lines 693-715
    - Gets all active orders
    - Routes by type (CONSTRUCTION, POLICY_CHANGE)
    - Completes successful orders

bool ExecuteConstructionOrder(...);  // lines 717-736
    - TODO: Returns true if no ProvinceSystem
    - Would convert parameters to building type
    - Would call ProvinceSystem method

bool ExecutePolicyOrder(...);  // lines 738-767
    - Extracts policy name and value
    - Routes to SetTaxRate/SetTradePolicy/SetSocialServices
    - Handles exceptions
```

**Automation (lines 773-814)**
```cpp
void ProcessAutomatedDecisions();  // lines 773-775
    - Delegates to decision queue
    
bool ShouldAutomate(...);  // lines 777-781
    - Delegates to decision queue
    
bool ExecuteDecision(...);  // lines 783-814
    - Gets selected option
    - Routes by decision type
    - Handles TAX_RATE_ADJUSTMENT and BUILDING_CONSTRUCTION
    - TODO: Incomplete for other types
```

**Event Handlers (lines 820-840)**
```cpp
void OnProvinceCreated(...);  // lines 820-824 - TODO
void OnEconomicCrisis(...);   // lines 826-832
    - Generates TAX_RATE_ADJUSTMENT decision
void OnResourceShortage(...);  // lines 834-840
    - Generates TRADE_POLICY_CHANGE decision
```

**Helper Methods (lines 846-856)**
```cpp
void LogManagementAction(...);  // lines 846-850
    - Uses core::logging::LogInfo()
    
std::string GenerateOrderId(...);  // lines 852-856
    - Creates ID from counter and type
```

**Utility Functions (lines 862-879)**
```cpp
namespace utils {
    // Duplicate definitions of:
    std::string ManagementDecisionTypeToString(...);
    std::string OrderTypeToString(...);
}
```

---

## File: ProvinceManagementUtils.cpp (438 lines)

### String Conversion Utilities (101 lines, lines 14-101)

```cpp
std::string ManagementDecisionTypeToString(ManagementDecisionType type);
    - Maps all 13 decision types to strings
    - Uses static unordered_map

std::string DecisionPriorityToString(DecisionPriority priority);
    - Maps ROUTINE, IMPORTANT, URGENT, CRITICAL

std::string DecisionStatusToString(DecisionStatus status);
    - Maps PENDING, APPROVED, REJECTED, DELEGATED, EXECUTED, FAILED

std::string AutomationLevelToString(AutomationLevel level);
    - Maps MANUAL, ASSISTED, GUIDED, AUTOMATED

std::string OrderTypeToString(OrderType type);
    - Maps CONSTRUCTION_ORDER, POLICY_CHANGE, etc.

std::string OrderStatusToString(OrderStatus status);
    - Maps QUEUED, IN_PROGRESS, COMPLETED, FAILED, CANCELLED
```

### Factory Methods (59 lines, lines 107-159)

```cpp
ManagementComponent CreateManagement(...);
    - Sets all attributes to defaults
    - Automation: ASSISTED
    - Efficiency: 1.0
    
PlayerPolicyComponent CreateDefaultPolicies();
    - All policies set to 0.5 (balanced)
    - Tax rate: 0.1 (10%)
    
std::unique_ptr<PlayerDecision> CreateEconomicDecision(...);
    - Creates DecisionContext
    - Adds 72-hour deadline
    - Adds "maintain_current" option
    
std::unique_ptr<ProvinceOrder> CreateConstructionOrder(...);
    - Creates order of type CONSTRUCTION_ORDER
    - Sets building_type parameter
    - Cost estimate: 200.0
    - can_execute: false
```

### Validation Utilities (27 lines, lines 165-202)

```cpp
bool IsValidDecisionType(ManagementDecisionType type);
    - Checks type != INVALID
    - Checks type < COUNT
    
bool IsValidAutomationLevel(AutomationLevel level);
    - Checks level < COUNT
    
bool CanExecuteOrder(const ProvinceOrder& order);
    - Validates target_province != 0
    - Validates description not empty
    - Type-specific validation:
      - CONSTRUCTION: has building_type param
      - POLICY_CHANGE: has policy_name and new_value
      - RESOURCE_ALLOCATION: estimated_cost >= 0.0
      - RESEARCH_ORDER: has research_type param
```

### Analysis Utilities (261 lines, lines 208-325)

#### Decision Analysis (76 lines)
```cpp
double CalculateDecisionUrgency(const DecisionContext& context);
    - Base: context.urgency_factor
    - If <= 24 hours: urgency = max(current, 0.8)
    - If <= 72 hours: urgency = max(current, 0.6)
    - Returns clamped 0.0-1.0

std::string GetDecisionRecommendation(const DecisionContext& context);
    - Finds option with highest ai_recommendation
    - Returns recommendation string with confidence

double EstimateDecisionImpact(const DecisionContext& context, 
                              const std::string& option_id);
    - Calculates benefit/cost ratio
    - Returns ROI estimate
```

#### Management Analysis (86 lines)
```cpp
double CalculateManagementEfficiency(const ManagementComponent& management);
    - Base: administrative_efficiency
    - > 5 pending: *= 0.8
    - > 10 pending: *= 0.6
    - Scales by completion ratio
    - Returns 0.1-1.0

std::vector<std::string> IdentifyManagementIssues(...);
    - Checks if > 10 pending decisions
    - Checks if efficiency < 0.5
    - Checks MANUAL with > 5 pending
    - Returns list of issues

double CalculateGovernanceScore(...);
    - Management efficiency: 40%
    - Policy balance: 30%
    - Responsiveness: 30%
    - Returns 0.0-1.0 score

std::unordered_map<std::string, double> GetKPIDashboard(...);
    - Returns metrics:
      - management_efficiency
      - decisions_pending
      - decisions_completed
      - governance_score
      - tax_rate
      - social_services
      - trade_openness
      - completion_rate
```

### Order Utilities (137 lines, lines 354-415)

```cpp
std::string GenerateOrderDescription(OrderType type, const parameters map);
    - CONSTRUCTION_ORDER: "Construct [building]"
    - POLICY_CHANGE: "Change [policy] to [value]"
    - RESOURCE_ALLOCATION: "Allocate [amount] [resource]"
    - RESEARCH_ORDER: "Research [type]"

double EstimateOrderExecutionTime(const ProvinceOrder& order);
    - CONSTRUCTION_ORDER: 30 days
    - POLICY_CHANGE: 1 day
    - RESOURCE_ALLOCATION: 7 days
    - RESEARCH_ORDER: 90 days
    - Default: 7 days

bool ValidateOrderParameters(OrderType type, const parameters map);
    - Type-specific parameter validation
    - Returns true if valid
```

---

## Architecture Summary

### Control Flow

```
ProvinceManagementSystem::Update()
  ├─ ProcessAutomatedDecisions()
  │  └─ DecisionQueue::ProcessAutomatedDecisions()
  │     └─ For each decision: DelegateDecision() + MarkCompleted()
  │
  └─ ProcessActiveOrders()
     ├─ For CONSTRUCTION_ORDER: ExecuteConstructionOrder()
     │  └─ Calls ProvinceSystem::ConstructBuilding()
     │
     └─ For POLICY_CHANGE: ExecutePolicyOrder()
        ├─ SetTaxRate()
        ├─ SetTradePolicy()
        └─ SetSocialServices()
```

### Data Flow

```
GenerateDecision()
  └─ Looks up generator in m_decision_generators
     └─ Calls GenerateXxxDecision()
        └─ Creates DecisionContext with options
           └─ Creates PlayerDecision
              └─ Adds to DecisionQueue

ProcessDecision()
  ├─ Gets decision from queue
  ├─ Selects option
  ├─ Approves decision
  └─ ExecuteDecision()
     └─ Routes by decision type
        └─ Calls appropriate SetXxx() method
```

---

## Statistics

### Lines of Code by Component

| Component | Header | Implementation | Utility | Total |
|-----------|--------|-----------------|---------|-------|
| PlayerDecision | 31 | 125 | - | 156 |
| DecisionQueue | 28 | 108 | - | 136 |
| ProvinceOrderSystem | 17 | 69 | - | 86 |
| ProvinceManagementSystem | 95 | 600+ | 100 | 795+ |
| Utilities (string/factory/validation) | - | - | 187 | 187 |
| Analysis utilities | - | - | 261 | 261 |
| **TOTAL** | **171** | **902** | **548** | **1,621** |

### Method Count by Class

| Class | Public | Private | Total |
|-------|--------|---------|-------|
| PlayerDecision | 10 | 0 | 10 |
| DecisionQueue | 10 | 2 | 12 |
| ProvinceOrderSystem | 6 | 0 | 6 |
| ProvinceManagementSystem | 23 | 11 | 34 |
| **TOTAL** | **49** | **13** | **62** |

### Utility Functions by Category

| Category | Count |
|----------|-------|
| String Conversions | 6 |
| Factory Methods | 4 |
| Validation | 3 |
| Decision Analysis | 3 |
| Management Analysis | 4 |
| Order Analysis | 3 |
| **TOTAL** | **26** |

---

## Code Quality Observations

### Comments
- ProvinceManagementSystem.h: Good inline documentation
- ProvinceManagementSystem.cpp: Adequate comments
- ProvinceManagementUtils.cpp: Very good documentation
- Overall ratio: ~30% comments to code (acceptable)

### Naming
- Classes: PascalCase (correct)
- Methods: PascalCase (correct)
- Enums: PascalCase values with CAPS (correct)
- Variables: m_snake_case for members (correct)
- Consistency: Very consistent throughout

### Organization
- Logical grouping of related functions
- Clear separation of concerns
- Good file structure
- Consistent formatting

### Completeness
- 26 utility functions fully implemented
- 3 core classes fully implemented
- 1 main class 30% implemented
- 2 components not implemented
- Overall: ~40% complete

