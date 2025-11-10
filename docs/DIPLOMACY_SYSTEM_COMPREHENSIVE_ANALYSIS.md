# DIPLOMACY SYSTEM - COMPREHENSIVE ANALYSIS

## OVERVIEW
The game features a sophisticated, multi-layered diplomacy system built on an ECS (Entity Component System) architecture. The system integrates with economic, military, and AI systems to create a dynamic geopolitical simulation.

---

## 1. CORE DIPLOMACY CLASSES & COMPONENTS

### 1.1 Data Structures (DiplomacyComponents.h)

#### Enums - Diplomatic States
- **DiplomaticRelation** (6 values):
  - ALLIED - formal military alliance
  - FRIENDLY - positive but informal
  - NEUTRAL - no particular stance
  - UNFRIENDLY - negative stance
  - HOSTILE - aggressive intent
  - AT_WAR - active military conflict

#### Enums - Treaty Types
- **TreatyType** (8 types):
  - ALLIANCE - mutual defense
  - TRADE_AGREEMENT - commercial benefits
  - NON_AGGRESSION - peace pact
  - MARRIAGE_PACT - dynastic bonding
  - TRIBUTE - vassal/superior relationship
  - BORDER_AGREEMENT - border stability
  - MILITARY_ACCESS - troop passage rights
  - DEFENSIVE_LEAGUE - multi-party mutual defense

#### Enums - Diplomatic Actions
- **DiplomaticAction** (12 actions):
  - PROPOSE_ALLIANCE - formal alliance proposal
  - PROPOSE_TRADE - commercial agreement
  - DECLARE_WAR - military conflict
  - SUE_FOR_PEACE - peace negotiation
  - SEND_GIFT - diplomatic gift
  - DEMAND_TRIBUTE - economic extraction
  - ARRANGE_MARRIAGE - dynastic marriage
  - ESTABLISH_EMBASSY - diplomatic presence
  - RECALL_AMBASSADOR - withdraw ambassador
  - ISSUE_ULTIMATUM - final demands
  - MEDIATE_CONFLICT - conflict resolution
  - GUARANTEE_INDEPENDENCE - protection pact

#### Enums - Casus Belli (War Justifications)
- **CasusBelli** (8 types):
  - BORDER_DISPUTE - territorial conflict
  - TRADE_INTERFERENCE - economic blocking
  - DYNASTIC_CLAIM - inheritance rights
  - RELIGIOUS_CONFLICT - faith-based war
  - INSULT_TO_HONOR - personal grievance
  - BROKEN_TREATY - contract violation
  - PROTECTION_OF_ALLY - ally defense
  - LIBERATION_WAR - freedom struggle

#### Enums - Diplomatic Personalities
- **DiplomaticPersonality** (8 types):
  - AGGRESSIVE - warlike approach
  - DIPLOMATIC - peace-focused
  - ISOLATIONIST - self-sufficient
  - OPPORTUNISTIC - pragmatic/flexible
  - HONORABLE - treaty-respecting
  - TREACHEROUS - untrustworthy
  - MERCHANT - trade-focused
  - RELIGIOUS - faith-motivated

### 1.2 Data Structures - Core Objects

#### DiplomaticState
Represents bilateral relationship between two realms
```
Fields:
- other_realm: EntityID
- relation: DiplomaticRelation (current status)
- opinion: int [-100, 100] (sentiment rating)
- trust: double [0.0, 1.0] (reliability confidence)
- prestige_difference: double (relative power perception)
- recent_actions: deque<string> (action history, max 10)
- last_contact: timestamp
- diplomatic_incidents: int (count of problems)
- trade_volume: double (economic exchange)
- economic_dependency: double [0.0, 1.0]
- military_access: bool (troop passage rights)
- has_common_enemies: bool
- has_border_tensions: bool
- action_cooldowns: map of action->timestamp (spam prevention)
- last_major_action: timestamp
- opinion_history: deque<int> (rolling window, max 12)
- historical_opinion_average: double (long-term trend)

Key Methods:
- IsActionOnCooldown(DiplomaticAction) - prevents spam
- SetActionCooldown(DiplomaticAction, days) - enforce cooldown
- GetRemainingCooldownDays(DiplomaticAction) - check status
- ApplyOpinionDecay(time_delta, baseline) - passive drift
- ApplyTrustDecay(time_delta, baseline) - passive decay
- UpdateOpinionHistory(opinion) - track trends
- GetHistoricalOpinionAverage() - memory system
```

