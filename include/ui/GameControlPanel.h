// ============================================================================
// GameControlPanel.h - Main game control UI (play/pause/speed/date)
// Created: October 29, 2025
// ============================================================================

#pragma once

#include "game/time/TimeComponents.h"
#include <string>

namespace ui {

    /**
     * @brief Main game control panel - play/pause, speed control, date display
     *
     * This is the most critical UI element - allows player to control game time.
     */
    class GameControlPanel {
    public:
        enum class GameSpeed {
            PAUSED = 0,
            SPEED_1 = 1,    // Normal speed
            SPEED_2 = 2,    // 2x speed
            SPEED_3 = 3,    // 3x speed
            SPEED_4 = 4     // 4x speed
        };

        GameControlPanel();
        ~GameControlPanel() = default;

        /**
         * @brief Render the control panel (call every frame)
         */
        void Render();

        /**
         * @brief Update game date display
         */
        void SetCurrentDate(const game::time::GameDate& date);

        /**
         * @brief Get current game speed setting
         */
        GameSpeed GetCurrentSpeed() const { return current_speed_; }

        /**
         * @brief Check if game is paused
         */
        bool IsPaused() const { return current_speed_ == GameSpeed::PAUSED; }

        /**
         * @brief Set visibility of the panel
         */
        void SetVisible(bool visible) { visible_ = visible; }

        /**
         * @brief Check if panel is visible
         */
        bool IsVisible() const { return visible_; }

    private:
        GameSpeed current_speed_ = GameSpeed::SPEED_1;
        game::time::GameDate current_date_;
        bool visible_ = true;

        std::string GetSpeedButtonLabel(GameSpeed speed) const;
        const char* GetSpeedIcon(GameSpeed speed) const;
    };

} // namespace ui
