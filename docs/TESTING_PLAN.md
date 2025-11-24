# Comprehensive Testing Plan for Mechanica Imperii Game Systems

**Generated:** 2025-11-10
**Purpose:** Systematic testing, debugging, and verification of all game systems

---

## Testing Methodology

### Testing Categories
1. **Unit Testing** - Individual component/function testing
2. **Integration Testing** - System interaction testing
3. **Performance Testing** - CPU/memory profiling
4. **Thread Safety Testing** - Concurrent access validation
5. **Data Validation Testing** - Save/load integrity
6. **Regression Testing** - Ensure fixes don't break existing functionality

### Testing Priority Levels
- **P0 (Critical):** Core functionality, crashes, data corruption
- **P1 (High):** Major features, gameplay blockers
- **P2 (Medium):** Minor features, edge cases
- **P3 (Low):** Polish, optimizations

---

## CATEGORY 1: CORE FOUNDATION SYSTEMS (6 systems)

### 1.1 Entity Component System (ECS)
**Files:** `EntityManager.h`, `ComponentAccessManager.h`, `MessageBus.h`, `IComponent.h`, `ISystem.h`

**Test Plan:**
1. **Entity Management Tests**
   - Create/destroy entities (10,000+ entities)
   - Entity versioning validation
   - Handle recycling and invalidation
   - Test entity handle safety after destruction

2. **Component Tests**
   - Add/remove components of various types
   - CRTP type system validation
   - Component data integrity after multiple operations
   - Test multiple components per entity

3. **Message Bus Tests**
   - Send messages between systems
   - Message type safety
   - Message queue overflow handling
   - Message delivery order validation

4. **Thread Safety Tests**
   - Concurrent component access (shared locks)
   - Exclusive lock validation (writes)
   - Deadlock detection
   - Race condition testing with ThreadSanitizer

**Verification Checklist:**
- [ ] No crashes with 10k+ entities
- [ ] No memory leaks (Valgrind)
- [ ] Thread-safe access confirmed
- [ ] Message delivery 100% reliable
- [ ] Entity versioning prevents stale access

**Priority:** P0 (Critical - Foundation for everything)

---

### 1.2 Threading System
**Files:** `ThreadedSystemManager.h`, `ThreadSafeMessageBus.h`, `ThreadingTypes.h`, `ScopeGuards.h`

**Test Plan:**
1. **Threading Strategy Tests**
   - MAIN_THREAD execution
   - THREAD_POOL distribution
   - DEDICATED_THREAD isolation
   - BACKGROUND_THREAD low-priority
   - HYBRID strategy switching

2. **Frame Synchronization Tests**
   - Frame barrier correctness
   - System execution order validation
   - Inter-frame dependencies
   - Game clock consistency

3. **Thread-Safe Message Bus Tests**
   - Priority queue ordering
   - Concurrent message posting
   - Message filtering
   - Performance under load

4. **Performance Tests**
   - Thread pool efficiency (target: 80%+ utilization)
   - Frame time consistency
   - Lock contention measurement
   - CPU core usage distribution

**Verification Checklist:**
- [ ] No deadlocks under any scenario
- [ ] Frame barriers work correctly
- [ ] Message priority respected
- [ ] Thread pool shows 80%+ utilization
- [ ] No data races (ThreadSanitizer)

**Priority:** P0 (Critical - Performance foundation)

---

### 1.3 Save System
**Files:** `SaveManager.h`, `SaveCompression.h`, `IncrementalSaveTracker.h`, `SaveManagerRecovery.cpp`

**Test Plan:**
1. **Full Save Tests**
   - Save complete game state
   - Load and verify all data matches
   - Test with 1k+ entities
   - Verify all component types saved

2. **Incremental Save Tests**
   - Dirty component tracking
   - Delta save size validation (should be <10% of full save)
   - Multiple incremental saves
   - Load full + incrementals = correct state

3. **Compression Tests**
   - LZ4 compression ratio (target: 60-80%)
   - Decompression speed
   - Corrupted compressed data handling
   - Large save files (100MB+)

4. **Validation Tests**
   - Schema versioning
   - Checksum validation
   - Corrupted save detection
   - Missing component handling

5. **Recovery Tests**
   - Load corrupted save
   - Repair mechanisms
   - Fallback to older saves
   - Recovery success rate

6. **Save/Load Cycle Tests**
   - Save → Load → Save → Load
   - Verify data identical after cycles
   - Test with active game (time running)

**Verification Checklist:**
- [ ] 100% data fidelity on save/load
- [ ] Compression 60-80% achieved
- [ ] Incremental saves work correctly
- [ ] Corrupted saves detected
- [ ] Recovery mechanisms functional
- [ ] No memory leaks during save/load

**Priority:** P0 (Critical - Data integrity)

---

### 1.4 Configuration System
**Files:** `GameConfig.h`, `GameConfig.json`, `ConfigHelpers.cpp`

**Test Plan:**
1. **Config Loading Tests**
   - Load all 119+ parameters
   - Validate default values
   - Missing parameter fallback
   - Invalid JSON handling
   - Type validation (numbers, strings, bools)

2. **Hot-Reload Tests**
   - Modify config file
   - Detect file changes
   - Reload without restart
   - Validate new values applied
   - Test while game running

3. **Parameter Access Tests**
   - Type-safe access for all types
   - Category-based access
   - Non-existent parameter handling

**Verification Checklist:**
- [ ] All 119 parameters load correctly
- [ ] Hot-reload works without restart
- [ ] Invalid configs don't crash
- [ ] Fallback defaults work
- [ ] Type safety maintained

**Priority:** P1 (High - Game tuning critical)

---

### 1.5 Logging System
**Files:** `Logger.h`

**Test Plan:**
1. **Log Level Tests**
   - DEBUG level messages
   - INFO level messages
   - WARN level messages
   - ERROR level messages
   - Log level filtering

2. **Performance Tests**
   - High-volume logging (10k+ messages)
   - Logging impact on frame time (<1%)
   - Thread-safe logging from multiple threads

3. **Output Tests**
   - Console output format
   - File output format
   - Structured metrics logging

**Verification Checklist:**
- [ ] All log levels work
- [ ] Thread-safe logging confirmed
- [ ] Performance impact <1%
- [ ] Log files created correctly

