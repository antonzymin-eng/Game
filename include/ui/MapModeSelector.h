#pragma once

#include "map/render/MapRenderer.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <functional>

namespace ui {

/**
 * @brief Map Mode Selector UI Component
 *
 * Provides an interface for selecting different map visualization modes
 * such as political, terrain, trade, military, diplomatic, etc.
 */
class MapModeSelector {
public:
    /**
     * @brief Map mode enumeration
     */
    enum class MapMode {
        POLITICAL = 0,      // Show political borders and nation colors
        TERRAIN,            // Show terrain types
        TRADE,              // Show trade routes and economic activity
        MILITARY,           // Show armies and fortifications
        DIPLOMATIC,         // Show alliances and relations
        RELIGIOUS,          // Show religious distribution
        CULTURAL,           // Show cultural groups
        DEVELOPMENT,        // Show province development levels
        POPULATION,         // Show population density
        COUNT
    };

    /**
     * @brief Construct a new Map Mode Selector
     *
     * @param map_renderer Reference to the map renderer to control
     */
    explicit MapModeSelector(game::map::MapRenderer& map_renderer);

    ~MapModeSelector() = default;

    // Non-copyable
    MapModeSelector(const MapModeSelector&) = delete;
    MapModeSelector& operator=(const MapModeSelector&) = delete;

    /**
     * @brief Render the map mode selector
     *
     * Called every frame to display the UI. Handles all ImGui rendering.
     */
    void Render();

    /**
     * @brief Set the visibility of the selector
     *
     * @param visible True to show the selector, false to hide it
     */
    void SetVisible(bool visible);

    /**
     * @brief Check if the selector is visible
     *
     * @return true if visible, false otherwise
     */
    bool IsVisible() const;

    /**
     * @brief Toggle the selector visibility
     */
    void ToggleVisibility();

    /**
     * @brief Set the current map mode
     *
     * @param mode The map mode to switch to
     */
    void SetMapMode(MapMode mode);

    /**
     * @brief Get the current map mode
     *
     * @return The current map mode
     */
    MapMode GetCurrentMapMode() const;

    /**
     * @brief Set callback for when map mode changes
     *
     * @param callback Function to call when mode changes
     */
    void SetOnMapModeChanged(std::function<void(MapMode)> callback);

private:
    // Dependencies
    game::map::MapRenderer& map_renderer_;

    // UI State
    bool visible_ = true;
    MapMode current_mode_ = MapMode::POLITICAL;
    bool expanded_ = false;

    // Callback
    std::function<void(MapMode)> on_mode_changed_callback_;

    // Helper methods
    void RenderCompactMode();
    void RenderExpandedMode();
    void RenderModeButton(MapMode mode, const char* label, const char* icon);

    // Utility methods
    std::string GetModeName(MapMode mode) const;
    std::string GetModeDescription(MapMode mode) const;
    const char* GetModeIcon(MapMode mode) const;
    ImVec4 GetModeColor(MapMode mode) const;

    // Apply map mode to renderer
    void ApplyMapMode(MapMode mode);
};

} // namespace ui
