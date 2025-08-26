#pragma once
#include <string>

namespace ui {

    enum class Screen { MainMenu, NewGame, Settings, Credits, Game, Quit };

    struct MenuContext { bool hasSave = false; };

    struct AppState {
      
        bool startNewGameRequested = false;  // main menu -> game
        bool continueRequested = false;  // main menu -> game after load
        bool exitRequested = false;  // main menu -> quit
        bool exitToMainMenu = false;  // in-game -> back to menu

        Screen screen = Screen::MainMenu;
        bool   requestExit = false;
        MenuContext menu;
        bool   pauseMenuOpen = false;
    };

    // declaration only
    AppState& App();

} // namespace ui
