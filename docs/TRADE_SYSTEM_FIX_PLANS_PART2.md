# Trade System Fix Plans - Part 2
**Hub Manager, Market Dynamics, Trade Calculator, and Cross-Cutting Fixes**

This document continues from TRADE_SYSTEM_FIX_PLANS.md with the remaining 15 items.

---

## 11. HubManager - Include Outgoing Routes in Specialization

### Current Problem
```cpp
void HubManager::UpdateSpecializations(TradeHub& hub) {
    // Analyze trade patterns to determine specializations
    std::unordered_map<types::ResourceType, double> resource_volumes;
    
    // Only counts INCOMING routes!
    for (const auto& route_id : hub.incoming_route_ids) {
        auto route_it = m_active_routes.find(route_id);
        if (route_it != m_active_routes.end()) {
            const TradeRoute& route = route_it->second;
            resource_volumes[route.resource] += route.GetEffectiveVolume();
        }
    }
    
    // Outgoing routes ignored - exporter hubs never specialize correctly!
}
```

### Fix
```cpp
void HubManager::UpdateSpecializations(TradeHub& hub) {
    std::unordered_map<types::ResourceType, double> resource_volumes;
    
    // Count BOTH incoming AND outgoing routes
    for (const auto& route_id : hub.incoming_route_ids) {
        auto route_it = m_active_routes.find(route_id);
        if (route_it != m_active_routes.end()) {
            const TradeRoute& route = route_it->second;
            resource_volumes[route.resource] += route.GetEffectiveVolume();
        }
    }
    
    // NEW: Include outgoing routes
    for (const auto& route_id : hub.outgoing_route_ids) {
        auto route_it = m_active_routes.find(route_id);
        if (route_it != m_active_routes.end()) {
            const TradeRoute& route = route_it->second;
            resource_volumes[route.resource] += route.GetEffectiveVolume();
        }
    }
    
    // Determine top 3 resources by volume
    // ... existing specialization logic ...
}
```

---

## 12. HubManager - Externalize Hub Type Thresholds to Config

### Current Problem
```cpp
HubType HubManager::DetermineOptimalType(types::EntityID province_id) const {
    double total_volume = GetTotalTradeVolume(province_id);
    int route_count = GetRouteCount(province_id);

    // Magic numbers hardcoded - can't tune without recompile!
    if (total_volume > 1000.0 && route_count > 20) {
        return HubType::INTERNATIONAL_PORT;
    } else if (total_volume > 500.0 && route_count > 10) {
        return HubType::MAJOR_TRADING_CENTER;
    } else if (route_count > 6) {
        return HubType::CROSSROADS;
    } else if (total_volume > 100.0 || route_count > 3) {
        return HubType::REGIONAL_HUB;
    } else {
        return HubType::LOCAL_MARKET;
    }
}
```

### Fix

**Step 1:** Add to GameConfig.json:
```json
{
  "trade": {
    "hub_evolution_thresholds": {
      "international_port": {
        "min_volume": 1000.0,
        "min_routes": 20,
        "min_utilization": 0.8
      },
      "major_trading_center": {
        "min_volume": 500.0,
        "min_routes": 10,
        "min_utilization": 0.6
      },
      "crossroads": {
        "min_volume": 0.0,
        "min_routes": 6,
        "min_utilization": 0.5
      },
      "regional_hub": {
        "min_volume": 100.0,
        "min_routes": 3,
        "min_utilization": 0.4
      }
    }
  }
}
```

