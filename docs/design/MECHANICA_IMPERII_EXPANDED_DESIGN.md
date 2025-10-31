# Mechanica Imperii — Expanded Design Document

**Document Status:** Original Design Document (Historical)
**Date:** Original (Pre-Implementation)
**Purpose:** Comprehensive feature list and planned extensions for all game systems

This version integrates the original scope with additional planned extensions, structured under each major system heading.

---

## 1. Engine & Runtime

**Core Features:**
- Stable SDL2 + OpenGL runtime with ImGui UI.
- Save/load, simulation ticks (hourly/daily).
- Main menu, HUD panels, pause/resume, speed controls.

**Planned Extensions:**
- **Timeline Replay:** a 'history book' panel to review decades of gameplay.
- **Advisor Reports:** daily/weekly summaries written in the tone of councilors.
- **Dynamic Tooltips:** show causal chains of events (e.g., famine → unrest).

---

## 2. Geography & World

**Core Features:**
- High-res Europe map (4901×4251).
- **Terrain grid at 1m zoom, stylized map at higher zoom levels.**
- Weather, fog of war, line of sight mechanics.

**Planned Extensions:**
- **Natural Disasters:** earthquakes, floods, volcanic eruptions.
- **Climate Cycles:** Medieval Warm Period, Little Ice Age.
- **Migration Waves:** large-scale displacement from war, famine, plague.

---

## 3. Population & Society

**Core Features:**
- Population types: classes, cultures, religions.
- Populations consume, produce, revolt, die, migrate.
- Demographics directly affect economy and manpower.

**Planned Extensions:**
- **Literacy & Information Flow:** impacts tech adoption, propaganda.
- **Guilds & Urban Politics:** guild factions drive urban unrest and policies.
- **Festivals & Public Life:** stability boosts, treasury costs.
- **Epidemics with cultural effects** (rise of sects, superstition).

---

## 4. Mesh Systems

**Core Features:**
- **Administrative Mesh:** efficiency of central control.
- **Trade Mesh:** dynamic routes, piracy, blockades.
- **Espionage Mesh:** visibility, interception, secrecy.
- **Cultural & Religious Mesh:** assimilation, faith spread.
- **Rural Mesh:** agricultural productivity, devastation.
- **Supply Mesh:** army logistics, attrition.

**Planned Extensions:**
- **Smuggling Networks:** bypass blockades, taxation.
- **Hybrid Cultural Practices:** syncretism reduces unrest but alters identity.
- **Logistics Infrastructure:** buildable depots and wagon trains.

---

## 5. Military

**Core Features:**
- Armies evolve from levies to modern forces (1000–1900).
- Delayed orders, fog of war, terrain/weather effects.
- Unit honors and legendary units tracked.

**Planned Extensions:**
- **Mercenary Companies:** independent entities, may defect if unpaid.
- **Officer Corps:** generals with traits, careers, loyalties.
- **Military Innovation Spread:** neighbors copy unit types/doctrines.
- **Logistics & Supply Depots:** explicit supply structures for armies.

---

## 6. Naval

**Core Features:**
- Naval tech tree with fleets, blockades, piracy.
- Shipyard buildings, ports, naval economy integration.

**Planned Extensions:**
- **Naval Crew Quality:** morale, loyalty, potential mutiny.
- **Privateering Licenses:** letters of marque affect economy and diplomacy.
- **Naval Trade Companies:** fleets that evolve into semi-autonomous factions.

---

## 7. Espionage & Intrigue

**Core Features:**
- Agents perform missions: sabotage, assassination, coups, propaganda.
- Espionage log/history, suspicion meters, counter-espionage.

**Planned Extensions:**
- **Espionage Economy:** agents can be bribed by rivals.
- **Blackmail:** spies uncover scandals to sway nobles or councilors.
- **Disinformation Campaigns:** fake news destabilizes enemy realms.

---

## 8. Royal Council, Factions & Politics

**Core Features:**
- Royal council roles: advisors, faction loyalties, coups, scandals.
- Factions: nobility, clergy, burghers, peasants, military.

**Planned Extensions:**
- **Dynastic Marriages:** alliances, claims, and succession crises.
- **Legal Codex:** layered laws (feudal, religious, codified).
- **Bureaucracy Simulation:** corruption risk as centralization grows.

---

## 9. Events & History

**Core Features:**
- Branching historical events: Hundred Years' War, Reformation, Dutch Revolt, etc.
- Political, economic, military, cultural, and religious events.

**Planned Extensions:**
- **Character-Driven Events:** scandals, duels, romances.
- **Parallel Histories:** alternate outcomes for major wars/reformations.
- **Cultural Golden Ages:** unlock unique art, science, prestige effects.

---

## 10. Technology

**Core Features:**
- 8 technologies per era (1250–1900).
- Unlocks: units, doctrines, reforms, espionage visibility.

**Planned Extensions:**
- **Technology Diffusion:** innovations spread along trade/cultural networks.
- **Patronage of Science/Arts:** funding accelerates certain branches.
- **Tech Theft Expansion:** espionage contributes partial progress.

---

## 11. Economy & Trade

**Core Features:**
- Dynamic goods flow based on production, demand, blockades.
- Buildings produce/consume, trade routes optimized dynamically.

**Planned Extensions:**
- **Financial Instruments:** banks, loans, bankruptcy risk.
- **Insurance:** for trade fleets vs piracy/storms.
- **Resource Depletion:** mines and forests can run dry.
- **Black Markets:** smuggling networks outside crown control.

---

## 12. AI Behavior

**Core Features:**
- AI decisions guided by doctrines, diplomacy, economy.
- AI has espionage, council, and factional logic.

**Planned Extensions:**
- **Doctrinal AI Styles:** Militarist, Diplomatic, Mercantile, Religious.
- **Espionage Personalities:** aggressive vs passive spy states.
- **Long-Term Memory:** AI remembers betrayals for decades.

---

## Implementation Status

See `docs/design/IMPLEMENTATION_STATUS.md` for tracking which features have been implemented vs. planned.

**Current Focus Areas:**
1. Core ECS systems (✅ Complete)
2. Map rendering LOD 0-3 (✅ Complete)
3. **Map rendering LOD 4 - Terrain Grid** (⚠️ PENDING - see Section 2: Geography & World)
4. Military and Naval campaigns with terrain integration (⚠️ PENDING)

---

*This document represents the original vision and planned extensions. Implementation is ongoing.*
