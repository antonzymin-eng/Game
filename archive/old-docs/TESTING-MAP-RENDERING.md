# Map Rendering & UI Testing Guide

**Date**: October 21, 2025  
**Purpose**: Visual testing checklist for map rendering system

---

## Prerequisites

- Game built successfully (`./mechanica_imperii` exists)
- Display/window system available (not headless)
- OpenGL 3.0+ support

---

## Launch Instructions

```bash
cd /path/to/Game/build
./mechanica_imperii
```

**Expected**: Game window opens showing map background

---

## Visual Testing Checklist

### 1. Basic Map Display ✓

**What to Check:**
- [ ] Window opens (1280x720 or configured size)
- [ ] Background is ocean blue color (0.1, 0.2, 0.3 RGB)
- [ ] Province boundaries visible as colored polygons
- [ ] At least 12 provinces visible (if test data loaded)

**Test Data Location**: `data/test_provinces.json`
- 12 provinces covering British Isles
- 3 realms: England (green), Wales (red), Scotland (blue)

---

### 2. Camera Controls ✓

**Pan Camera:**
- [ ] Press **W** - camera moves up
- [ ] Press **A** - camera moves left
- [ ] Press **S** - camera moves down
- [ ] Press **D** - camera moves right
- [ ] **Middle Mouse + Drag** - pan camera

**Zoom Camera:**
- [ ] **Mouse Wheel Up** - zoom in (provinces get larger)
- [ ] **Mouse Wheel Down** - zoom out (provinces get smaller)
- [ ] Zoom centers on mouse cursor position

**Expected Behavior:**
- Smooth movement with no stuttering
- Camera respects boundaries (if implemented)
- Zoom range: 0.2x to 5.0x

---

### 3. LOD System (Level of Detail) ✓

**LOD 0 - Strategic (0-20% zoom / furthest out):**
- [ ] Only realm/state boundaries visible
- [ ] Thick border lines
- [ ] Large text labels
- [ ] No individual provinces shown

**LOD 1 - Regional (20-40% zoom):**
- [ ] State and province boundaries
- [ ] Medium border lines
- [ ] Province names visible
- [ ] No features shown

**LOD 2 - Provincial (40-60% zoom / DEFAULT):**
- [ ] Province polygons with fill colors
- [ ] Province names clearly visible
- [ ] Border lines visible
- [ ] Major cities shown as dots

**LOD 3 - Local (60-80% zoom):**
- [ ] All province details
- [ ] Cities with names
- [ ] Terrain features visible:
  - Mountains (triangles)
  - Forests (tree icons)
  - Rivers (blue lines)
  - Fortresses (castle icons)

**LOD 4 - Tactical (80-100% zoom / closest):**
- [ ] Heightmap terrain rendering (if TerrainRenderer implemented)
- [ ] Buildings visible
- [ ] Roads visible
- [ ] Units visible

**How to Test:**
1. Start at default zoom (LOD 2)
2. Zoom all the way out → Should transition through LOD 1 → LOD 0
3. Zoom all the way in → Should transition through LOD 3 → LOD 4
4. Verify smooth transitions (no popping or sudden changes)

---

### 4. Province Selection ✓

**Click Selection:**
- [ ] **Left-click** on a province
- [ ] Province highlights (animated pulse effect)
- [ ] Province ID printed to console
- [ ] PopulationInfoWindow updates (if population data exists)

**Visual Feedback:**
- [ ] Selected province has pulsing alpha highlight
- [ ] Pulse animation cycles smoothly (2-second period)
- [ ] Border thickness increases on selection
- [ ] Previous selection clears when clicking new province

**Click Empty Area:**
- [ ] Click on ocean/empty space
- [ ] Selection clears
- [ ] PopulationInfoWindow shows "Select a province..." message

---

### 5. PopulationInfoWindow UI ✓

**Window Location:**
- Should appear on right side or as floating window
- Title: "Population Info - [Province Name]"

**When No Province Selected:**
- [ ] Shows message: "Select a province to view population data"
- [ ] Window is grey/inactive

**When Province Selected (with PopulationComponent):**
- [ ] **Total Population** section:
  - Total population count
  - Population density (per sq km)
  - Growth rate (colored: green = positive, red = negative)
  - Birth rate, death rate, migration rate

- [ ] **Demographics** section (collapsible):
  - Age distribution (children/adults/elderly with percentages)
  - Gender distribution (males/females with percentages)
  - Quality of life: happiness, health, literacy, wealth

- [ ] **Employment** section (collapsible):
  - Employment rate
  - Labor force breakdown (workers, unemployed, dependents)
  - Military: eligible, quality, service obligation

- [ ] **Culture & Religion** section (collapsible):
  - Cultural groups with populations and percentages
  - Religious groups with populations and percentages
  - Social dynamics (assimilation, conversion, mobility, tension)

**When Province Has No PopulationComponent:**
- [ ] Shows warning: "No population data available for this province"

**Real-Time Updates:**
- [ ] Click different provinces → window updates immediately
- [ ] All statistics display correctly formatted
- [ ] No crashes or errors

---

### 6. Debug Window ✓

**Map Debug Window** (should appear automatically):
- [ ] Shows current LOD level
- [ ] Shows camera position (x, y)
- [ ] Shows zoom level
- [ ] Shows rendered province count
- [ ] Shows rendered feature count
- [ ] Shows frame time (ms)

**Settings:**
- [ ] Toggle "Render Borders" checkbox
- [ ] Toggle "Render Names" checkbox
- [ ] Toggle "Render Features" checkbox
- [ ] Changes apply immediately

---

### 7. Feature Rendering (LOD 2-3) ✓

**Cities:**
- [ ] Shown as circles/dots
- [ ] Larger cities = larger dots
- [ ] Names appear on hover or at close zoom
- [ ] Scale with zoom level