**Step 2:** Use config in DetermineOptimalType:
```cpp
HubType HubManager::DetermineOptimalType(types::EntityID province_id) const {
    auto& config = game::config::GameConfig::Instance();
    
    double total_volume = GetTotalTradeVolume(province_id);
    int route_count = GetRouteCount(province_id);
    
    auto hub_it = m_trade_hubs.find(province_id);
    double utilization = hub_it != m_trade_hubs.end() ? 
        hub_it->second.current_utilization : 0.0;
    
    // Check thresholds from config in priority order
    if (total_volume >= config.GetDouble("trade.hub_evolution_thresholds.international_port.min_volume", 1000.0) &&
        route_count >= config.GetInt("trade.hub_evolution_thresholds.international_port.min_routes", 20) &&
        utilization >= config.GetDouble("trade.hub_evolution_thresholds.international_port.min_utilization", 0.8)) {
        return HubType::INTERNATIONAL_PORT;
    }
    
    if (total_volume >= config.GetDouble("trade.hub_evolution_thresholds.major_trading_center.min_volume", 500.0) &&
        route_count >= config.GetInt("trade.hub_evolution_thresholds.major_trading_center.min_routes", 10) &&
        utilization >= config.GetDouble("trade.hub_evolution_thresholds.major_trading_center.min_utilization", 0.6)) {
        return HubType::MAJOR_TRADING_CENTER;
    }
    
    if (route_count >= config.GetInt("trade.hub_evolution_thresholds.crossroads.min_routes", 6) &&
        utilization >= config.GetDouble("trade.hub_evolution_thresholds.crossroads.min_utilization", 0.5)) {
        return HubType::CROSSROADS;
    }
    
    if (total_volume >= config.GetDouble("trade.hub_evolution_thresholds.regional_hub.min_volume", 100.0) ||
        route_count >= config.GetInt("trade.hub_evolution_thresholds.regional_hub.min_routes", 3)) {
        return HubType::REGIONAL_HUB;
    }
    
    return HubType::LOCAL_MARKET;
}
```

---

## 13. HubManager - Optimize Lock Scope

### Current Problem
```cpp
void HubManager::UpdateAllHubs() {
    std::lock_guard<std::mutex> lock(m_trade_mutex);  // ONE BIG LOCK
    
    for (auto& [province_id, hub] : m_trade_hubs) {
        // Long operation - holds lock entire time
        UpdateUtilization(hub);
        UpdateSpecializations(hub);
        
        // Check evolution
        if (ShouldEvolve(hub)) {
            EvolveHub(province_id);
        }
    }
    // Lock held for ENTIRE hub processing - blocks all trade operations!
}
```

### Fix

**Option A: Per-hub fine-grained locking**
```cpp
void HubManager::UpdateAllHubs() {
    // Get snapshot of hub IDs (minimal lock)
    std::vector<types::EntityID> hub_ids;
    {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        hub_ids.reserve(m_trade_hubs.size());
        for (const auto& [province_id, _] : m_trade_hubs) {
            hub_ids.push_back(province_id);
        }
    }
    
    // Process each hub individually
    for (auto province_id : hub_ids) {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        auto hub_it = m_trade_hubs.find(province_id);
        if (hub_it == m_trade_hubs.end()) continue;
        
        TradeHub& hub = hub_it->second;
        UpdateUtilization(hub);
        UpdateSpecializations(hub);
        
        if (ShouldEvolve(hub)) {
            EvolveHub(province_id);
        }
    }
}
```

**Option B: Split read/write phases (preferred)**
```cpp
void HubManager::UpdateAllHubs() {
    // Phase 1: Read-only analysis (can be done with read lock or even unlocked)
    struct HubAnalysis {
        types::EntityID province_id;
        double new_utilization;
        std::vector<types::ResourceType> new_specializations;
        HubType recommended_type;
    };
    
    std::vector<HubAnalysis> analyses;
    
    {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        analyses.reserve(m_trade_hubs.size());
        
        for (const auto& [province_id, hub] : m_trade_hubs) {
            HubAnalysis analysis;
            analysis.province_id = province_id;
            analysis.new_utilization = CalculateUtilization(hub);
            analysis.new_specializations = DetermineSpecializations(hub);
            analysis.recommended_type = DetermineOptimalType(province_id);
            analyses.push_back(analysis);
        }
    }
    
    // Phase 2: Apply changes (quick writes)
    {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        for (const auto& analysis : analyses) {
            auto hub_it = m_trade_hubs.find(analysis.province_id);
            if (hub_it == m_trade_hubs.end()) continue;
            
            TradeHub& hub = hub_it->second;
            hub.current_utilization = analysis.new_utilization;
            hub.specialized_resources = analysis.new_specializations;
            
            if (analysis.recommended_type != hub.hub_type) {
                EvolveHub(analysis.province_id);
            }
        }
    }
}
```

---

## 14. MarketDynamicsEngine - Unify Price Bounds

