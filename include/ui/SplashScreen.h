#pragma once

#include <chrono>

namespace ui {
    /**
     * SplashScreen - Opening splash screen for the game
     * Displays game title and "Press any key to continue"
     */
    class SplashScreen {
    public:
        SplashScreen();
        ~SplashScreen() = default;

        void Render();
        bool ShouldAdvance() const { return advance_to_menu_; }
        void Reset();

    private:
        bool advance_to_menu_;
        float fade_alpha_;
        std::chrono::steady_clock::time_point start_time_;

        void RenderBackground();
        void RenderTitle();
        void RenderPrompt();
    };
}
