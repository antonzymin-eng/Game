# System Integration & Debug Workflow

> **Purpose**: Complete step-by-step workflow consolidating all procedures from project documentation into a single actionable checklist.

## 🤖 **How to Use with GitHub Copilot**

### **Standard Copilot Prompts:**

**For Complete Integration:**
```
Follow SYSTEM-INTEGRATION-WORKFLOW.md to debug and integrate [SYSTEM_NAME]
```

**For Specific Systems:**
```
Follow SYSTEM-INTEGRATION-WORKFLOW.md to debug and integrate MilitarySystem
Follow SYSTEM-INTEGRATION-WORKFLOW.md to debug and integrate EconomicSystem
Follow SYSTEM-INTEGRATION-WORKFLOW.md to debug and integrate TechnologySystem
```

**For Quick Reference Approach:**
```
Use INTEGRATION-QUICK-CHECKLIST.md to fix [SYSTEM_NAME] - run all phases
```

**For Partial Workflow:**
```
Follow SYSTEM-INTEGRATION-WORKFLOW.md starting from Phase [X] for [SYSTEM_NAME]
```

**For Emergency/Broken Systems:**
```
System [NAME] is broken - follow complete SYSTEM-INTEGRATION-WORKFLOW.md from pre-integration phase
```

### **What Happens When You Use These Prompts:**

When you provide any of these standardized prompts, GitHub Copilot will:

1. **📋 Load Workflow** - Reference this complete methodology
2. **🔍 Execute Pre-Integration** - System consultation, validation, baseline creation
3. **⚙️ Run Integration Phases 1-5** - Analysis → Components → System → Methods → Testing
4. **🧹 Execute Cleanup Phases 6-8** - Redundancy detection → Analysis → Safe removal
5. **📝 Complete Documentation** - Update PROJECT-STATUS.md and integration metrics

Each phase includes success criteria verification, risk mitigation checks, progress reporting, and rollback procedures if needed.

---

## 🎯 **Pre-Integration Phase** ✅ **MANDATORY**

### **Step 1: Architecture Database Consultation** 
*(Ref: ARCHITECTURAL-CHECKLIST.md)*

```bash
# Read relevant sections BEFORE starting work
less ARCHITECTURE-DATABASE.md | grep -A 10 -B 5 "[SystemName]"
less ARCHITECTURAL-CHECKLIST.md
less PROJECT-STATUS.md | grep -A 20 "Integration Progress"
```

**Checklist:**
- [ ] Read system-specific architecture documentation
- [ ] Check ECS integration patterns from successful systems
- [ ] Review system dependencies matrix
- [ ] Understand current build state

### **Step 2: ECS Architecture Validation**
*(Ref: ARCHITECTURAL-CHECKLIST.md, DEBUGGING-METHODOLOGY.md)*

```bash
# Verify ECS foundation status
grep -r "game::core::Component" include/core/ECS/
grep -r "::core::ecs::EntityManager" include/core/ECS/
grep -r "ComponentAccessManager" include/core/ECS/
```

**Checklist:**
- [ ] ECS foundation is operational
- [ ] No architectural conflicts remain
- [ ] Namespace standards are clear (`::core::ecs` vs `game::core`)
- [ ] Working system templates available (Population, Administrative)

### **Step 3: Build Baseline Establishment**
*(Ref: DEBUGGING-METHODOLOGY.md)*

```bash
# Establish clean build baseline
cd build && make clean && make -j$(nproc)
# Expected: [100%] Built target mechanica_imperii

# Document current state
grep -A 20 "set([SYSTEM]_SOURCES" CMakeLists.txt
```

**Checklist:**
- [ ] Clean build successful before any changes
- [ ] Current system status documented
- [ ] CMakeLists.txt configuration understood

---

## 🔧 **System Integration Phase** (5-Phase Methodology)

### **Phase 1: Architecture Analysis**
*(Ref: DEBUGGING-METHODOLOGY.md Phase 1)*