**Terrain Features:**
- [ ] **Mountains**: Triangle icons, grey/brown color
- [ ] **Forests**: Tree icons, green color
- [ ] **Rivers**: Blue lines connecting points
- [ ] **Fortresses**: Castle icons, stone color

**Feature Visibility:**
- [ ] Features only appear at LOD 2+
- [ ] Features disappear when zooming out to LOD 0-1
- [ ] No overdraw or z-fighting issues

---

### 8. Performance Testing ✓

**Target**: <16ms frame time (60 FPS)

**Monitor:**
- [ ] Check debug window "Frame Time"
- [ ] Should be <16ms most of the time
- [ ] No stuttering or freezing

**Stress Tests:**
1. **Pan rapidly** across map
   - [ ] No lag, smooth movement
   
2. **Zoom in/out rapidly**
   - [ ] LOD transitions smooth
   - [ ] No frame drops
   
3. **Select provinces rapidly**
   - [ ] UI updates without lag
   - [ ] No memory leaks (monitor over time)

**Expected Performance:**
- LOD 0: ~10-50 polygons → <1ms
- LOD 1: ~100-200 polygons → <2ms
- LOD 2: ~200-500 polygons → <5ms
- LOD 3: ~500-1000 polygons + features → <10ms

---

### 9. Integration Testing ✓

**Test Scenario 1: Province Exploration**
1. Launch game
2. Pan camera to find all 12 test provinces
3. Zoom in and out at various levels
4. Click each province and verify:
   - Selection works
   - Name displays correctly
   - PopulationInfoWindow updates (if data exists)

**Test Scenario 2: UI Data Flow**
1. Select a province with population data
2. Verify all statistics display correctly
3. Select different provinces
4. Verify UI updates in real-time
5. Click empty space, verify UI clears

**Test Scenario 3: LOD Transitions**
1. Start at default zoom (LOD 2)
2. Slowly zoom out → LOD 1 → LOD 0
3. Verify provinces disappear/simplify correctly
4. Slowly zoom in → LOD 1 → LOD 2 → LOD 3
5. Verify features appear at correct zoom levels

---

## Known Issues / Limitations

**Current Implementation:**
- ✅ LOD 0-3 fully implemented
- ⚠️ LOD 4 (TerrainRenderer) not yet implemented
- ✅ Province selection works
- ✅ PopulationInfoWindow wired to ECS
- ✅ Test data: 12 provinces (British Isles)

**Not Yet Implemented:**
- LOD 4 heightmap terrain rendering
- Texture atlasing for features
- Mesh caching (provinces rebuilt each frame)
- Advanced culling (only basic frustum culling)

**Performance Notes:**
- Current implementation uses ImGui DrawList (CPU-based)
- Large maps (1000+ provinces) may need GPU mesh rendering
- Consider implementing mesh caching if frame time >16ms

---

## Test Data Details

**File**: `data/test_provinces.json`

**Provinces** (12 total):
1. **England Provinces** (6):
   - London (Capital, 250k pop)
   - Cornwall, Devon, Kent, East Anglia, Yorkshire

2. **Wales Provinces** (3):
   - Cardiff (Capital, 80k pop)
   - Gwynedd, Powys

3. **Scotland Provinces** (3):
   - Edinburgh (Capital, 120k pop)
   - Highlands, Lowlands

**Features** (various):
- 3 capitals with large populations
- Mountains in Wales and Scotland
- Forests scattered throughout
- Coastal provinces with harbors

---

## Troubleshooting

**Game Won't Launch:**
- Check OpenGL support: `glxinfo | grep "OpenGL version"`
- Verify SDL2 installed: `ldconfig -p | grep SDL2`
- Check console for error messages

**No Map Visible:**
- Verify `data/test_provinces.json` exists
- Check console: "Loaded X provinces" message
- Try zooming out (mouse wheel down)

**UI Not Updating:**
- Verify province has PopulationComponent
- Check console for entity manager errors
- Try restarting game

**Performance Issues:**
- Check debug window frame time
- Reduce zoom level to lower LOD
- Close other applications

**Selection Not Working:**
- Verify left-clicking inside province boundaries
- Check console for "Selected province: X" message
- Try clicking province centers (easier hit detection)

---

## Console Output Reference

**Expected Console Messages:**

```
Mechanica Imperii - Starting with all critical fixes applied...
Configuration loaded successfully
Enhanced systems initialized
Legacy systems initialized
Map system initializing...
Loading provinces from: data/test_provinces.json
Loaded 12 provinces with LOD boundaries
Map system initialized
UI systems initialized
[Main loop running]
Selected province: 1  # When clicking provinces
```

**Error Messages to Watch For:**
- "Failed to load provinces" → Check JSON file path
- "No PopulationComponent found" → Province missing population data (OK)
- "EntityManager error" → ECS integration issue
- OpenGL errors → Graphics driver issue

---

## Success Criteria

✅ **All tests pass if:**
1. Game launches without crashes
2. Map displays correctly
3. All 12 provinces visible
4. Camera controls work smoothly
5. LOD transitions are smooth
6. Province selection works
7. PopulationInfoWindow displays data
8. Frame time <16ms at all LOD levels
9. No memory leaks over 5 minutes
10. UI remains responsive

---

## Next Steps After Testing

If all tests pass:
- ✅ Map rendering system is production-ready
- Consider implementing TerrainRenderer (LOD 4)
- Profile performance on larger maps
- Add texture atlasing for features
- Implement mesh caching

If issues found:
- Document issues in GitHub issues
- Check console logs for errors
- Run headless tests for regression
- Debug with GDB if crashes occur

---

**Happy Testing!** 🗺️