#### Treaty
Formal agreement between two realms
```
Fields:
- treaty_id: string (unique identifier)
- type: TreatyType
- signatory_a/b: EntityID (parties)
- terms: map<string, double> (parameters)
- conditions: vector<string> (requirements)
- signed_date/expiry_date: timestamps
- is_active: bool
- compliance_a/b: double [0.0, 1.0] (adherence rate)
- tribute_amount: double (annual payment)
- trade_bonus: double (economic benefit)

Key Methods:
- IsExpired() - check expiration
- IsBroken() - check violation
- GetOverallCompliance() - average adherence
```

#### DynasticMarriage
Alliance via marriage between two realms
```
Fields:
- marriage_id: string
- bride_realm/groom_realm: EntityID (participating nations)
- bride_character/groom_character: EntityID (individuals)
- diplomatic_bonus: double (default 20.0)
- inheritance_claim: double (succession rights)
- produces_alliance: bool (automatic treaty)
- marriage_date: timestamp
- is_active: bool
- children: vector<EntityID> (offspring)
```

#### DiplomaticProposal
Pending proposal for diplomatic action
```
Fields:
- proposal_id: string
- proposer/target: EntityID
- action_type: DiplomaticAction
- terms: map<string, double>
- conditions: vector<string>
- message: string (diplomatic flavor text)
- proposed_date/expiry_date: timestamps
- is_pending: bool
- ai_evaluation: double (AI assessment)
- acceptance_chance: double [0.0, 1.0]
```

### 1.3 ECS Components

#### DiplomacyComponent
Main diplomatic component for each realm
```
Fields:
- relationships: map<EntityID, DiplomaticState> (all bilateral relations)
- active_treaties: vector<Treaty> (active agreements)
- marriages: vector<DynasticMarriage> (dynastic bonds)
- allies: vector<EntityID> (cached ally list)
- enemies: vector<EntityID> (cached enemy list)
- personality: DiplomaticPersonality
- prestige: double (international status)
- diplomatic_reputation: double (reliability rating)
- war_weariness: double [0.0, 1.0] (war exhaustion)

Key Methods:
- GetRelationship(EntityID) - access state
- SetRelation(EntityID, DiplomaticRelation) - change status
- ModifyOpinion(EntityID, change, reason) - adjust sentiment
- AddTreaty(Treaty) - add agreement
- RemoveTreaty(treaty_id) - end treaty
- BreakTreaty(EntityID, TreatyType) - violate treaty
- GetTreatiesWith(EntityID) - query agreements
- HasTreatyType(EntityID, TreatyType) - check agreement
- IsAtWar() - check war status
- IsAtWarWith(EntityID) - specific conflict
- IsAlliedWith(EntityID) - check alliance
- GetWarEnemies()/GetMilitaryAllies() - cached lists
```

#### TreatyComponent
Individual treaty tracking
```
- treaty_type: TreatyType
- participant_1/2: EntityID
- start_date/end_date: timestamps
- compliance_rate: double
- is_active: bool
- parameters: map<string, double>
```

#### DiplomaticActionComponent
Action tracking
```
- actor/target: EntityID
- action_type: string
- description: string
- timestamp: system_clock::time_point
- impact_value: double
- is_resolved: bool
```

---

## 2. DIPLOMATIC SYSTEM (DiplomacySystem.h/cpp)

### 2.1 Main DiplomacySystem Class

**Responsibilities:**
- Orchestrate all diplomatic interactions
- Process proposals and treaties
- Calculate opinion changes
- Manage war declaration and peace
- Integrate with AI decision-making
- Implement cooldown system

### 2.2 Diplomatic Actions

#### Alliance Management
```cpp
bool ProposeAlliance(proposer, target, terms)
bool EstablishAlliance(realm_a, realm_b)
void BreakAllianceBidirectional(realm_a, realm_b)
```

#### Trade System
```cpp
bool ProposeTradeAgreement(proposer, target, trade_bonus, duration_years)
void UpdateTradeRelations(realm_id)
double CalculateTradeValue(realm_a, realm_b)
void ProcessTradeDisputes(realm_id)
```

#### War & Peace
```cpp
bool DeclareWar(aggressor, target, casus_belli)
bool SueForPeace(supplicant, victor, peace_terms)
void ProcessWarDeclaration(aggressor, defender, cb)
void HandleAllyActivation(war_leader, allies)
void ProcessPeaceNegotiation(realm_a, realm_b)
```

