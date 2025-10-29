// ============================================================================
// NationOverviewWindow.h - Nation statistics overview
// Created: October 29, 2025
// ============================================================================

#pragma once

#include <string>

namespace ui {

    /**
     * @brief Nation overview window - shows national statistics
     *
     * Displays treasury, income, population, military, etc.
     */
    class NationOverviewWindow {
    public:
        NationOverviewWindow();
        ~NationOverviewWindow() = default;

        /**
         * @brief Render the nation overview window
         */
        void Render();

        /**
         * @brief Toggle window visibility
         */
        void Toggle() { visible_ = !visible_; }

        /**
         * @brief Set visibility
         */
        void SetVisible(bool visible) { visible_ = visible; }

        /**
         * @brief Check if window is visible
         */
        bool IsVisible() const { return visible_; }

    private:
        bool visible_ = false; // Hidden by default, show on hotkey

        void RenderEconomyTab();
        void RenderMilitaryTab();
        void RenderDiplomacyTab();
    };

} // namespace ui
