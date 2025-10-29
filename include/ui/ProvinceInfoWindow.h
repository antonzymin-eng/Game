// ============================================================================
// ProvinceInfoWindow.h - Detailed province information panel
// Created: October 29, 2025
// ============================================================================

#pragma once

#include "game/gameplay/Province.h"
#include <memory>

namespace ui {

    /**
     * @brief Province information window - shows detailed province data
     *
     * Displays when a province is selected on the map.
     * Shows population, economy, administration, military, etc.
     */
    class ProvinceInfoWindow {
    public:
        ProvinceInfoWindow();
        ~ProvinceInfoWindow() = default;

        /**
         * @brief Render the province info window
         */
        void Render();

        /**
         * @brief Set the currently selected province
         */
        void SetSelectedProvince(const game::Province* province);

        /**
         * @brief Clear selection (hide window)
         */
        void ClearSelection();

        /**
         * @brief Check if a province is currently selected
         */
        bool HasSelection() const { return selected_province_ != nullptr; }

        /**
         * @brief Set visibility
         */
        void SetVisible(bool visible) { visible_ = visible; }

        /**
         * @brief Check if window is visible
         */
        bool IsVisible() const { return visible_; }

    private:
        const game::Province* selected_province_ = nullptr;
        bool visible_ = true;

        // Render subsections
        void RenderHeader();
        void RenderPopulationSection();
        void RenderEconomySection();
        void RenderAdministrationSection();
        void RenderGeographySection();
    };

} // namespace ui
