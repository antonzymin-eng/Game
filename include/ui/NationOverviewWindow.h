// ============================================================================
// NationOverviewWindow.h - Nation statistics overview
// Created: October 29, 2025
// Updated: November 18, 2025 - Added portrait support
// ============================================================================

#pragma once

#include <string>
#include <cstdint>

namespace ui {
    class PortraitGenerator;
}

namespace game {
namespace character {
    class CharacterComponent;
}
}

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

        /**
         * @brief Set the portrait generator
         */
        void SetPortraitGenerator(PortraitGenerator* generator) { portraitGenerator_ = generator; }

        /**
         * @brief Set the current ruler to display
         */
        void SetRuler(const game::character::CharacterComponent* ruler) { currentRuler_ = ruler; }

    private:
        bool visible_ = false; // Hidden by default, show on hotkey
        PortraitGenerator* portraitGenerator_ = nullptr;
        const game::character::CharacterComponent* currentRuler_ = nullptr;

        void RenderRulerPortrait();
        void RenderEconomyTab();
        void RenderMilitaryTab();
        void RenderDiplomacyTab();
    };

} // namespace ui
