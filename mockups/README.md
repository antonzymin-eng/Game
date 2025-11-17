# Mechanica Imperii - UI Mock-ups

This directory contains HTML mock-ups of the game's user interface in two categories:

## Actual Implementation Mock-ups (Current State)

These represent what the **actual game code** currently implements:

### `actual_ingame_view.html`
The main in-game view showing the current ImGui-based UI implementation:
- **Menu Bar**: Game, Systems, Configuration, Debug menus
- **Game Control Panel**: Date display, speed controls (pause, 1x-4x), Quick Save/Load
- **Map Mode Selector**: 9 map modes (Political, Terrain, Trade, Military, Diplomatic, Religious, Cultural, Development, Population)
- **Province Information Window**: Tabbed interface with Overview, Administration, and Geography
- **Map Canvas**: OpenGL-rendered game map (simplified representation in HTML)

**Code Location**: `/apps/main.cpp` (RenderUI function), various `/src/ui/*.cpp` files

### `actual_trade_window.html`
The Trade System window as implemented in the code:
- **Header Statistics**: Active routes, disrupted routes, trade hubs, monthly volume
- **Tabbed Interface**: Trade Routes, Trade Hubs, Market Analysis, Opportunities
- **Filtering**: Active only, profitable only, search functionality
- **Route Details**: Revenue, costs, utilization, resource types, disruption status

**Code Location**: `/src/ui/TradeSystemWindow.cpp`

### `actual_population_window.html`
The Population Information window as implemented:
- **Population Statistics**: Total population, density, growth rate, birth/death rates
- **Class Distribution**: Visual breakdown of nobles, clergy, merchants, artisans, peasants, outlaws
- **Employment**: Detailed job distribution and unemployment rate
- **Culture & Religion**: Cultural composition and religious demographics

**Code Location**: `/src/ui/PopulationInfoWindow.cpp`

---

## Design Concept Mock-ups (Future Possibilities)

These are **design ideas** for what the UI could look like with a more polished, historical grand strategy aesthetic:

### `start_screen.html`
Concept for a dedicated start screen with:
- Dramatic title display
- "Press any key to begin" prompt
- Historical theming with gold/brown color scheme

**Status**: Not currently implemented (game launches directly into gameplay)

### `main_menu.html`
Concept for a main menu with:
- Campaign management options
- News/dispatches section
- Game statistics panel
- Settings and configuration

**Status**: Not currently implemented (MainMenuUI.cpp is a placeholder)

### `ingame_ui.html`
Enhanced concept for in-game interface with:
- Top resource bar (gold, prestige, manpower, research)
- Nation information display
- Notifications panel
- Minimap
- Enhanced province panels

**Status**: Partially implemented - current version uses ImGui windows instead of integrated HUD

### `ingame_map.html`
Interactive map concept with:
- Clickable SVG provinces
- Interactive tooltips
- Army/city markers
- Trade route visualization
- Zoom controls

**Status**: Map rendering exists in OpenGL, but this HTML represents an enhanced interactive concept

---

## Key Differences

| Feature | Actual Implementation | Design Concepts |
|---------|----------------------|-----------------|
| **UI Framework** | ImGui (immediate mode GUI) | HTML/CSS (conceptual) |
| **Visual Style** | Dark gray ImGui theme | Gold/brown historical theme |
| **Start Screen** | None - launches into game | Dramatic title screen |
| **Main Menu** | Placeholder only | Full-featured menu |
| **Windows** | Movable ImGui windows | Integrated HUD panels |
| **Color Scheme** | Gray/blue/cyan (#5080a0) | Gold/brown (#d4af37) |

---

## Implementation Status

### ✅ Fully Implemented
- Game Control Panel
- Map Mode Selector
- Province Information Window
- Technology Information Window
- Trade System Window
- Population Info Window
- Battle Viewer Window (structure complete, needs data integration)
- Nation Overview Window (structure complete, needs data integration)

### 🚧 Partially Implemented
- Toast notification system (logs to console, not visual yet)
- Main menu bar (functional but basic)

### ❌ Not Implemented
- Start screen / splash screen
- Main menu UI
- Integrated HUD (currently uses separate windows)
- Administrative UI
- Historical aesthetic theme

---

## Viewing the Mock-ups

1. **Open any `.html` file** in a web browser to see the mock-up
2. **Actual implementation files** show what the game currently renders
3. **Design concept files** show potential future enhancements

---

## Technical Notes

- The actual game uses **SDL2 + OpenGL 3.3 + ImGui** for rendering
- All UI code is in `/src/ui/` and `/include/ui/`
- Main render loop is in `/apps/main.cpp` lines 867-1021
- UI windows are modular and can be toggled from the menu bar
- The game runs at 60 FPS with real-time UI updates

---

*Last Updated: 2025-11-17*