#### Marriage & Dynastics
```cpp
bool ArrangeMarriage(bride_realm, groom_realm, create_alliance)
void ProcessMarriageEffects(DynasticMarriage)
```

#### Diplomacy Exchange
```cpp
bool EstablishEmbassy(sender, host)
void RecallAmbassador(sender, host)
void SendDiplomaticGift(sender, recipient, value)
```

#### Treaty Management
```cpp
void ProcessTreatyCompliance(realm_id)
void UpdateTreatyStatus(treaty)
void HandleTreatyViolation(treaty_id, violator)
void BreakTreatyBidirectional(realm_a, realm_b, type)
```

### 2.3 Relationship Dynamics

#### Opinion System
- Range: -100 (maximum hatred) to +100 (maximum love)
- Baseline: 0 (neutral)
- **Opinion Changes:**
  - Alliance Formed: +20
  - Alliance Broken: -30
  - War Declared: -50
  - Peace Signed: +10
  - Trade Agreement: +5
  - Marriage Arranged: +20
  - Gift Sent: +5 to +30 (scales with gift value)
  - Embassy Established: +10
  - Embassy Recalled: -15
  - Treaty Honored: +1
  - Treaty Violated: -30
  - Border Incident: -5
  - Insult Given: -10
  - Praise Given: +5

#### Trust System
- Range: 0.0 (complete distrust) to 1.0 (complete trust)
- Baseline: 0.5 (neutral)
- **Trust Changes:**
  - Treaty Breach: -0.2
  - Betrayal: -0.3
  - Military Aggression: -0.2
  - Espionage Discovered: -0.15
  - Honoring Alliance: +0.1
  - Keeping Promise: +0.15
  - Diplomatic Support: +0.1
  - Trade Fulfilled: +0.05

#### Prestige System
- International status and power
- Gained from: Military victories, treaties, alliances
- Lost from: Treaty violations, defeats
- Affects: Opinion modifiers, war likelihood

### 2.4 Personality-Based Modifiers

Each personality type affects behavior:

| Personality | War Likelihood | Trade Preference | Alliance Pref | Speed Modifier |
|-----------|---|---|---|---|
| AGGRESSIVE | High (0.8) | Low | Low | Normal |
| DIPLOMATIC | Low (0.2) | Medium | High | Normal |
| ISOLATIONIST | Low (0.3) | Very Low | Very Low | Slow |
| OPPORTUNISTIC | High (0.7) | High | Medium | Fast |
| HONORABLE | Low (0.2) | Medium | High | Normal |
| TREACHEROUS | Very High | High | Low | Variable |
| MERCHANT | Low | Very High | Medium | Fast |
| RELIGIOUS | Medium | Low | High | Normal |

### 2.5 Cooldown System

Prevents spam of diplomatic actions with configurable delays:

```json
"cooldown": {
  "declare_war": 365 days,
  "propose_alliance": 180 days,
  "propose_trade": 90 days,
  "demand_tribute": 365 days,
  "issue_ultimatum": 180 days,
  "arrange_marriage": 730 days,
  "default": 30 days
}
```

Features:
- Per-action cooldown tracking
- Last major action timestamp
- GetRemainingCooldownDays() query
- IsActionOnCooldown() check

### 2.6 Decay & Erosion Systems

#### Opinion Decay
- Passive drift toward baseline (default 0 - neutral)
- Rate: 0.1 opinion points per time unit
- Personality affects rate:
  - FORGIVING: 1.5x faster forgetting
  - VENGEFUL: 0.5x slower forgetting
  - PRAGMATIC: 1.2x faster decay

#### Trust Decay
- Passive drift toward 0.5 (neutral trust)
- Rate: 0.01 trust per time unit
- Affected by diplomatic incidents
- Restoration when no incidents occur

#### Memory System
- Rolling window of last 12 opinion values
- Historical opinion average calculation
- Allows AI to detect betrayal patterns
- Distinguishes sudden betrayal from long-term enemy

---

## 3. DIPLOMATIC CALCULATOR

### 3.1 Opinion Calculations

```cpp
// Core calculation
int CalculateOpinionChange(
    const DiplomaticState& current_state,
    DiplomaticAction action,
    double magnitude = 1.0
)

// Personality-based decay
int CalculateOpinionDecay(
    int current_opinion,
    float time_delta,
    DiplomaticPersonality personality
)

// Base opinion between realms
int CalculateBaseOpinion(
    const DiplomacyComponent& realm1,
    const DiplomacyComponent& realm2
)

// Utility
int ClampOpinion(int opinion)  // [-100, 100]
```