**Priority:** P2 (Medium - Important for debugging)

---

### 1.6 Type System
**Files:** `game_types.h`, `TypeRegistry.cpp`

**Test Plan:**
1. **Strong Type Tests**
   - ProvinceID creation and comparison
   - RealmID creation and comparison
   - CharacterID creation and comparison
   - EntityID creation and comparison
   - Invalid ID handling

2. **Type Registry Tests**
   - Component type registration
   - Type ID consistency
   - Lookup performance

**Verification Checklist:**
- [ ] All strong types work correctly
- [ ] Type safety enforced
- [ ] Type registry functional
- [ ] No ID collisions

**Priority:** P2 (Medium - Type safety important)

---

## CATEGORY 2: GAME ENTITY SYSTEMS (5 systems)

### 2.1 Time Management System
**Files:** `TimeManagementSystem.h`, `TimeComponents.h`, `TimeManagementSystem.cpp`

**Test Plan:**
1. **Game Clock Tests**
   - Time progression (day/month/year)
   - Leap year handling
   - Month day limits (28, 30, 31)
   - Time scale changes (PAUSED, SLOW, NORMAL, FAST, VERY_FAST)

2. **Event Scheduling Tests**
   - Schedule events at specific dates
   - Event execution timing validation
   - Event cancellation
   - Multiple events on same date
   - Event priority ordering

3. **Tick System Tests**
   - DAILY tick execution
   - MONTHLY tick execution
   - YEARLY tick execution
   - HOURLY tick execution
   - Tick frequency validation

4. **Message Delivery Tests**
   - Distance-based delays
   - Delivery time calculation
   - Message queue management

**Verification Checklist:**
- [ ] Calendar math correct
- [ ] Events execute on time
- [ ] All time scales work
- [ ] Message delays accurate
- [ ] No timing drift

**Priority:** P0 (Critical - Core game mechanic)

---

### 2.2 Province System
**Files:** `ProvinceSystem.h`, `Province.h`, `ProvinceComponent.h`, `ProvinceManagementSystem.h`

**Test Plan:**
1. **Province Data Tests**
   - Province creation with all properties
   - Terrain and climate data
   - Owner realm tracking
   - Connected provinces (adjacency)

2. **Province Management Tests**
   - Decision queue operations
   - Decision priority ordering
   - Status tracking
   - Player command delegation

3. **Integration Tests**
   - Province selection
   - Province queries
   - Iterator support
   - GameWorld container operations

**Verification Checklist:**
- [ ] All province properties stored
- [ ] Adjacency graph correct
- [ ] Decision queue works
- [ ] Owner tracking accurate

**Priority:** P0 (Critical - Core game entity)

---

### 2.3 Realm System
**Files:** `RealmManager.h`, `RealmComponents.h`, `RealmCalculator.h`, `RealmRepository.h`

**Test Plan:**
1. **Realm Data Tests**
   - Realm creation
   - Government type tracking
   - Succession law application
   - Heir management

2. **Vassalage Tests**
   - Vassal-overlord relationships
   - Vassalage hierarchy validation
   - Realm independence/subjugation

3. **Prestige System Tests**
   - Prestige gain/loss
   - Prestige calculations
   - Effects on diplomacy

**Verification Checklist:**
- [ ] Realms created correctly
- [ ] Vassalage hierarchy works
- [ ] Succession logic correct
- [ ] Prestige system functional

**Priority:** P0 (Critical - Core game entity)

---

### 2.4 Map System
**Files:** `MapSystem.h`, `MapData.h`, `ProvinceGeometry.h`, `GeoJSONLoader.h`, `ShapefileLoader.h`

**Test Plan:**
1. **Map Data Loading Tests**
   - GeoJSON province loading
   - Shapefile loading
   - Terrain data processing
   - Province boundary validation

2. **Spatial Index Tests**
   - Province lookup by position
   - Bounding box calculations
   - Spatial query performance
   - Adjacency detection

3. **Geometry Tests**
   - Province polygon validity
   - Province center calculation
   - Border detection

**Verification Checklist:**
- [ ] Map loads correctly
- [ ] All provinces have geometry
- [ ] Spatial queries fast (<1ms)
- [ ] Adjacency correct

**Priority:** P1 (High - Map is core feature)

---

### 2.5 Spatial Index
**Files:** `SpatialIndex.h`

**Test Plan:**
1. **Query Performance Tests**
   - Point-in-province query (<1ms)
   - Bounding box query
   - Radius query
   - Stress test with 1000+ queries/frame

2. **Accuracy Tests**
   - Correct province returned
   - Edge case handling (borders)
   - No false positives/negatives

**Verification Checklist:**
- [ ] Query speed <1ms
- [ ] 100% accuracy
- [ ] Handles edge cases

**Priority:** P1 (High - Performance critical)

---

## CATEGORY 3: PRIMARY GAME SYSTEMS (8 systems)

### 3.1 Population System
**Files:** `PopulationSystem.h`, `PopulationComponents.h`, `PopulationCalculator.h`, `PopulationAggregator.h`, `PopulationEventProcessor.h`

**Test Plan:**
1. **Demographics Tests**
   - Age/gender distribution
   - Birth rate calculation (3.5%)
   - Death rate calculation (3.0%)
   - Crisis multiplier effects (plague 3x, famine 2x, war 1.5x)

2. **Social Mobility Tests**
   - Class transitions (upward 0.5%, downward 0.3%)
   - Employment changes
   - Wealth distribution

3. **Cultural Integration Tests**
   - Cultural assimilation rates
   - Religious conversion
   - Cultural diversity tracking

4. **Settlement Tests**
   - Settlement type progression (hamlet → village → town → city)
   - Population thresholds
   - Growth factors

5. **Employment Tests**
   - Sector distribution (agriculture, craft, trade)
   - Unemployment tracking
   - Guild formation

6. **Population Events Tests**
   - Plague events
   - Famine events
   - Migration events
   - Literacy changes
   - Innovation events

**Verification Checklist:**
- [ ] Birth/death rates match config
- [ ] Crisis multipliers work
- [ ] Social mobility occurs
- [ ] Cultural assimilation gradual
- [ ] Settlement progression works
- [ ] Events trigger correctly