```bash
# Check for architectural conflicts
grep -A 5 "class [SystemName]" include/game/[system]/[SystemName].h
grep -A 5 "class [SystemName]" src/game/[system]/[SystemName].cpp

# Namespace consistency validation
grep -r "namespace.*[system]" include/game/[system]/
grep -r "using namespace\|namespace.*=" include/game/[system]/

# Include path verification
grep -r "#include.*[system]" src/game/[system]/
```

**Checklist:**
- [ ] No header/implementation mismatches found
- [ ] Namespace consistency validated
- [ ] Include paths follow project standards
- [ ] Dependencies identified and available

### **Phase 2: ECS Component Creation**
*(Ref: DEBUGGING-METHODOLOGY.md Phase 2)*

```bash
# Verify component inheritance pattern
grep -A 5 "struct.*Component.*public.*game::core::Component" include/game/[system]/

# Check component method declarations
grep -A 3 "GetComponentTypeName" include/game/[system]/
```

**Component Template Verification:**
```cpp
// CORRECT pattern (use this):
struct [System]Component : public game::core::Component<[System]Component> {
    // Component data members
    
    std::string GetComponentTypeName() const override {
        return "[System]Component";
    }
};
```

**Checklist:**
- [ ] Components inherit from `game::core::Component<T>`
- [ ] GetComponentTypeName() implemented
- [ ] Component data matches existing system structure
- [ ] All required components defined

### **Phase 3: System Class ECS Integration**
*(Ref: DEBUGGING-METHODOLOGY.md Phase 3)*

```bash
# Verify system inherits from ISystem
grep -A 5 "class.*System.*public.*game::core::ISystem" include/game/[system]/

# Check constructor signature
grep -A 3 "[System]System.*ComponentAccessManager.*MessageBus" include/game/[system]/
```

**System Template Verification:**
```cpp
// CORRECT pattern (use this):
class [System]System : public game::core::ISystem {
public:
    explicit [System]System(::core::ecs::ComponentAccessManager& access_manager,
                           ::core::ecs::MessageBus& message_bus);
    virtual ~[System]System() = default;

    // ISystem interface
    void Initialize() override;
    void Update(float delta_time) override;
    void Shutdown() override;
};
```

**Checklist:**
- [ ] System inherits from `game::core::ISystem`
- [ ] Proper constructor with ComponentAccessManager and MessageBus
- [ ] Initialize, Update, Shutdown methods declared
- [ ] Virtual destructor present

### **Phase 4: System Method Integration**
*(Ref: DEBUGGING-METHODOLOGY.md Phase 4)*

```bash
# Check for stub vs real implementations
grep -A 10 -B 2 "LogInfo.*stub\|TODO\|FIXME" src/game/[system]/

# Verify ECS API usage
grep -r "entity_manager->.*Component<" src/game/[system]/
grep -r "AddComponent<\|GetComponent<" src/game/[system]/
```

**Implementation Verification:**
```cpp
// REPLACE stubs like this:
void SystemMethod() {
    LogInfo("System", "Method called - stub implementation");  // ❌ STUB
}

// WITH real ECS integration:
void SystemMethod() {
    auto* entity_manager = m_access_manager.GetEntityManager();
    auto component = entity_manager->GetComponent<SystemComponent>(entity_handle);
    if (!component) {
        component = entity_manager->AddComponent<SystemComponent>(entity_handle);
    }
    // Real component manipulation
}
```

**Checklist:**
- [ ] All stub implementations replaced with real ECS calls
- [ ] EntityManager properly accessed from ComponentAccessManager
- [ ] Components created and retrieved correctly
- [ ] Data persistence verified

### **Phase 5: Integration Testing & Validation**
*(Ref: DEBUGGING-METHODOLOGY.md Phase 5)*