### Current Problem
```cpp
// In ApplySeasonalAdjustment - clamps to 50
market.current_price = TradeCalculator::ClampPrice(market.current_price, 0.1, 50.0);

// In ApplyMarketForces - clamps to 100
market.current_price = TradeCalculator::ClampPrice(market.current_price, 0.1, 100.0);

// In TradeCalculator - different bounds
return Clamp(price, 0.2, 5.0);  // Yet another range!
```

### Fix

**Step 1:** Add to GameConfig.json:
```json
{
  "trade": {
    "price_constraints": {
      "absolute_min": 0.1,
      "absolute_max": 100.0,
      "modifier_clamp_min": 0.2,
      "modifier_clamp_max": 5.0
    },
    "seasonal_normalization": {
      "enabled": true,
      "annual_average_target": 1.0,
      "max_seasonal_deviation": 0.3
    }
  }
}
```

**Step 2:** Centralize price clamping:
```cpp
// In TradeCalculator.h
static double ClampPrice(double price);  // Uses config
static double ClampPriceModifier(double modifier);  // Uses config

// In TradeCalculator.cpp
double TradeCalculator::ClampPrice(double price) {
    auto& config = game::config::GameConfig::Instance();
    double min_price = config.GetDouble("trade.price_constraints.absolute_min", 0.1);
    double max_price = config.GetDouble("trade.price_constraints.absolute_max", 100.0);
    return std::clamp(price, min_price, max_price);
}

double TradeCalculator::ClampPriceModifier(double modifier) {
    auto& config = game::config::GameConfig::Instance();
    double min_mod = config.GetDouble("trade.price_constraints.modifier_clamp_min", 0.2);
    double max_mod = config.GetDouble("trade.price_constraints.modifier_clamp_max", 5.0);
    return std::clamp(modifier, min_mod, max_mod);
}
```

**Step 3:** Normalize seasonal multipliers:
```cpp
// In TradeGoodProperties
std::array<double, 12> seasonal_demand_multipliers;  // Per month
std::array<double, 12> seasonal_supply_multipliers;

void NormalizeSeasonalMultipliers() {
    auto& config = game::config::GameConfig::Instance();
    if (!config.GetBool("trade.seasonal_normalization.enabled", true)) {
        return;
    }
    
    double target_avg = config.GetDouble("trade.seasonal_normalization.annual_average_target", 1.0);
    
    // Normalize demand multipliers to average 1.0
    double demand_sum = 0.0;
    for (double val : seasonal_demand_multipliers) {
        demand_sum += val;
    }
    double demand_avg = demand_sum / 12.0;
    double demand_adjustment = target_avg / demand_avg;
    
    for (double& val : seasonal_demand_multipliers) {
        val *= demand_adjustment;
    }
    
    // Same for supply
    double supply_sum = 0.0;
    for (double val : seasonal_supply_multipliers) {
        supply_sum += val;
    }
    double supply_avg = supply_sum / 12.0;
    double supply_adjustment = target_avg / supply_avg;
    
    for (double& val : seasonal_supply_multipliers) {
        val *= supply_adjustment;
    }
}
```

---

## 15. MarketDynamicsEngine - Data-Driven Random Shocks

### Current Problem
```cpp
void MarketDynamicsEngine::ProcessPriceShocks() {
    utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();

    if (rng.randomFloat(0.0f, 1.0f) < 0.001) { // Hardcoded 0.1%
        // ...
        double shock_magnitude = rng.randomFloat(-0.3f, 0.3f); // Hardcoded ±30%
        // ...
    }
}
```

### Fix
```json
{
  "trade": {
    "price_shocks": {
      "enabled": true,
      "probability_per_update": 0.001,
      "min_magnitude": -0.3,
      "max_magnitude": 0.3,
      "cooldown_months": 6.0,
      "prevent_compounding": true
    }
  }
}
```