**Priority:** P0 (Critical - Core gameplay)

---

### 3.2 Economic System
**Files:** `EconomicSystem.h`, `EconomicComponents.h`

**Test Plan:**
1. **Treasury Tests**
   - Tax collection (base 10%)
   - Treasury limits enforcement
   - Minimum treasury maintained
   - Starting balance initialization

2. **Production Tests**
   - Production chains (raw → intermediate → finished)
   - Resource stockpiles
   - Production efficiency

3. **Market Tests**
   - Supply/demand calculations
   - Price fluctuations
   - Market equilibrium

4. **Trade Tests**
   - Trade efficiency (85%)
   - Trade income calculation
   - Trade route effects

5. **Economic Event Tests**
   - Good events (15% chance/month)
   - Bad events (15% chance/month)
   - Event weight distribution
   - Event effects on economy

6. **Inflation Tests**
   - Inflation rate (2%)
   - Price adjustments
   - Long-term stability

**Verification Checklist:**
- [ ] Tax collection accurate
- [ ] Production chains work
- [ ] Market prices realistic
- [ ] Trade income correct
- [ ] Events trigger at right frequency
- [ ] Inflation accumulates correctly

**Priority:** P0 (Critical - Core gameplay)

---

### 3.3 Trade System
**Files:** `TradeSystem.h`, `TradeRepository.h`, `TradeCalculator.h`, `MarketDynamicsEngine.h`, `HubManager.h`

**Test Plan:**
1. **Trade Route Tests**
   - Route types (land, river, coastal, sea, overland long)
   - Route establishment
   - Route disruption
   - Route status changes
   - Route abandonment

2. **Hub Tests**
   - Hub types (local market, regional hub, major center, international port, crossroads)
   - Hub creation and management
   - Regional trade center effects

3. **Market Dynamics Tests**
   - Price movements (stable, rising, falling, volatile, shock up/down)
   - Supply/demand effects
   - Market shocks

4. **Trade Route Handlers Tests**
   - EstablishRouteHandler functionality
   - DisruptRouteHandler functionality
   - Route validation
   - Cost calculations

**Verification Checklist:**
- [ ] All route types work
- [ ] Routes establish correctly
- [ ] Disruption mechanics work
- [ ] Hubs functional
- [ ] Market dynamics realistic
- [ ] Handlers process correctly

**Priority:** P1 (High - Important economic feature)

---

### 3.4 Military System
**Files:** `MilitarySystem.h`, `MilitaryComponents.h`, `MilitaryRecruitmentSystem.h`, `BattleResolutionCalculator.h`

**Test Plan:**
1. **Unit Management Tests**
   - Unit creation with all types
   - Unit stats validation
   - Unit upkeep costs
   - Unit equipment quality

2. **Army Tests**
   - Army creation
   - Commander assignment
   - Army movement
   - Army positioning
   - Army composition

3. **Recruitment Tests**
   - Recruit from population
   - Recruitment costs
   - Population depletion
   - Recruitment limits

4. **Battle Tests**
   - Battle initiation
   - Combat calculator
   - Unit composition effects
   - Commander effects
   - Terrain modifiers
   - Battle resolution
   - Casualty calculation
   - Victory/defeat determination

5. **Siege Tests**
   - Siege initiation
   - Siege progress
   - Fortification effects
   - Siege resolution

**Verification Checklist:**
- [ ] Units created correctly
- [ ] Armies functional
- [ ] Recruitment works
- [ ] Battle resolution realistic
- [ ] Siege mechanics work
- [ ] Upkeep costs applied

**Priority:** P0 (Critical - Core gameplay)

---

### 3.5 Military Recruitment System
**Files:** `MilitaryRecruitmentSystem.h`

**Test Plan:**
1. **Recruitment Mechanics Tests**
   - Population pool depletion
   - Recruitment costs
   - Equipment quality effects
   - Supply line mechanics

2. **Integration Tests**
   - Economy integration (costs)
   - Population integration (manpower)
   - Military system integration

**Verification Checklist:**
- [ ] Recruitment depletes population
- [ ] Costs calculated correctly
- [ ] Supply lines work
- [ ] Integration seamless

**Priority:** P1 (High - Military subsystem)

---

### 3.6 Diplomacy System
**Files:** `DiplomacySystem.h`, `DiplomacyComponents.h`, `DiplomacyRepository.h`, `DiplomaticCalculator.h`

**Test Plan:**
1. **Treaty Tests**
   - Alliance creation
   - Non-aggression pact
   - Trade agreement
   - Vassalage treaty
   - Treaty compliance
   - Treaty violations

2. **Diplomatic Action Tests**
   - Alliance proposals
   - War declarations with Casus Belli
   - Marriage diplomacy
   - Embassy establishment
   - Diplomatic gifts
   - Tribute demands

3. **Relationship Tests**
   - Opinion calculation
   - Trust building/erosion
   - Rivalry mechanics
   - Prestige effects

4. **War Tests**
   - War declaration validation
   - Casus Belli requirements
   - Peace negotiations
   - War exhaustion

5. **Handler Tests**
   - AllianceProposalHandler
   - WarDeclarationHandler

**Verification Checklist:**
- [ ] Treaties created and enforced
- [ ] Diplomatic actions work
- [ ] Relationships tracked correctly
- [ ] War mechanics functional
- [ ] Handlers process correctly

**Priority:** P0 (Critical - Core gameplay)

---

### 3.7 Technology System
**Files:** `TechnologySystem.h`, `TechnologyComponents.h`, `TechnologyEffects.h`, `TechnologyEffectApplicator.h`

**Test Plan:**
1. **Research Tests**
   - Research project creation
   - Research progress tracking
   - Research completion
   - Research costs (funding)

2. **Innovation Tests**
   - Technology discovery
   - Knowledge accumulation
   - Innovation tracking

3. **Technology Tree Tests**
   - Prerequisites validation
   - Research paths
   - Technology categories (military, economic, administrative, cultural, naval)

4. **Effect Application Tests**
   - Technology modifiers
   - Effects on other systems
   - Effect stacking

**Verification Checklist:**
- [ ] Research progresses correctly
- [ ] Prerequisites enforced
- [ ] Technologies discovered
- [ ] Effects applied correctly
- [ ] All categories work