```bash
# Build test
cd build && make clean && make -j$(nproc)
# Expected: [100%] Built target mechanica_imperii

# Component compilation test
grep -A 10 -B 5 "Building.*[system].*\.cpp\.o" build_output.log
```

**Test Implementation Template:**
```cpp
// Create simple integration test
void test_[system]_components() {
    auto entity_manager = std::make_unique<::core::ecs::EntityManager>();
    auto entity_handle = entity_manager->CreateEntity();
    
    // Test component creation
    auto component = entity_manager->AddComponent<[System]Component>(entity_handle);
    assert(component != nullptr);
    
    // Test component retrieval
    auto retrieved = entity_manager->GetComponent<[System]Component>(entity_handle);
    assert(retrieved == component);
    
    std::cout << "✅ [System] ECS integration test passed" << std::endl;
}
```

**Checklist:**
- [ ] Clean compilation successful
- [ ] Integration test created and passed
- [ ] Component creation/retrieval verified
- [ ] No compilation warnings
- [ ] Template established for future systems

---

## 🧹 **Post-Integration Cleanup Phase** ✅ **MANDATORY**
*(Ref: CLEANUP-PROCEDURES.md)*

### **Phase 6: File Redundancy Detection**

```bash
# Step 1: Inventory system files
ls -la src/game/[system]/*.cpp
wc -l src/game/[system]/*.cpp
ls -lt src/game/[system]/*.cpp

# Step 2: Find suspect patterns
find src/game/[system]/ -name "*_broken*" -o -name "*_old*" \
    -o -name "*_simplified*" -o -name "*_temp*" -o -name "*_backup*"

# Step 3: Check build system usage
grep -A 20 "set([SYSTEM]_SOURCES" CMakeLists.txt
cd build && make clean && make -j$(nproc) 2>&1 | grep -E "[system].*\.cpp\.o"
```

**Checklist:**
- [ ] All system files inventoried
- [ ] Suspect duplicate files identified
- [ ] Build system file usage verified
- [ ] File sizes and dates compared

### **Phase 7: Content Analysis**

```bash
# Function overlap detection
grep -r "Initialize\|Update\|Shutdown\|Process.*\|Calculate.*" src/game/[system]/

# Implementation comparison
diff -u file1.cpp file2.cpp | head -50

# Architecture pattern validation
grep -r "EntityManager\|Component<\|game::core::" src/game/[system]/
grep -r "GetWriteAccess\|old_pattern" src/game/[system]/  # Legacy patterns
```

**Decision Matrix Application:**
| Pattern Found | Action | Command |
|---------------|--------|---------|
| `System.cpp` (complete, ECS) | ✅ Keep | `# No action needed` |
| `System_simplified.cpp` | ❌ Remove | `cp file.cpp file.cpp.backup && rm file.cpp` |
| `System_broken.cpp` | ❌ Remove | `cp file.cpp file.cpp.backup && rm file.cpp` |
| Legacy patterns | ❌ Remove | `cp file.cpp file.cpp.backup && rm file.cpp` |

**Checklist:**
- [ ] Function overlaps identified
- [ ] Implementation quality compared
- [ ] ECS vs legacy patterns distinguished
- [ ] Cleanup decisions made using matrix

### **Phase 8: Safe Cleanup Execution**

```bash
# Always backup first
for file in list_of_files_to_remove; do
    cp "$file" "$file.backup"
done

# Remove incrementally with build verification
for file in list_of_files_to_remove; do
    echo "Removing $file"
    rm "$file"
    
    cd build
    make clean && make -j$(nproc)
    
    if [ $? -ne 0 ]; then
        echo "❌ Build failed after removing $file"
        cd ..
        cp "$file.backup" "$file"
        echo "⚠️  Restored $file - investigate before retry"
        break
    else
        echo "✅ Build successful after removing $file"
    fi
    cd ..
done
```

**Checklist:**
- [ ] All files backed up before removal
- [ ] Incremental removal with build verification
- [ ] Build success maintained throughout process
- [ ] Failed removals properly restored