```cpp
void MarketDynamicsEngine::ProcessPriceShocks() {
    auto& config = game::config::GameConfig::Instance();
    
    if (!config.GetBool("trade.price_shocks.enabled", true)) {
        return;
    }
    
    utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
    double shock_probability = config.GetDouble("trade.price_shocks.probability_per_update", 0.001);

    if (rng.randomFloat(0.0f, 1.0f) < shock_probability) {
        if (m_market_data.empty()) return;

        auto market_it = m_market_data.begin();
        std::advance(market_it, rng.randomInt(0, static_cast<int>(m_market_data.size()) - 1));
        
        MarketData& market = market_it->second;
        
        // Check if already in shock (prevent compounding)
        if (config.GetBool("trade.price_shocks.prevent_compounding", true)) {
            if (market.trend == PriceMovement::SHOCK_UP || 
                market.trend == PriceMovement::SHOCK_DOWN) {
                return;  // Skip - already shocked
            }
        }
        
        double min_mag = config.GetDouble("trade.price_shocks.min_magnitude", -0.3);
        double max_mag = config.GetDouble("trade.price_shocks.max_magnitude", 0.3);
        double shock_magnitude = rng.randomFloat(static_cast<float>(min_mag), static_cast<float>(max_mag));
        
        ApplyPriceShock(market_it->second.province_id, market_it->second.resource,
                      shock_magnitude, "Market volatility");
    }
}
```

---

## 16. MarketDynamicsEngine - Document Trend Thresholds

### Current Problem
```cpp
void MarketDynamicsEngine::UpdatePriceTrend(MarketData& market, double price_change) {
    if (price_change > 0.1) {  // Magic number - what units?
        market.trend = PriceMovement::RISING;
    } else if (price_change < -0.1) {  // Magic number
        market.trend = PriceMovement::FALLING;
    } else {
        market.trend = PriceMovement::STABLE;
    }
}
```

### Fix
```json
{
  "trade": {
    "price_trends": {
      "rising_threshold": 0.1,
      "falling_threshold": -0.1,
      "volatile_threshold": 0.3,
      "shock_threshold": 0.5,
      "units": "fractional_change_per_update"
    }
  }
}
```

```cpp
void MarketDynamicsEngine::UpdatePriceTrend(MarketData& market, double price_change) {
    auto& config = game::config::GameConfig::Instance();
    
    // Thresholds in fractional change (0.1 = 10% change)
    double rising_threshold = config.GetDouble("trade.price_trends.rising_threshold", 0.1);
    double falling_threshold = config.GetDouble("trade.price_trends.falling_threshold", -0.1);
    double volatile_threshold = config.GetDouble("trade.price_trends.volatile_threshold", 0.3);
    double shock_threshold = config.GetDouble("trade.price_trends.shock_threshold", 0.5);
    
    double abs_change = std::abs(price_change);
    
    if (abs_change > shock_threshold) {
        market.trend = (price_change > 0) ? PriceMovement::SHOCK_UP : PriceMovement::SHOCK_DOWN;
    } else if (abs_change > volatile_threshold) {
        market.trend = PriceMovement::VOLATILE;
    } else if (price_change > rising_threshold) {
        market.trend = PriceMovement::RISING;
    } else if (price_change < falling_threshold) {
        market.trend = PriceMovement::FALLING;
    } else {
        market.trend = PriceMovement::STABLE;
    }
}
```

---

## 17. Establish Global Lock Order

### Current Problem
```cpp
// TradeSystem has trade_mutex
std::mutex m_trade_mutex;

// MarketDynamicsEngine has market_mutex
std::mutex m_market_mutex;

// No defined lock order - potential deadlock:
// Thread A: locks trade → locks market
// Thread B: locks market → locks trade
// DEADLOCK!
```

### Fix

**Step 1:** Document lock hierarchy in header:
```cpp
/**
 * @file TradeSystem.h
 * @brief Trade System with thread-safe operations
 * 
 * LOCK HIERARCHY (always acquire in this order):
 * 1. m_market_mutex (highest - market data)
 * 2. m_trade_mutex (middle - routes and hubs)
 * 3. repository locks (lowest - ECS components)
 * 
 * NEVER lock in reverse order to prevent deadlocks!
 */
```