**Priority:** P1 (High - Important progression system)

---

### 3.8 Administrative System
**Files:** `AdministrativeSystem.h`, `AdministrativeComponents.h`, `AdministrativeOfficial.h`

**Test Plan:**
1. **Official Tests**
   - Official creation
   - Trait system (efficiency, corruption, loyalty)
   - Efficiency calculation (base 0.7, min 0.1, max 1.0)
   - Trait bonuses

2. **Bureaucracy Tests**
   - Bureaucracy expansion
   - Expansion costs
   - Efficiency scaling
   - Salary costs

3. **Corruption Tests**
   - Corruption rate (5%)
   - Corruption effects
   - Anti-corruption measures

4. **Stability Tests**
   - Realm stability modifiers
   - Official loyalty effects
   - Administrative efficiency impact

**Verification Checklist:**
- [ ] Officials created with traits
- [ ] Efficiency calculated correctly
- [ ] Corruption applied
- [ ] Salaries deducted
- [ ] Stability modifiers work

**Priority:** P1 (High - Important management system)

---

## CATEGORY 4: AI SYSTEMS (5 systems)

### 4.1 AI Director
**Files:** `AIDirector.h`, `AIDirectorCalculator.cpp`

**Test Plan:**
1. **Attention Budget Tests**
   - Budget allocation to nations
   - Budget allocation to characters
   - Dynamic reallocation
   - Priority handling

2. **Message Priority Tests**
   - CRITICAL priority handling
   - HIGH priority handling
   - MEDIUM priority handling
   - LOW priority handling
   - Priority queue ordering

3. **Information Coordination Tests**
   - Information sharing between AI
   - Coordination of AI systems
   - Scheduled processing with delays

4. **Performance Tests**
   - CPU budget compliance
   - Processing time per frame
   - Scalability with many AI entities

**Verification Checklist:**
- [ ] Attention budget allocated correctly
- [ ] Priority queue works
- [ ] Information sharing functional
- [ ] CPU budget respected
- [ ] Scales with entity count

**Priority:** P1 (High - AI coordination)

---

### 4.2 Nation AI
**Files:** `NationAI.h`, `NationAICalculator.cpp`

**Test Plan:**
1. **Strategic Goal Tests**
   - Expansion goal logic
   - Consolidation goal logic
   - Economic goal logic
   - Diplomatic goal logic
   - Cultural goal logic
   - Survival goal logic
   - Technological goal logic
   - Goal priority and switching

2. **War Decision Tests**
   - Cost/benefit analysis
   - Target selection
   - War declaration conditions
   - Peace negotiation logic

3. **Diplomatic Decision Tests**
   - Alliance decisions
   - Trade agreement decisions
   - Denunciation logic
   - Tribute decisions

4. **Economic Decision Tests**
   - Tax rate adjustments
   - Infrastructure investment
   - Trade focus
   - Currency management
   - Resource allocation

5. **Technology Decision Tests**
   - Research priorities
   - Technology path selection

6. **Threat Assessment Tests**
   - Existential threat detection
   - Severe threat detection
   - Moderate threat detection
   - Low threat detection
   - Minimal threat detection
   - Response to threats

**Verification Checklist:**
- [ ] Strategic goals work
- [ ] War decisions reasonable
- [ ] Diplomatic decisions sensible
- [ ] Economic decisions effective
- [ ] Technology choices valid
- [ ] Threat assessment accurate

**Priority:** P0 (Critical - Core AI)

---

### 4.3 Character AI
**Files:** `CharacterAI.h`, `AICalculator.cpp`

**Test Plan:**
1. **Ambition Tests**
   - Gain title ambition
   - Accumulate wealth ambition
   - Gain land ambition
   - Prestige ambition
   - Love ambition
   - Revenge ambition
   - Knowledge ambition
   - Piety ambition
   - Power ambition
   - Legacy ambition

2. **Mood Tests**
   - Content mood effects
   - Happy mood effects
   - Stressed mood effects
   - Angry mood effects
   - Afraid mood effects
   - Ambitious mood effects
   - Desperate mood effects

3. **Plot Tests**
   - Assassination plots
   - Coup plots
   - Blackmail schemes
   - Fabricate claim plots
   - Steal secrets operations
   - Sabotage actions
   - Seduction attempts

4. **Personality Tests**
   - Personality-driven decisions
   - Interaction with traits
   - Long-term behavior consistency

**Verification Checklist:**
- [ ] Ambitions drive behavior
- [ ] Moods affect decisions
- [ ] Plots execute correctly
- [ ] Personality consistent
- [ ] Characters feel alive

**Priority:** P1 (High - Immersion important)

---

### 4.4 AI Attention Manager
**Files:** `AIAttentionManager.h`, `AIAttentionCalculator.cpp`

**Test Plan:**
1. **Attention Profile Tests**
   - Character archetypes (Warrior King, Diplomat, Administrator, Merchant, Scholar)
   - Nation personalities (Expansionist, Diplomatic, Economic, Technological, Religious)
   - Profile-based filtering

2. **Relevance Classification Tests**
   - CRITICAL relevance
   - HIGH relevance
   - MEDIUM relevance
   - LOW relevance
   - IRRELEVANT classification

3. **Distance-Based Tests**
   - Attention falloff with distance
   - Threshold-based filtering
   - Special interest tracking (rivals, allies, watched provinces)

4. **Performance Tests**
   - CPU reduction validation (target: 98%)
   - Information filtering efficiency
   - Scaling with entity count

**Verification Checklist:**
- [ ] Attention profiles work
- [ ] Relevance classified correctly
- [ ] Distance falloff appropriate
- [ ] 98% CPU reduction achieved
- [ ] Special interests tracked

**Priority:** P0 (Critical - Performance essential)

---

### 4.5 Information Propagation System
**Files:** `InformationPropagationSystem.h`

**Test Plan:**
1. **Information Type Tests**
   - Military action propagation
   - Diplomatic change propagation
   - Economic crisis propagation
   - Succession crisis propagation
   - Rebellion propagation
   - Technology propagation
   - Religious event propagation
   - Trade disruption propagation
   - Alliance propagation
   - Natural disaster propagation
   - Plague propagation
   - Cultural shift propagation