### 3.2 Trust Calculations

```cpp
double CalculateTrustChange(
    double current_trust,
    DiplomaticIncident incident
)

double CalculateTrustDecay(
    double current_trust,
    int diplomatic_incidents,
    float time_delta
)

double CalculateTrustRestoration(
    double current_trust,
    float time_delta
)

double ClampTrust(double trust)  // [0.0, 1.0]
```

### 3.3 War Likelihood

```cpp
double CalculateWarLikelihood(
    const DiplomacyComponent& aggressor,
    const DiplomacyComponent& target,
    int opinion,
    double prestige_difference
)

// Personality-based preferences
double GetPersonalityWarLikelihood(DiplomaticPersonality)
double GetPersonalityTradePreference(DiplomaticPersonality)
double GetPersonalityAlliancePreference(DiplomaticPersonality)
```

### 3.4 Prestige Calculations

```cpp
double CalculatePrestige(
    const DiplomacyComponent& diplomacy,
    int alliance_count,
    int marriage_count,
    int hostile_count
)

int CalculatePrestigeOpinionModifier(double prestige_difference)
```

### 3.5 War Score

```cpp
double CalculateWarScore(types::EntityID realm1, types::EntityID realm2)
// Returns [0.0, 1.0] where:
// 0.0 = realm2 winning decisively
// 0.5 = stalemate
// 1.0 = realm1 winning decisively
```

### 3.6 Treaty Evaluation

```cpp
double CalculateAllianceValue(
    const DiplomacyComponent& evaluator,
    const DiplomacyComponent& potential_ally
)

double CalculateTradeTermsAcceptability(
    double trade_bonus,
    int current_opinion
)
```

---

## 4. DIPLOMATIC AI SYSTEM

### 4.1 DiplomaticAI Class

**Purpose:** Make AI-controlled nations strategically decide diplomatic actions

### 4.2 AI Decision Making

#### Main Decision Method
```cpp
std::vector<AIDecision> EvaluateDiplomaticOptions(types::EntityID realm_id)
```

Returns prioritized list of diplomatic actions to take

#### Alliance Decisions
```cpp
bool ShouldProposeAlliance(realm_id, candidate)
std::vector<EntityID> GetAllianceCandidates(realm_id, max_count)

double CalculateAllianceDesirability(
    const DiplomacyComponent& evaluator,
    const DiplomacyComponent& candidate
)
```

Factors considered:
- Mutual military strength
- Economic compatibility
- Common enemies
- Geographic proximity
- Personality alignment
- Alliance needs

#### War Decisions
```cpp
bool ShouldDeclareWar(realm_id, target)
std::vector<EntityID> GetWarTargets(realm_id, max_count)

double CalculateWarDesirability(
    const DiplomacyComponent& aggressor,
    const DiplomacyComponent& target
)
```

Factors considered:
- Military advantage
- Economic impact
- War weariness
- Casus belli strength
- Ally support probability
- Prestige gain

#### Trade Decisions
```cpp
bool ShouldProposeTrade(realm_id, candidate)

double CalculateTradeValue(
    const DiplomacyComponent& evaluator,
    const DiplomacyComponent& partner
)
```

### 4.3 AI Constraints

```cpp
bool NeedsAlliances(const DiplomacyComponent& diplomacy)
bool IsOverextendedInWar(const DiplomacyComponent& diplomacy)
```

---

## 5. DIPLOMATIC ACTION HANDLERS

### 5.1 Handler Architecture

#### IDiplomaticActionHandler Interface
```cpp
// Base interface for all diplomatic actions
struct DiplomaticActionResult {
    bool success;
    std::string message;
    int opinion_change;
    double trust_change;
};

virtual DiplomaticActionResult Execute(
    EntityID initiator,
    EntityID target,
    const map<string, double>& parameters = {}
) = 0;

virtual bool Validate(EntityID initiator, EntityID target) const = 0;
virtual std::string GetValidationFailureReason(...) const = 0;
```

#### BaseDiplomaticHandler
Common base class with helper methods:
- ValidateBasicRequirements()
- LogEvent()

### 5.2 Implemented Handlers