---

## ✅ **Validation & Documentation Phase**

### **Final Verification**
*(Ref: CLEANUP-PROCEDURES.md Phase 5)*

```bash
# Build verification
cd build && make clean && make -j$(nproc) 2>&1 | tail -5
# Expected: [100%] Built target mechanica_imperii

# Check for remaining duplicates
grep -r "duplicate_function_name" src/game/[system]/

# Verify ECS pattern consistency
grep -r "Component<\|EntityManager" src/game/[system]/

# Count final files
ls -1 src/game/[system]/*.cpp | wc -l
```

**Success Criteria Checklist:**
- [ ] Clean build: `[100%] Built target mechanica_imperii`
- [ ] No duplicate function implementations
- [ ] File count reduced without functionality loss
- [ ] ECS architecture patterns consistent
- [ ] Clear separation of concerns
- [ ] No compilation warnings

### **Documentation Updates**
*(Ref: PROJECT-STATUS.md)*

```bash
# Update PROJECT-STATUS.md
cat >> PROJECT-STATUS.md << EOF

### **[System] Integration Complete** ✅ ($(date +"%B %d, %Y"))
- **Status**: Full ECS integration complete
- **Files**: [count] implementation files
- **Cleanup**: [X] redundant files removed ([Y] lines eliminated)
- **Components**: [List of components created]
- **Integration Test**: ✅ All tests passed
- **Build Status**: Clean compilation successful

EOF
```

**Documentation Checklist:**
- [ ] PROJECT-STATUS.md updated with integration results
- [ ] ARCHITECTURE-DATABASE.md updated with new system API
- [ ] Cleanup results documented with metrics
- [ ] Success template available for future integrations

---

## 🚨 **Emergency Procedures**

### **If Build Breaks During Integration**
```bash
# Restore from backup
git checkout -- [modified_files]
# OR manually restore backups
cp file.cpp.backup file.cpp

# Verify restoration
cd build && make clean && make -j$(nproc)
```

### **If Integration Fails**
```bash
# Document failure state
echo "Integration failed at [phase]: [error_description]" >> integration_log.txt

# Consult documentation
less DEBUGGING-METHODOLOGY.md | grep -A 20 "Common Anti-Patterns"
less ARCHITECTURAL-INCONSISTENCIES.md

# Seek patterns from successful integrations
grep -A 30 "Integration Success" PROJECT-STATUS.md
```

### **Common Error Recovery**
- **Namespace issues**: Check ARCHITECTURAL-CHECKLIST.md namespace standards
- **Include path errors**: Verify against working system examples
- **ECS API errors**: Compare with Population/Administrative system patterns
- **Build system issues**: Verify CMakeLists.txt configuration

---

## 📊 **Workflow Success Metrics**

**Integration Phase Success:**
- ✅ System inherits from `game::core::ISystem`
- ✅ Components inherit from `game::core::Component<T>`
- ✅ Integration test passes
- ✅ Clean build maintained

**Cleanup Phase Success:**
- ✅ Redundant files identified and removed
- ✅ Build integrity maintained throughout
- ✅ Function duplication eliminated
- ✅ Architecture consistency validated

**Documentation Success:**
- ✅ PROJECT-STATUS.md updated
- ✅ Integration results documented
- ✅ Template available for future systems
- ✅ Cleanup metrics recorded

---

## 🎯 **Estimated Timeline**

- **Pre-Integration Phase**: 30-60 minutes
- **Integration Phase (5 phases)**: 4-6 hours
- **Cleanup Phase**: 30-60 minutes  
- **Documentation Phase**: 15-30 minutes
- **Total**: 5.5-8 hours per system

---

*This workflow consolidates procedures from: ARCHITECTURAL-CHECKLIST.md, DEBUGGING-METHODOLOGY.md, CLEANUP-PROCEDURES.md, PROJECT-STATUS.md*