2. **Delay Tests**
   - Distance-based delays
   - Diplomatic channel speed boost
   - Delay calculation accuracy

3. **Accuracy Tests**
   - Information accuracy degradation
   - Distance effects on accuracy
   - Time effects on accuracy
   - Rumor mechanics

4. **Player Information Tests**
   - Perfect information for player (configurable)
   - Player information advantage

**Verification Checklist:**
- [ ] All information types propagate
- [ ] Delays realistic
- [ ] Accuracy degrades appropriately
- [ ] Diplomatic channels faster
- [ ] Player info correct

**Priority:** P1 (High - Realism important)

---

## CATEGORY 5: RENDERING & VISUALIZATION SYSTEMS (6 systems)

### 5.1 Map Renderer
**Files:** `MapRenderer.h`, `MapRenderer.cpp`, `ProvinceRenderComponent.h`

**Test Plan:**
1. **LOD Tests**
   - LOD 0 (Strategic) rendering
   - LOD 1 (Regional) rendering
   - LOD 2 (Provincial) rendering
   - LOD 3 (Local) rendering
   - LOD switching smoothness
   - LOD selection accuracy

2. **Viewport Culling Tests**
   - Culling rate (target: 70-90%)
   - No visible pop-in
   - Correct visibility determination

3. **Rendering Mode Tests**
   - Political map mode
   - Terrain map mode
   - Border visibility
   - Feature visibility

4. **Layer Tests**
   - Political borders layer
   - Terrain layer
   - Trade routes layer
   - Units layer
   - Layer compositing

5. **Performance Tests**
   - Frame rate (target: 60 FPS)
   - Draw call count
   - GPU memory usage
   - CPU overhead

**Verification Checklist:**
- [ ] All LOD levels render correctly
- [ ] Culling 70-90% achieved
- [ ] All rendering modes work
- [ ] Layers composite correctly
- [ ] 60 FPS maintained

**Priority:** P1 (High - Visual quality important)

---

### 5.2 Terrain Renderer
**Files:** `TerrainRenderer.h`

**Test Plan:**
1. **Terrain Feature Tests**
   - Mountains rendering
   - Plains rendering
   - Forests rendering
   - Rivers rendering
   - Coastlines rendering

2. **Performance Tests**
   - Rendering speed
   - Memory usage

**Verification Checklist:**
- [ ] All terrain types render
- [ ] Performance acceptable

**Priority:** P2 (Medium - Visual feature)

---

### 5.3 Unit Renderer
**Files:** `UnitRenderer.h`

**Test Plan:**
1. **Unit Visualization Tests**
   - Unit icon rendering
   - Unit position accuracy
   - Unit selection highlighting
   - Unit grouping visualization

2. **Performance Tests**
   - Rendering many units (1000+)
   - Culling effectiveness

**Verification Checklist:**
- [ ] Units render correctly
- [ ] Positions accurate
- [ ] Selection works
- [ ] Handles many units

**Priority:** P2 (Medium - Visual feature)

---

### 5.4 Building Renderer
**Files:** `BuildingRenderer.h`

**Test Plan:**
1. **Building Visualization Tests**
   - Settlement rendering by type
   - Building icon display
   - Building position accuracy

**Verification Checklist:**
- [ ] Buildings render correctly
- [ ] Positions accurate

**Priority:** P3 (Low - Visual polish)

---

### 5.5 Viewport Culler
**Files:** `ViewportCuller.h`

**Test Plan:**
1. **Culling Tests**
   - Bounding box intersection
   - View frustum calculation
   - Culling accuracy (no false positives)
   - Culling rate (target: 70-90%)

2. **Performance Tests**
   - Culling algorithm speed (<1ms)
   - Memory overhead

**Verification Checklist:**
- [ ] Culling accurate
- [ ] 70-90% culling rate
- [ ] Performance <1ms

**Priority:** P1 (High - Performance critical)

---

### 5.6 Tactical Terrain Renderer
**Files:** `TacticalTerrainRenderer.h`

**Test Plan:**
1. **Tactical View Tests**
   - Battle terrain rendering
   - Terrain effects display
   - Unit positioning on terrain

**Verification Checklist:**
- [ ] Tactical terrain renders
- [ ] Terrain effects visible

**Priority:** P2 (Medium - Battle feature)

---

## CATEGORY 6: USER INTERFACE SYSTEMS (13 systems)

### 6.1 Main UI System
**Files:** `UI.h`

**Test Plan:**
1. **UI Coordination Tests**
   - Window management
   - Window switching
   - UI state persistence
   - ImGui integration

**Verification Checklist:**
- [ ] All windows accessible
- [ ] Window switching works
- [ ] UI responsive

**Priority:** P1 (High - User experience)

---

### 6.2 Main Menu UI
**Files:** `MainMenuUI.h`

**Test Plan:**
1. **Menu Tests**
   - New game start
   - Load game
   - Save list display
   - Settings access

**Verification Checklist:**
- [ ] Menu functional
- [ ] New game works
- [ ] Load game works

**Priority:** P1 (High - Entry point)

---

### 6.3 Province Info Window
**Files:** `ProvinceInfoWindow.h`, `SimpleProvincePanel.h`

**Test Plan:**
1. **Province Display Tests**
   - Province details accuracy
   - Population display
   - Economy display
   - Military display
   - Building list

2. **Interaction Tests**
   - Province selection
   - Decision commands
   - Data updates

**Verification Checklist:**
- [ ] Province data accurate
- [ ] All stats displayed
- [ ] Interactions work

**Priority:** P1 (High - Core UI)

---

### 6.4 Population Info Window
**Files:** `PopulationInfoWindow.h`

**Test Plan:**
1. **Population Display Tests**
   - Demographics display
   - Class distribution
   - Employment stats
   - Growth trends

**Verification Checklist:**
- [ ] Population stats accurate
- [ ] Charts/graphs work

**Priority:** P2 (Medium - Info window)

---

### 6.5 Technology Info Window
**Files:** `TechnologyInfoWindow.h`

**Test Plan:**
1. **Technology Display Tests**
   - Research tree display
   - Current research
   - Completed technologies
   - Effects display