#### AllianceProposalHandler
```cpp
// Execute alliance proposal
// Validate: not already allied, not at war, positive opinion
// Effects: Create alliance treaty, opinion boost, military benefits
```

#### WarDeclarationHandler
```cpp
// Declare war and activate conflict
// Validate: valid casus belli, not already at war
// Effects: Relationship becomes AT_WAR, allies activated, economic damage
```

#### Additional Handler Framework
Other handlers can be implemented following same pattern:
- TradeProposalHandler
- MarriageProposalHandler
- GiftHandler
- EmbassyHandler
- PeaceNegotiationHandler

---

## 6. DIPLOMACY-ECONOMY BRIDGE

### 6.1 Sanctions & Embargoes

#### Sanction Types
```
TRADE_EMBARGO - Complete trade ban
PARTIAL_EMBARGO - Specific goods only
FINANCIAL_SANCTIONS - Restrict financial flows
TARIFF_INCREASE - Increased trade costs
ASSET_FREEZE - Freeze financial assets
DIPLOMATIC_ISOLATION - Diplomatic restrictions
```

#### Sanction Severity
```
MILD, MODERATE, SEVERE, TOTAL
```

#### Sanction System
```cpp
std::string ImposeSanction(
    EntityID imposer, EntityID target,
    SanctionType, SanctionSeverity, reason
)

std::string ImposeTradeEmbargo(
    EntityID imposer, EntityID target,
    resources
)

void LiftSanction(sanction_id)
void LiftAllSanctions(imposer, target)

// Queries
std::vector<Sanction> GetActiveSanctionsAgainst(target)
std::vector<Sanction> GetSanctionsImposedBy(imposer)
bool IsUnderSanction(realm_id)
double GetTotalSanctionImpact(realm_id)
```

Economic Impact:
- trade_reduction_factor: 0.0-1.0
- cost_increase_factor: multiplier
- monthly_economic_damage: direct cost
- opinion_modifier: -50 typical
- prestige_cost: 10.0 typical

### 6.2 Trade Agreements

```cpp
std::string CreateTradeAgreement(
    EntityID a, EntityID b,
    trade_bonus, duration_years
)

void TerminateTradeAgreement(agreement_id)
void RenewTradeAgreement(agreement_id, additional_years)

// Features
- preferential_access
- most_favored_nation status
- exclusive_trade_rights
- covered_resources filtering
- auto_renewal
```

Economic Benefits:
- trade_bonus_multiplier: 1.2x typical
- tariff_reduction: 0.5x typical
- monthly_revenue_bonus
- opinion_bonus: +10
- linked_treaty_id: diplomatic tie

### 6.3 Economic Dependency Analysis

```cpp
struct EconomicDependency {
    trade_dependency: [0.0, 1.0]
    resource_dependency: [0.0, 1.0]
    financial_dependency: [0.0, 1.0]
    overall_dependency: weighted average
    critical_imports: map<ResourceType, double>
    vulnerability_to_disruption: [0.0, 1.0]
    estimated_months_to_collapse: int
}

EconomicDependency CalculateDependency(realm_id, partner)
void UpdateAllDependencies()
void UpdateDependenciesForRealm(realm_id)

// Queries
std::vector<EconomicDependency> GetDependencies(realm_id)
std::vector<EntityID> GetCriticalTradingPartners(realm_id)
bool IsDependentOn(realm_id, partner, threshold=0.6)
double GetDependencyLevel(realm_id, partner)
```

Thresholds:
- HighlyDependent: > 0.6
- CriticallyDependent: > 0.8

### 6.4 War Economic Integration

```cpp
struct WarEconomicImpact {
    total_military_spending: int
    total_trade_losses: int
    total_infrastructure_damage: int
    total_population_loss: int
    
    monthly_war_cost: int
    monthly_trade_disruption: int
    
    disrupted_trade_routes: vector<string>
    affected_neutral_parties: set<EntityID>
    
    estimated_recovery_months: int
    economic_devastation: [0.0, 1.0]
}

void OnWarDeclared(aggressor, defender)
void OnPeaceTreaty(realm_a, realm_b)
void ProcessWarEconomics()

// Queries
WarEconomicImpact* GetWarImpact(aggressor, defender)
int GetMonthlyWarCost(realm_id)
std::vector<string> GetDisruptedTradeRoutes(realm_id)
```

### 6.5 Bidirectional Integration