**Step 2:** Enforce with scoped lock guards:
```cpp
// Helper class to enforce lock order
class TradeSystemLockGuard {
public:
    TradeSystemLockGuard(std::mutex& market_mutex, std::mutex& trade_mutex)
        : m_market_lock(market_mutex)
        , m_trade_lock(trade_mutex) {
        // Locks acquired in correct order automatically
    }
    
private:
    std::lock_guard<std::mutex> m_market_lock;
    std::lock_guard<std::mutex> m_trade_lock;
};

// Usage
void SomeOperation() {
    TradeSystemLockGuard lock(m_market_mutex, m_trade_mutex);
    // Safe - locks acquired in correct order
}
```

**Step 3:** Add deadlock detection in debug builds:
```cpp
#ifndef NDEBUG
class LockOrderValidator {
public:
    static thread_local int current_lock_level;
    
    static void AcquireLock(int level, const char* name) {
        if (level <= current_lock_level) {
            std::cerr << "LOCK ORDER VIOLATION: Attempting to acquire " 
                     << name << " (level " << level 
                     << ") while holding level " << current_lock_level << std::endl;
            assert(false);
        }
        current_lock_level = level;
    }
    
    static void ReleaseLock(int level) {
        current_lock_level = level - 1;
    }
};

#define ACQUIRE_LOCK(mutex, level, name) \
    LockOrderValidator::AcquireLock(level, name); \
    std::lock_guard<std::mutex> lock_##level(mutex); \
    (void)lock_##level;
#else
#define ACQUIRE_LOCK(mutex, level, name) std::lock_guard<std::mutex> lock_##level(mutex);
#endif
```

---

## 18. TradeCalculator - Fix RNG Usage

### Current Problem
```cpp
// Header claims "pure" but uses RNG!
/**
 * @brief Pure calculation functions - NO side effects
 */
class TradeCalculator {
public:
    static double CalculateSupplyLevel(types::EntityID province_id, types::ResourceType resource);
};

// Implementation uses RNG - NOT PURE!
double TradeCalculator::CalculateSupplyLevel(types::EntityID province_id, types::ResourceType resource) {
    utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
    double base_supply = 1.0;
    double variation = rng.randomFloat(0.5f, 2.0f);  // Non-deterministic!
    return base_supply * variation;
}
```

### Fix Options

**Option A: Pass seeded RNG context**
```cpp
// Add context parameter to all RNG-using methods
struct TradeCalculationContext {
    uint64_t seed;  // Deterministic seed from province_id + resource + game_time
    utils::RandomGenerator rng;
    
    TradeCalculationContext(types::EntityID province, types::ResourceType resource, uint64_t game_tick) {
        seed = HashCombine(province, static_cast<uint64_t>(resource), game_tick);
        rng.setSeed(seed);
    }
};

double TradeCalculator::CalculateSupplyLevel(
    types::EntityID province_id, 
    types::ResourceType resource,
    const TradeCalculationContext& context  // NEW
) {
    double base_supply = 1.0;
    double variation = context.rng.randomFloat(0.5f, 2.0f);  // Deterministic!
    return base_supply * variation;
}
```

**Option B: Use deterministic province seeds (preferred)**
```cpp
double TradeCalculator::CalculateSupplyLevel(types::EntityID province_id, types::ResourceType resource) {
    // Create deterministic seed from province and resource
    uint64_t seed = HashCombine(static_cast<uint64_t>(province_id), 
                                static_cast<uint64_t>(resource));
    
    // Use local seeded RNG - deterministic
    utils::RandomGenerator local_rng;
    local_rng.setSeed(seed);
    
    double base_supply = 1.0;
    double variation = local_rng.randomFloat(0.5f, 2.0f);
    return base_supply * variation;
}
```

**Option C: Move randomness out of calculator**
```cpp
// TradeCalculator becomes truly pure
double TradeCalculator::CalculateSupplyLevel(
    double base_supply,
    double variation_factor  // Passed in, not random
) {
    return base_supply * variation_factor;
}

// Caller handles randomness
double GetSupplyForProvince(types::EntityID province_id, types::ResourceType resource) {
    double base = GetBaseSupply(province_id, resource);
    double variation = DeterministicVariation(province_id, resource);  // Seeded
    return TradeCalculator::CalculateSupplyLevel(base, variation);
}
```

---

## 19. TradeCalculator - Wire Route Type Economics

*Already covered in Fix #2*

---

## 20. TradeCalculator - Fix Hub Reputation Normalization