**Verification Checklist:**
- [ ] Tech tree accurate
- [ ] Research progress shown
- [ ] Effects displayed

**Priority:** P2 (Medium - Info window)

---

### 6.6 Administrative UI
**Files:** `AdministrativeUI.h`

**Test Plan:**
1. **Administrative Display Tests**
   - Official list
   - Bureaucracy status
   - Reform options
   - Cost display

**Verification Checklist:**
- [ ] Officials shown
- [ ] Bureaucracy accurate
- [ ] Actions work

**Priority:** P2 (Medium - Management UI)

---

### 6.7 Trade System Window
**Files:** `TradeSystemWindow.h`

**Test Plan:**
1. **Trade Display Tests**
   - Route list
   - Hub display
   - Market prices
   - Trade income

**Verification Checklist:**
- [ ] Routes shown
- [ ] Hubs displayed
- [ ] Prices accurate

**Priority:** P2 (Medium - Management UI)

---

### 6.8 Nation Overview Window
**Files:** `NationOverviewWindow.h`

**Test Plan:**
1. **Nation Display Tests**
   - Realm stats
   - Diplomatic relations
   - Prestige display
   - Vassals list

**Verification Checklist:**
- [ ] Nation stats accurate
- [ ] Relations shown
- [ ] All data correct

**Priority:** P1 (High - Important overview)

---

### 6.9 Battle Viewer Window
**Files:** `BattleViewerWindow.h`

**Test Plan:**
1. **Battle Display Tests**
   - Battle visualization
   - Combat results
   - Casualty display

**Verification Checklist:**
- [ ] Battles shown
- [ ] Results accurate

**Priority:** P2 (Medium - Battle feature)

---

### 6.10 Game Control Panel
**Files:** `GameControlPanel.h`

**Test Plan:**
1. **Control Tests**
   - Pause/unpause
   - Speed controls
   - Time scale changes

**Verification Checklist:**
- [ ] Controls work
- [ ] Time changes correctly

**Priority:** P1 (High - Core control)

---

### 6.11 Performance Window
**Files:** `PerformanceWindow.h`

**Test Plan:**
1. **Performance Display Tests**
   - FPS display
   - CPU usage
   - Memory usage
   - System timings

**Verification Checklist:**
- [ ] Metrics accurate
- [ ] Display updates

**Priority:** P3 (Low - Debug tool)

---

### 6.12 Map Mode Selector
**Files:** `MapModeSelector.h`

**Test Plan:**
1. **Map Mode Tests**
   - Mode selection
   - Mode switching
   - Visual changes

**Verification Checklist:**
- [ ] Modes selectable
- [ ] Switching works

**Priority:** P2 (Medium - Map feature)

---

### 6.13 Toast Notifications
**Files:** `Toast.h`

**Test Plan:**
1. **Toast Tests**
   - Notification display
   - Auto-dismiss timing
   - Multiple toasts
   - Priority ordering

**Verification Checklist:**
- [ ] Toasts appear
- [ ] Auto-dismiss works
- [ ] Multiple toasts handled

**Priority:** P3 (Low - Polish)

---

## CATEGORY 7: BRIDGE & INTEGRATION SYSTEMS (4 systems)

### 7.1 Diplomacy-Economic Bridge
**Files:** `DiplomacyEconomicBridge.h`

**Test Plan:**
1. **Trade Agreement Tests**
   - Trade agreement income
   - Trade route effects
   - Economic integration

2. **Sanction Tests**
   - Trade embargo effects
   - Partial embargo
   - Financial sanctions
   - Tariff increases
   - Asset freezes
   - Diplomatic isolation

3. **Tribute Tests**
   - Tribute payment
   - Compensation mechanics

4. **Trade Dispute Tests**
   - Dispute resolution
   - Economic penalties

**Verification Checklist:**
- [ ] Trade agreements work
- [ ] Sanctions effective
- [ ] Tribute mechanics work
- [ ] Disputes resolved

**Priority:** P1 (High - Cross-system integration)

---

### 7.2 Economic-Population Bridge
**Files:** `EconomicPopulationBridge.h`

**Test Plan:**
1. **Tax Effect Tests**
   - Tax rate on happiness
   - Revenue vs happiness balance

2. **Employment Tests**
   - Employment effects on economy
   - Wage calculations
   - Unemployment costs

3. **Wealth Inequality Tests**
   - Inequality tracking
   - Social unrest from inequality

4. **Infrastructure Tests**
   - Infrastructure quality effects
   - Public investment impacts

5. **Trade Per Capita Tests**
   - Trade income distribution
   - Economic growth effects

6. **Inflation Tests**
   - Inflation on population
   - Real wage calculations

**Verification Checklist:**
- [ ] Tax effects work
- [ ] Employment integrated
- [ ] Inequality tracked
- [ ] Infrastructure effects visible
- [ ] Inflation impacts correct

**Priority:** P1 (High - Cross-system integration)

---

### 7.3 Military-Economic Bridge
**Files:** `MilitaryEconomicBridge.h`

**Test Plan:**
1. **Military Cost Tests**
   - Maintenance costs
   - Recruitment costs
   - Equipment costs
   - Fortification costs

2. **War Economic Effect Tests**
   - Trade disruption
   - Infrastructure damage
   - War exhaustion penalties

3. **Military Income Tests**
   - Loot mechanics
   - Tribute from wars
   - Conquest territory value

4. **Employment Tests**
   - Military employment effects

**Verification Checklist:**
- [ ] Costs applied correctly
- [ ] War affects economy
- [ ] Military income works
- [ ] Employment integrated

**Priority:** P1 (High - Cross-system integration)

---

### 7.4 Technology-Economic Bridge
**Files:** `TechnologyEconomicBridge.h`

**Test Plan:**
1. **Research Funding Tests**
   - Research costs
   - Funding allocation
   - Budget constraints

2. **Economic Bonus Tests**
   - Technology bonuses
   - Production efficiency
   - Trade efficiency

**Verification Checklist:**
- [ ] Research funding works
- [ ] Tech bonuses applied

**Priority:** P2 (Medium - Cross-system integration)

---

## CATEGORY 8: SUPPORT SYSTEMS (3 systems)

### 8.1 Scenario System
**Files:** `ScenarioSystem.h`