**Diplomatic Event → Economic Impact:**
- OnAllianceFormed()
- OnAllianceBroken()
- OnTreatyViolation()
- OnDiplomaticGift()
- ApplyTreatyEconomicEffects()
- RemoveTreatyEconomicEffects()

**Economic Event → Diplomatic Impact:**
- OnTradeRouteDisrupted()
- OnEconomicCrisis()
- OnResourceShortage()
- AdjustOpinionBasedOnTrade()
- ProcessEconomicInfluenceOnRelations()

---

## 7. GAME CONFIGURATION (GameConfig.json)

### 7.1 Diplomacy Configuration

```json
"diplomacy": {
  "treaty_durations": {
    "non_aggression_duration_years": 5,
    "trade_agreement_duration_years": 20,
    "alliance_duration_years": 25,
    "marriage_pact_duration_years": 50,
    "default_treaty_duration_years": 10
  },
  
  "treaty_mechanics": {
    "treaty_compliance_threshold": 0.5,
    "marriage_base_bonus": 20.0,
    "max_opinion": 100,
    "min_opinion": -100,
    "max_recent_actions": 10
  },
  
  "relationship_thresholds": {
    "friendly_threshold": 80,
    "neutral_threshold": 20,
    "hostile_threshold": -50
  },
  
  "opinion_mechanics": {
    "opinion_per_gift_value": 0.1,
    "opinion_decay_rate": 0.1,
    "opinion_history_window": 12
  },
  
  "trust_mechanics": {
    "compliance_decay_rate": 0.01,
    "treaty_violation_penalty": 30.0,
    "trust_decay_days_threshold": 365,
    "trust_decay_rate": 0.01
  },
  
  "prestige_mechanics": {
    "prestige_per_ally": 2.0,
    "prestige_decay_rate": 0.1
  },
  
  "trade_mechanics": {
    "base_trade_value": 50.0,
    "ally_join_war_probability": 0.8
  },
  
  "cooldown": {
    "declare_war": 365,
    "propose_alliance": 180,
    "propose_trade": 90,
    "demand_tribute": 365,
    "issue_ultimatum": 180,
    "arrange_marriage": 730,
    "default": 30
  }
}
```

---

## 8. DATA FILES & NATION DEFINITIONS

### 8.1 Nations Data
Files: `data/nations/nations_*.json`
- Nations per region (Western, Eastern, Central Europe)
- Base diplomatic personality per nation
- Starting prestige values
- Relationship initialization

### 8.2 Technologies & Cultures
- Cultural groups affect alignment
- Technologies unlock diplomatic options
- Administrative technologies improve reputation

---

## 9. TESTS & VALIDATION

### 9.1 Test Suite

#### test_diplomacy_refactoring.cpp
- DiplomacyRepository functionality
- DiplomaticCalculator opinion/trust/war calculations
- AllianceProposalHandler
- WarDeclarationHandler
- DiplomaticAI decision-making
- Alliance candidate evaluation
- War target selection

#### test_diplomacy_cooldown.cpp
- Basic cooldown functionality
- Multiple action cooldowns
- Cooldown expiry mechanics
- Config-based defaults
- Last major action tracking
- Prevents diplomatic action spam

#### test_diplomacy_memory.cpp
- Opinion history tracking (rolling window)
- Long-term memory of relationships
- Historical average calculation
- Integration with ModifyOpinion
- Volatile opinion handling
- Current opinion vs historical comparison
- AI can detect pattern changes

#### test_diplomacy_decay.cpp
- Opinion decay toward baseline
- Trust decay toward neutral (0.5)
- Custom baseline support
- Trust clamping
- No overshoot behavior
- Time scaling verification
- Personality-based decay rates

---

## 10. UI COMPONENTS

### 10.1 NationOverviewWindow

**Location:** `src/ui/NationOverviewWindow.cpp`

Features:
- Tabbed interface (Economy, Military, Diplomacy)
- **Diplomacy Tab:**
  - Allied nations count
  - Rival nations count
  - Trade partners count
  - Active treaties list
  - Placeholder for real data integration

Current State: Placeholder implementation (ready for full ECS integration)

---

## 11. THREADING & PERFORMANCE

### 11.1 Thread Safety

- `DiplomacyRepository`: Thread-safe component access
- `DiplomacyEconomicBridge`: Shared mutexes for concurrent updates
  - m_sanctions_mutex
  - m_agreements_mutex
  - m_dependencies_mutex
  - m_wars_mutex

### 11.2 Update Strategy

