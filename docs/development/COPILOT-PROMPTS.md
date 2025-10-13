# GitHub Copilot Integration Prompts

> **Purpose**: Standardized prompts for triggering systematic debugging workflows with GitHub Copilot.

## 🚀 **System Integration Workflows**

### **Complete Integration (Recommended)**
```
Follow SYSTEM-INTEGRATION-WORKFLOW.md to debug and integrate [SYSTEM_NAME]
```

**Examples:**
- `Follow SYSTEM-INTEGRATION-WORKFLOW.md to debug and integrate MilitarySystem`
- `Follow SYSTEM-INTEGRATION-WORKFLOW.md to debug and integrate EconomicSystem`  
- `Follow SYSTEM-INTEGRATION-WORKFLOW.md to debug and integrate TechnologySystem`

**What it does:** Executes complete 8-phase workflow (Pre-Integration + Phases 1-5 Integration + Phases 6-8 Cleanup + Documentation)

---

### **Quick Reference Approach**
```
Use INTEGRATION-QUICK-CHECKLIST.md to fix [SYSTEM_NAME] - run all phases
```

**Examples:**
- `Use INTEGRATION-QUICK-CHECKLIST.md to fix MilitarySystem - run all phases`
- `Use INTEGRATION-QUICK-CHECKLIST.md to fix EconomicSystem - run all phases`

**What it does:** Uses rapid reference checklist with time budgets and copy-paste commands

---

### **Partial Workflow Execution**
```
Follow SYSTEM-INTEGRATION-WORKFLOW.md starting from Phase [X] for [SYSTEM_NAME]
```

**Examples:**
- `Follow SYSTEM-INTEGRATION-WORKFLOW.md starting from Phase 3 for MilitarySystem`
- `Follow SYSTEM-INTEGRATION-WORKFLOW.md starting from cleanup phases for EconomicSystem`

**What it does:** Executes workflow from specific phase (useful for resuming interrupted work)

---

### **Emergency/Broken System Recovery**
```
System [NAME] is broken - follow complete SYSTEM-INTEGRATION-WORKFLOW.md from pre-integration phase
```

**Examples:**
- `System MilitarySystem is broken - follow complete SYSTEM-INTEGRATION-WORKFLOW.md from pre-integration phase`
- `System EconomicSystem is broken - follow complete SYSTEM-INTEGRATION-WORKFLOW.md from pre-integration phase`

**What it does:** Full diagnostic and recovery workflow starting from architecture validation

---

## 🧹 **Cleanup-Only Operations**

### **Post-Integration Cleanup**
```
Follow CLEANUP-PROCEDURES.md to eliminate redundancy in [SYSTEM_NAME]
```

**Examples:**
- `Follow CLEANUP-PROCEDURES.md to eliminate redundancy in PopulationSystem`
- `Follow CLEANUP-PROCEDURES.md to eliminate redundancy in AdministrativeSystem`

**What it does:** Executes only cleanup phases (6-8) for systems already integrated

---

## 🔍 **Diagnostic and Analysis**

### **System Status Analysis**
```
Analyze current status of [SYSTEM_NAME] using PROJECT-STATUS.md and ARCHITECTURAL-CHECKLIST.md
```

**Examples:**
- `Analyze current status of MilitarySystem using PROJECT-STATUS.md and ARCHITECTURAL-CHECKLIST.md`
- `Analyze current status of EconomicSystem using PROJECT-STATUS.md and ARCHITECTURAL-CHECKLIST.md`

**What it does:** Comprehensive system health check without making changes

---

### **Architecture Compliance Check**
```
Verify [SYSTEM_NAME] compliance with ARCHITECTURAL-CHECKLIST.md standards
```

**Examples:**
- `Verify MilitarySystem compliance with ARCHITECTURAL-CHECKLIST.md standards`
- `Verify EconomicSystem compliance with ARCHITECTURAL-CHECKLIST.md standards`

**What it does:** Checks architectural patterns, ECS integration, and coding standards

---

## 🎯 **Build and Testing**

### **Clean Build Verification**
```
Verify clean build for [SYSTEM_NAME] following DEBUGGING-METHODOLOGY.md
```

**Examples:**
- `Verify clean build for MilitarySystem following DEBUGGING-METHODOLOGY.md`
- `Verify clean build for all systems following DEBUGGING-METHODOLOGY.md`

**What it does:** Build testing with systematic error analysis

---

## 📚 **Documentation Updates**

### **Status Documentation Update**
```
Update PROJECT-STATUS.md with [SYSTEM_NAME] integration results
```

**Examples:**
- `Update PROJECT-STATUS.md with MilitarySystem integration results`
- `Update PROJECT-STATUS.md with EconomicSystem integration results`

**What it does:** Updates project documentation with integration metrics and status

---

## ⚡ **Currently Ready Systems (December 2024)**

Based on PROJECT-STATUS.md, these systems are ready for integration:

| System | Status | Recommended Prompt |
|--------|--------|-------------------|
| **MilitarySystem** | Partially Fixed | `Follow SYSTEM-INTEGRATION-WORKFLOW.md to debug and integrate MilitarySystem` |
| **EconomicSystem** | Assessment Needed | `System EconomicSystem is broken - follow complete SYSTEM-INTEGRATION-WORKFLOW.md from pre-integration phase` |
| **TechnologySystem** | Assessment Needed | `System TechnologySystem is broken - follow complete SYSTEM-INTEGRATION-WORKFLOW.md from pre-integration phase` |
| **TradeSystem** | Assessment Needed | `System TradeSystem is broken - follow complete SYSTEM-INTEGRATION-WORKFLOW.md from pre-integration phase` |
| **DiplomacySystem** | Assessment Needed | `System DiplomacySystem is broken - follow complete SYSTEM-INTEGRATION-WORKFLOW.md from pre-integration phase` |

---

## 🔗 **Related Documentation**

- **SYSTEM-INTEGRATION-WORKFLOW.md** - Complete 8-phase methodology
- **INTEGRATION-QUICK-CHECKLIST.md** - Rapid reference with commands
- **CLEANUP-PROCEDURES.md** - Systematic redundancy elimination
- **DEBUGGING-METHODOLOGY.md** - Error analysis and resolution
- **ARCHITECTURAL-CHECKLIST.md** - Standards and patterns
- **PROJECT-STATUS.md** - Current system integration status

---

*Save this file as reference for consistent Copilot interactions. Use exact prompt formats for optimal results.*