**Test Plan:**
1. **Scenario Loading Tests**
   - Load scenario from config
   - Event timing
   - Event triggers

2. **Event Execution Tests**
   - All event types execute
   - Effects applied correctly
   - Event chaining

3. **Completion Tests**
   - Completion conditions
   - Victory/defeat detection

**Verification Checklist:**
- [ ] Scenarios load
- [ ] Events trigger
- [ ] Effects work
- [ ] Completion detected

**Priority:** P2 (Medium - Scenario feature)

---

### 8.2 Definition Loader
**Files:** `DefinitionLoader.h`

**Test Plan:**
1. **Data Loading Tests**
   - Resources loading
   - Buildings loading
   - Technologies loading
   - Units loading
   - Scenarios loading
   - Nations loading
   - Cultures loading
   - Provinces loading

2. **Validation Tests**
   - JSON validation
   - Data integrity
   - Missing data handling

**Verification Checklist:**
- [ ] All data types load
- [ ] Validation works
- [ ] Errors handled

**Priority:** P1 (High - Data foundation)

---

### 8.3 Random Number Generator
**Files:** `RandomGenerator.h`

**Test Plan:**
1. **RNG Tests**
   - Seeded generation
   - Deterministic output
   - Distribution uniformity
   - Range validation

**Verification Checklist:**
- [ ] Seeding works
- [ ] Deterministic
- [ ] Distribution uniform

**Priority:** P2 (Medium - Core utility)

---

## TESTING EXECUTION PLAN (UPDATED)

### Phase 0: Pre-Testing Setup (Week 0) **[CRITICAL]**
**Goals:** Establish testing infrastructure and baseline metrics

**Tasks:**
1. **Test Infrastructure Setup**
   - [ ] Identify existing test framework (Google Test, Catch2, etc.)
   - [ ] If none exists, integrate test framework
   - [ ] Set up test directory structure (`tests/unit/`, `tests/integration/`, `tests/regression/`)
   - [ ] Configure CMake for testing
   - [ ] Set up sanitizers (ThreadSanitizer, AddressSanitizer)

2. **Baseline Profiling**
   - [ ] Run game and profile current performance (perf, gprof)
   - [ ] Measure current FPS, frame time, CPU usage
   - [ ] Profile memory usage (Valgrind Massif)
   - [ ] Document baseline metrics
   - [ ] Identify current bottlenecks

3. **Test Data Creation**
   - [ ] Create test data fixtures for each system
   - [ ] Set up test scenarios (small map, large map, etc.)
   - [ ] Configure RNG seeding for deterministic tests
   - [ ] Create test save files

4. **Metrics Dashboard**
   - [ ] Set up performance tracking
   - [ ] Create metrics log format
   - [ ] Document success criteria per system

**Deliverables:**
- Working test infrastructure
- Baseline metrics document
- Test data repository
- Testing guidelines document

---

### Phase 1: Foundation Systems (Week 1-2) **[EXTENDED]**
**Recommended Order:** Config → Types → Logging → ECS → Threading → Save

1. **Configuration System** (Day 1-2)
   - Simple system for learning test process
   - Quick win to build confidence

2. **Type System** (Day 3)
   - Type safety validation
   - Type registry testing

3. **Logging System** (Day 4)
   - Log level testing
   - Performance validation

4. **Entity Component System** (Day 5-7)
   - Entity management
   - Component operations
   - Message bus
   - **Thread safety deep dive**

5. **Threading System** (Day 8-10)
   - Threading strategies
   - Frame synchronization
   - **Thread safety stress tests**
   - Lock contention measurement

6. **Save System** (Day 11-14)
   - Full save/load
   - Incremental saves
   - Compression
   - Recovery mechanisms

**Additional Testing:**
- Performance benchmarks for each system
- Thread safety validation with ThreadSanitizer
- Memory leak detection with Valgrind

---

### Phase 2: Entity Systems (Week 3)
1. Time Management
2. Province System
3. Realm System
4. Map System
5. Spatial Index

---

### Phase 3: Primary Game Systems (Week 4-6)
1. Population
2. Economy
3. Trade
4. Military
5. Diplomacy
6. Technology
7. Administrative

---

### Phase 4: AI Systems (Week 7)
1. AI Attention Manager (start here - performance critical)
2. Information Propagation
3. AI Director
4. Nation AI
5. Character AI

---

### Phase 5: Rendering & UI (Week 8)
1. Viewport Culler (performance critical)
2. Map Renderer
3. Terrain/Unit/Building Renderers
4. All UI Windows

---

### Phase 6: Bridge Systems (Week 9)
1. Diplomacy-Economic
2. Economic-Population
3. Military-Economic
4. Technology-Economic

---

### Phase 7: Support Systems (Week 9)
1. Scenario System
2. Definition Loader
3. RNG

---

### Phase 8: Integration Testing (Week 10-11) **[NEW - CRITICAL]**

**Purpose:** Test system interactions and full game loop

**Integration Test Scenarios:**

1. **Full Game Loop Test**
   ```
   1. Initialize all systems
   2. Run 100 in-game years at max speed
   3. Verify state consistency
   4. No crashes, no memory leaks
   5. Performance stable (no degradation)
   ```

2. **Save/Load Stress Test**
   ```
   1. Start game
   2. Run 10 years → Save
   3. Run 10 years → Save
   4. Run 10 years → Save
   5. Load middle save
   6. Run 10 years
   7. Verify state correct
   ```

3. **War Cascade Test**
   ```
   1. Declare war between 2 nations
   2. Trigger alliance chains
   3. Verify: Economy affected
   4. Verify: Population affected
   5. Verify: Diplomacy updated
   6. Verify: Military systems working
   7. Resolve war
   8. Verify recovery
   ```

4. **Economic Collapse Test**
   ```
   1. Trigger economic crisis
   2. Verify: Population happiness drops
   3. Verify: Trade disrupted
   4. Verify: Military funding reduced
   5. Verify: Rebellion risk increases
   6. Test recovery mechanisms
   ```

5. **AI Behavior Test**
   ```
   1. Set up 50 nations with AI
   2. Run 100 years
   3. Verify: Nations make decisions
   4. Verify: Wars happen
   5. Verify: Diplomacy active
   6. Verify: Economic development
   7. Check AI appears intelligent
   ```