- Monthly update cycle (real-time game time)
- Incremental processing of proposals
- Lazy evaluation of relationships
- Cached ally/enemy lists

---

## 12. FEATURE SUMMARY TABLE

| Feature Category | Count | Key Components |
|---|---|---|
| Diplomatic Relations | 6 states | ALLIED, FRIENDLY, NEUTRAL, UNFRIENDLY, HOSTILE, AT_WAR |
| Treaty Types | 8 | Alliance, Trade, Non-Aggression, Marriage, Tribute, Border, Military Access, Defensive League |
| Diplomatic Actions | 12 | Alliance, Trade, War, Peace, Gift, Marriage, Embassy, Ultimatum, Mediation, Independence Guarantee |
| Casus Belli | 8 | Border Dispute, Trade, Dynastic, Religious, Honor, Treaty Breach, Ally Protection, Liberation |
| Personalities | 8 | Aggressive, Diplomatic, Isolationist, Opportunistic, Honorable, Treacherous, Merchant, Religious |
| Calculation Types | 6+ | Opinion, Trust, War Likelihood, Prestige, War Score, Alliance Value |
| Sanction Types | 6 | Trade Embargo, Partial Embargo, Financial, Tariff, Asset Freeze, Diplomatic Isolation |
| Sanction Severity | 4 | Mild, Moderate, Severe, Total |
| Diplomatic Incidents | 8 | Treaty Breach, Betrayal, Aggression, Espionage, Honoring Alliance, Promise, Support, Trade |
| Economic Integrations | 5+ | Trade Agreements, Embargoes, Dependency Analysis, War Economics, Cross-System Impact |

---

## 13. ARCHITECTURE PATTERNS

### 13.1 Design Patterns Used

1. **ECS (Entity Component System)**
   - Entities are realms
   - Components store diplomatic data
   - Systems process updates

2. **Repository Pattern**
   - DiplomacyRepository encapsulates ECS access
   - Consistent interface for component operations

3. **Strategy Pattern**
   - Personality-based behavior modification
   - Different calculation strategies per personality

4. **Handler/Command Pattern**
   - IDiplomaticActionHandler base class
   - Specific handlers for different actions

5. **Observer Pattern**
   - MessageBus for event notification
   - Systems subscribe to diplomatic events

6. **Bridge Pattern**
   - DiplomacyEconomicBridge connects systems
   - Bidirectional impact updates

### 13.2 Integration Points

- **Military System:** War declaration, ally activation, military strength calculations
- **Economic System:** Trade values, tribute, sanctions, economic damage
- **AI System:** Decision-making, strategy planning, behavior modification
- **UI System:** Nation overview, diplomacy tab, relation displays
- **Scenario System:** Nation initialization, starting relations

---

## 14. ADVANCED FEATURES

### 14.1 Opinion Memory System

The system tracks a rolling window of 12 past opinion values:
- Calculates historical average
- Distinguishes sudden betrayal from long-term enmity
- Enables nuanced AI decision-making
- Prevents over-reaction to temporary fluctuations

### 14.2 Prestige System

Multi-faceted status tracking:
- Per-alliance prestige gain
- Per-marriage prestige gain
- War victory prestige
- Prestige affects opinion modifiers
- High prestige realm gets opinion bonuses

### 14.3 Economic Leverage

Tracks economic interdependence:
- Trade-based leverage
- Resource dependency
- Financial vulnerability
- Collapse timeline estimation
- Influences peace/war decisions

### 14.4 Dynamic Cooldown System

Per-action cooldown tracking:
- Prevents diplomatic action spam
- Configurable per action type
- Last major action timestamp
- Remaining time queries
- Config-based defaults

---

## CONCLUSION

The diplomacy system is a comprehensive, production-ready implementation featuring:

- **Realistic diplomatic interactions** with multiple action types
- **Dynamic relationship modeling** with opinion and trust
- **AI decision-making** based on strategic analysis
- **Economic integration** with sanctions and trade
- **Multi-layered conflict system** with war scores and peace negotiation
- **Dynasty system** linking nations through marriage
- **Personality-driven behavior** making each nation unique
- **Memory and decay systems** creating natural relationship erosion
- **Thread-safe concurrent access** for multi-threaded game loops
- **Comprehensive testing** validating core mechanics
- **Extensible handler architecture** for adding new diplomatic actions

The system demonstrates sophisticated game design balancing realism, complexity, playability, and performance.

