# 11th Century Historical Data - Year 1100 CE

## Overview

This implementation adds authentic historical borders and polities for Europe in the year 1100 CE, based on scholarly GeoJSON data from the [aourednik/historical-basemaps](https://github.com/aourednik/historical-basemaps) project.

## Data Source

**Source Repository**: aourednik/historical-basemaps
**License**: Open source
**Format**: GeoJSON (WGS 84, EPSG:4326)
**Files Used**:
- `world_1000.geojson` - Europe at year 1000 CE
- `world_1100.geojson` - Europe at year 1100 CE

**Location in Project**: `data/maps/geojson_source/historical/`

## Statistics

- **Total Map Files**: 54
- **Individual Polity Maps**: 53
- **Combined Europe Map**: 1 (59 provinces from 53 polities)
- **Nation Definitions**: 8 major polities
- **Time Period**: High Middle Ages (11th century)

## European Polities Included (Year 1100 CE)

### Major Kingdoms

1. **Kievan Rus'** - Federation of East Slavic principalities under Rurikid dynasty
   - At peak territorial extent in 11th century
   - Controlled vast territories from Baltic to Black Sea
   - Orthodox Christian state with Byzantine cultural influence

2. **Holy Roman Empire** - Multi-ethnic confederation under Germanic emperor
   - Theoretical successor to Western Roman Empire
   - Controlled Germany, parts of Italy, Burgundy
   - Elective monarchy with prince-electors

3. **Kingdom of France** - Capetian dynasty
   - Royal domain centered on Île-de-France
   - Powerful feudal lords (dukes of Normandy, Aquitaine, etc.)
   - "Most Christian King" allied with papacy

4. **Kingdom of England** - Post-Norman Conquest
   - Norman French-speaking aristocracy ruling Anglo-Saxon England
   - William the Conqueror's successors (William II, Henry I)
   - Continental possessions in Normandy

5. **Byzantine Empire** - Eastern Roman Empire
   - Recovering from Manzikert defeat (1071)
   - Still controlled Anatolia, Balkans, parts of Italy
   - Orthodox Christian civilization with Greek culture

### Iberian Peninsula (Reconquista)

6. **Kingdom of Castile** - Leading Christian kingdom in Reconquista
7. **Kingdom of León** - Separate kingdom, rival of Castile
8. **Kingdom of Aragón** - Eastern Christian kingdom
9. **Kingdom of Navarre** - Pyrenean kingdom
10. **Almoravid Dynasty** - Islamic Berber empire controlling southern Iberia

### Central and Eastern Europe

11. **Kingdom of Poland** - Piast dynasty, recently elevated to kingdom (1025)
12. **Kingdom of Hungary** - Magyar kingdom under Árpád dynasty
13. **Kingdom of Croatia** - Independent kingdom (later personal union with Hungary)
14. **Kingdom of Serbia** - Emerging Slavic kingdom in Balkans

### Scandinavia

15. **Kingdom of Norway** - Recently Christianized Viking kingdom
16. **Kingdom of Sweden** - Consolidating under Christian kings
17. **Kingdom of Denmark** - North Sea power with English ambitions
18. **Icelandic Commonwealth** - Unique republican government with Althing

### Italy

19. **Papal States** - Temporal domain of Pope
20. **Duchy of Benevento** - Lombard duchy in southern Italy
21. **Venice** - Merchant republic, major Mediterranean naval power
22. **Corsica** - Island territory
23. **Sardinia** - Island kingdom

### Celtic and Atlantic

24. **Scotland** - Independent kingdom under Malcolm III and successors
25. **Celtic Kingdoms** - Wales and other Celtic territories
26. **Duchy of Brittany** - Semi-independent duchy in France

### Other European Polities

27. **Duchy of Burgundy** - Powerful French duchy
28. **Principality of Polotsk** - Rus' principality in modern Belarus
29. **Prussians** - Pagan Baltic tribe
30. **Sámi** - Indigenous people of northern Scandinavia
31. **Finno-Ugric Tribes** - Taiga hunter-gatherers in northern Russia

### Caucasus Region

32. **Kingdom of Georgia** - Christian kingdom in Caucasus
33. **Armenia** - Armenian principalities
34. **Various Caucasian Principalities**: Shirvan, Derbent, Arran, Artsakh, Goghtn, Syunik, Tashir, Kakheti-Hereti, Khundzi, Leks, Maskat, Durdzuks, Emirate of Tiflis

### Steppe and Eastern

35. **Cuman-Kipchak Confederation** - Turkic nomads in Pontic-Caspian steppe
36. **Volga Bulgars** - Turkic-Bulgarian state on Volga River
37. **Bulgar Khanate** - Bulgarian polity
38. **Alans** - Iranian people in North Caucasus
39. **Karakalpaks** - Turkic people
40. **Oghuz** - Turkic tribal confederation

### Islamic Powers

41. **Seljuk Empire** - Turkic empire controlling much of Middle East and Anatolia
42. **Almoravid Dynasty** - Berber dynasty in North Africa and Iberia

## Major Nations with Full Data

The following 8 polities have complete nation definitions with national ideas, traditions, and historical context:

1. **Kievan Rus'** (kievan_rus)
2. **Holy Roman Empire** (holy_roman_empire)
3. **Kingdom of France** (france)
4. **Kingdom of England** (england)
5. **Byzantine Empire** (byzantine_empire)
6. **Kingdom of Castile** (castile)
7. **Kingdom of Poland** (poland)
8. **Kingdom of Hungary** (hungary)

## File Structure

### Map Files
**Location**: `data/maps/historical_1100/`

Individual polity maps:
- Format: `map_{polity_id}_1100.json`
- Examples: `map_kievan_rus_1100.json`, `map_holy_roman_empire_1100.json`
- Each contains provinces with authentic historical borders

Combined map:
- `map_europe_1100.json` - All 59 provinces in one file

### Nation Data
**Location**: `data/nations/`

- `nations_11th_century.json` - Nation definitions for 8 major polities

### Source Data
**Location**: `data/maps/geojson_source/historical/`

- `world_1000.geojson` - Historical GeoJSON for year 1000 CE
- `world_1100.geojson` - Historical GeoJSON for year 1100 CE

### Generation Script
**Location**: Root directory

- `generate_historical_1100.py` - Converts GeoJSON to game format

## Data Structure

### Map Format

```json
{
  "map_region": {
    "id": "kievan_rus_1100",
    "name": "Kievan Rus' (1100 CE)",
    "description": "Historical borders of Kievan Rus' in year 1100 CE",
    "coordinate_system": "cartesian_2d",
    "unit": "game_units",
    "bounds": { "min_x": ..., "max_x": ..., "min_y": ..., "max_y": ... },
    "provinces": [ ... ],
    "sea_zones": [],
    "trade_nodes": [],
    "historical_year": 1100
  }
}
```

### Province Attributes

Each province includes:
- **id**: Unique identifier
- **name**: Historical territory name
- **owner_realm**: Polity ID
- **terrain_type**: Based on geography (plains, mountains, forest, coast)
- **center**: Centroid coordinates
- **boundary**: Polygon coordinates converted from GeoJSON
- **religion**: Historically accurate (orthodox, catholic, sunni, pagan)
- **culture**: Matches polity
- **climate**: Geographic climate zone
- **historical_year**: 1100 marker

## Coordinate System

Same equirectangular projection as modern maps for consistency:

- **Center Point**: 10°E, 50°N (Central Europe)
- **Scale Factor**: 50 units per degree
- **Unit**: game_units
- **Projection**: WGS 84 (EPSG:4326) converted to Cartesian 2D

## Historical Context

### 11th Century Europe

**Major Events**:
- 1066: Norman Conquest of England
- 1071: Battle of Manzikert (Seljuk defeat of Byzantines)
- 1054: East-West Schism (Catholic-Orthodox split)
- 1095-1099: First Crusade
- 1000s: Christianization of Scandinavia, Poland, Hungary
- Peak of Kievan Rus' power before fragmentation
- Investiture Controversy between Pope and Emperor

**Characteristics**:
- **Feudalism**: Decentralized political power
- **Crusades Era**: Religious fervor and East-West conflict
- **Agricultural Revolution**: Three-field system spreading
- **Cathedral Building**: Romanesque architecture
- **Monastic Orders**: Cluny, Cistercian reforms
- **Trade Revival**: Italian merchant republics, Hanseatic precursors

## Usage Examples

### Loading Historical Maps

```cpp
#include "map/HistoricalMapLoader.h"

// Load 11th century Europe
auto loader = HistoricalMapLoader();
auto europe1100 = loader.loadFromJSON("data/maps/historical_1100/map_europe_1100.json");

// Load specific polity
auto kievanRus = loader.loadFromJSON("data/maps/historical_1100/map_kievan_rus_1100.json");
```

### Loading Historical Nations

```cpp
// Load 11th century nation definitions
auto nations = loader.loadNations("data/nations/nations_11th_century.json");
```

### Historical Scenario Setup

```cpp
// Create 1100 CE scenario
Scenario scenario1100;
scenario1100.year = 1100;
scenario1100.map = europe1100;
scenario1100.nations = nations11thCentury;
scenario1100.name = "High Middle Ages - Year 1100";
```

## Technical Details

### Conversion Process

1. **Download**: Fetch GeoJSON from historical-basemaps repository
2. **Filter**: Extract European polities based on geographic bounds
3. **Group**: Organize features by polity name
4. **Convert**: Transform lat/lon to game coordinates
5. **Process**: Extract main polygons from MultiPolygon geometries
6. **Enrich**: Add terrain, religion, culture based on historical context
7. **Generate**: Create JSON map files in game format

### Entity Name Mapping

Historical names are normalized to game-friendly IDs:
- "Kievan Rus" → `kievan_rus`
- "Holy Roman Empire" → `holy_roman_empire`
- "Kingdom of France" → `france`
- "Cuman-Kipchak confederation" → `cumans`
- etc.

### Terrain Determination

Automatic terrain classification based on:
- Entity names (e.g., "highland", "forest", "steppe")
- Geographic location
- Historical knowledge

### Religion Assignment

Historically accurate religions:
- **Orthodox**: Byzantine Empire, Kievan Rus', Serbia, Bulgaria
- **Catholic**: Western/Central Europe (France, HRE, Poland, Hungary, England)
- **Sunni**: Islamic polities (Almoravids, Seljuks)
- **Pagan**: Prussians, Sámi, some Finno-Ugric groups

## Validation

All generated files validated for:
- ✓ Valid JSON structure
- ✓ Required fields present
- ✓ Province boundaries (minimum 3 points)
- ✓ Coordinate ranges within bounds
- ✓ Non-empty province lists
- ✓ Consistent naming conventions

## Comparison with Modern Data

### Differences from Modern Maps

1. **Territory**: Polities much larger/smaller than modern nation-states
2. **Borders**: Less precise, often "fuzzy" (border precision = 1)
3. **Fragmentation**: Holy Roman Empire vs. modern Germany
4. **Union**: Kievan Rus' vs. separate Russia/Ukraine/Belarus
5. **Reconquista**: Islamic Iberia vs. modern Christian Spain/Portugal

### Similarities

- Same coordinate system for compatibility
- Same data structure for easy integration
- Same province format (id, name, boundary, center)

## Future Enhancements

Potential additions:

1. **More Time Periods**: Year 1000, 1200, 1300 maps
2. **Higher Granularity**: Individual duchies and counties within kingdoms
3. **Dynamic Borders**: Represent border changes over time
4. **Vassal Relationships**: Encode feudal hierarchies
5. **Trade Routes**: Historical trade nodes and routes
6. **Battle Sites**: Major historical battle locations
7. **Cities**: Major cities with historical populations
8. **Claims**: Historical territorial claims and pretensions

## Research Sources

Historical borders based on:
- Academic historical atlases
- Scholarly consensus on 11th century political geography
- Archaeological and documentary evidence
- Modern historical GIS projects

**Primary Source**: [aourednik/historical-basemaps](https://github.com/aourednik/historical-basemaps)

## License

- **GeoJSON Source Data**: Subject to original historical-basemaps license
- **Generated Game Data**: Part of Game project
- **Nation Definitions**: Original historical research and game design

## Credits

- **Historical Data**: André Ourednik and contributors (historical-basemaps)
- **Conversion Script**: Generated for Game project
- **Nation Definitions**: Historical research and game balance

---

*For modern European geography, see EUROPEAN_REGIONS_SUMMARY.md*