### Current Problem
```cpp
double TradeCalculator::CalculateHubReputation(double trade_volume, double security, double infrastructure) {
    double trade_volume_factor = std::min(2.0, trade_volume);  // Saturates at 2.0!
    // If trade_volume is 1000, this caps at 2.0 - meaningless
    
    double reputation = (trade_volume_factor + security + infrastructure) / 3.0;
    return Clamp(reputation, 0.0, 1.0);
}
```

### Fix
```cpp
double TradeCalculator::CalculateHubReputation(
    double trade_volume, 
    double hub_capacity,  // NEW: For normalization
    double security, 
    double infrastructure
) {
    // Normalize trade volume by capacity (0.0 to 1.0+ range)
    double utilization = hub_capacity > 0.0 ? (trade_volume / hub_capacity) : 0.0;
    utilization = std::min(2.0, utilization);  // Cap at 200% capacity
    
    // Weighted average
    double reputation = (utilization * 0.4 +  // 40% based on activity
                        security * 0.3 +      // 30% based on safety
                        infrastructure * 0.3  // 30% based on facilities
                       );
    
    return Clamp(reputation, 0.0, 1.0);
}
```

---

## 21-25. Cross-Cutting Improvements

### 21. Align Event Payloads
**Standard event structure:**
```cpp
struct TradeEventBase {
    std::string event_id;
    uint64_t timestamp;
    types::EntityID affected_province;
    
    // Impact metrics - always include both
    ImpactMetrics impact {
        double monthly_delta = 0.0;      // Change per month
        double total_impact = 0.0;       // Total over duration
        double duration_months = 0.0;    // Time horizon
        std::string impact_type = "";   // "revenue_loss", "cost_increase", etc.
    };
};
```

### 22. Serialization for Mutable Fields
```cpp
// Add to TradeRoute serialization
Json::Value TradeRoute::ToJson() const {
    Json::Value json;
    // ... existing fields ...
    
    // Mutable state
    json["status"] = static_cast<int>(status);
    json["disruption_count"] = disruption_count;
    json["is_recovering"] = is_recovering;
    json["recovery_progress"] = recovery_progress;
    
    return json;
}

// Add to MarketData serialization
Json::Value MarketData::ToJson() const {
    Json::Value json;
    // ... existing fields ...
    
    json["volatility_index"] = volatility_index;
    json["trend"] = static_cast<int>(trend);
    json["price_change_rate"] = price_change_rate;
    
    return json;
}
```

### 23. Unit Tests
```cpp
// Test suite structure
TEST(TradeCalculatorTest, MarketPriceCalculation) {
    // Test price calculation with various supply/demand ratios
}

TEST(TradeCalculatorTest, PriceClampingBounds) {
    // Test min/max price enforcement
}

TEST(TradeCalculatorTest, TransportCostByRouteType) {
    // Test that different route types have different costs
}

TEST(TradeCalculatorTest, SeasonalAdjustmentNormalization) {
    // Test that seasonal factors average to 1.0
}

TEST(TradeCalculatorTest, DeterministicSupplyCalculation) {
    // Test same inputs produce same outputs
}
```

### 24. Extract Magic Constants
See individual fixes above - all constants moved to `GameConfig.json` under `trade` section.

---

## Implementation Priority

### Phase 1: Critical Fixes (Prevents Bugs)
1. ✅ Enhanced validation (#1)
2. ✅ Eliminate state duplication (#3)
3. ✅ Shrink lock scope (#6-7)
4. ✅ Fix RNG determinism (#18)
5. ✅ Global lock order (#17)

### Phase 2: Economic Correctness
6. Route type modifiers (#2)
7. Profit math consistency (#4-5)
8. Hub upgrade math (#10)
9. Price bound unification (#14)
10. Hub reputation normalization (#20)

### Phase 3: Features & Polish
11. Disruption recovery (#8-9)
12. Hub specialization fix (#11)
13. Config externalization (#12-16)
14. Event standardization (#21)
15. Serialization (#22)

### Phase 4: Testing & Documentation
16. Unit tests (#23)
17. Integration tests
18. Performance profiling
19. Documentation updates

---
*End of Part 2 Fix Plans*