6. **Long-Running Stability Test**
   ```
   1. Run game for 10+ real-time hours
   2. Monitor: Memory usage (no leaks)
   3. Monitor: CPU usage (stable)
   4. Monitor: Frame time (stable)
   5. Verify: No crashes
   6. Verify: Performance maintained
   ```

**Verification:**
- [ ] All integration scenarios pass
- [ ] No system interaction bugs
- [ ] Performance stable over time
- [ ] Save/load 100% reliable

---

### Phase 9: Regression & Polish (Week 12)
1. **Regression Suite**
   - Run all regression tests
   - Fix any newly discovered bugs
   - Add tests for all fixed bugs

2. **Performance Tuning**
   - Optimize hotspots identified during testing
   - Validate performance targets met
   - Run load tests (1000+ provinces, 100+ nations)

3. **Code Quality**
   - Run static analysis (clang-tidy, cppcheck)
   - Fix compiler warnings
   - Code cleanup

4. **Documentation**
   - Update documentation with findings
   - Create troubleshooting guide
   - Document known limitations

---

### Phase 10: Final Validation & Playtest (Week 13)
1. **Final Verification**
   - All automated tests passing
   - All P0 bugs fixed
   - All P1 bugs fixed or documented
   - Performance targets met

2. **Manual Playtest**
   - Play game for multiple hours
   - Test all major features
   - Validate game balance
   - Check user experience

3. **Acceptance Criteria**
   - [ ] 100% automated tests passing
   - [ ] Code coverage >80% for core systems
   - [ ] ThreadSanitizer clean
   - [ ] Valgrind clean
   - [ ] 60 FPS sustained
   - [ ] <2GB RAM usage
   - [ ] 10+ hour stability test passed
   - [ ] Save/load 100% reliable
   - [ ] Game enjoyable to play

**Timeline:** 13 weeks total (vs. original 8 weeks)

---

## TOOLS & AUTOMATION

### Testing Tools
- **Google Test / Catch2:** Unit test framework (TBD - check existing)
- **Valgrind:** Memory leak detection
- **Valgrind Massif:** Heap profiling
- **ThreadSanitizer (TSan):** Race condition detection
- **AddressSanitizer (ASan):** Memory safety
- **UndefinedBehaviorSanitizer (UBSan):** Undefined behavior detection
- **gdb:** Interactive debugging
- **perf:** Performance profiling (Linux)
- **gprof:** Profiling
- **gcov/lcov:** Code coverage
- **clang-tidy:** Static analysis
- **cppcheck:** Additional static analysis
- **Google Benchmark:** Performance benchmarking (optional)

### Build Commands
```bash
# Clean debug build
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Debug build with sanitizers (ThreadSanitizer)
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" ..
make -j$(nproc)
./game  # Run to detect data races

# Debug build with AddressSanitizer + UBSan
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g" ..
make -j$(nproc)
./game  # Run to detect memory errors

# Run unit tests (if framework exists)
./build/tests/unit_tests
# OR
ctest --output-on-failure

# Memory leak check
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./build/game

# Memory profiling
valgrind --tool=massif ./build/game
ms_print massif.out.<pid>

# CPU profiling with perf
perf record -g ./build/game
perf report

# Code coverage
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage" ..
make -j$(nproc)
./build/tests/unit_tests  # Run tests
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_report
# Open coverage_report/index.html

# Static analysis
clang-tidy src/**/*.cpp -- -Iinclude
cppcheck --enable=all --suppress=missingIncludeSystem src/

# Performance benchmarking (if using Google Benchmark)
./build/benchmarks/benchmark_runner --benchmark_filter=ECS.*
```

### Test Data Setup
```bash
# Create test data directory
mkdir -p tests/data
mkdir -p tests/fixtures
mkdir -p tests/scenarios

# Generate test save files
./build/game --generate-test-data

# Create small test map (100 provinces)
# Create medium test map (500 provinces)
# Create large test map (2000 provinces)
```

### Continuous Testing Script
```bash
#!/bin/bash
# test_all.sh - Run full test suite

set -e  # Exit on error

echo "=== Building with sanitizers ==="
mkdir -p build_test && cd build_test
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" ..
make -j$(nproc)

echo "=== Running unit tests ==="
./tests/unit_tests

echo "=== Running ThreadSanitizer ==="
./game --test-mode --max-frames=1000

echo "=== Running Valgrind ==="
cd ..
mkdir -p build_valgrind && cd build_valgrind
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
valgrind --leak-check=full --error-exitcode=1 ./game --test-mode --max-frames=100

echo "=== All tests passed! ==="
```

---

## SUCCESS CRITERIA

### Per-System Criteria
- [ ] All tests pass (100%)
- [ ] No memory leaks (Valgrind clean)
- [ ] No data races (ThreadSanitizer clean)
- [ ] Code coverage >80%
- [ ] Performance targets met
- [ ] Integration tests pass

### Overall Project Criteria
- [ ] All 50+ systems verified
- [ ] No critical (P0) bugs remaining
- [ ] <5 high (P1) bugs remaining
- [ ] Game stable for 1+ hour continuous play
- [ ] Save/load cycle works flawlessly
- [ ] Performance: 60 FPS sustained
- [ ] Memory: <2GB RAM usage
- [ ] CPU: <70% single-core usage

---

## ISSUE TRACKING

### Issue Categories
1. **Crash:** Application crash/segfault
2. **Data Corruption:** Save data or game state corruption
3. **Logic Error:** Incorrect game behavior
4. **Performance:** Performance below targets
5. **Memory Leak:** Memory not freed
6. **Race Condition:** Thread safety violation
7. **UI Bug:** User interface issue
8. **Visual Bug:** Rendering issue

### Issue Template
```
**System:** [System name]
**Category:** [Crash/Data/Logic/Performance/Memory/Race/UI/Visual]
**Priority:** [P0/P1/P2/P3]
**Description:** [What's wrong]
**Reproduction:** [Steps to reproduce]
**Expected:** [Expected behavior]
**Actual:** [Actual behavior]
**Fix:** [Proposed fix]
**Status:** [Open/In Progress/Fixed/Verified]
```

---

**End of Testing Plan**
