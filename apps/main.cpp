#include <SDL.h>
// Use GLEW as the GL loader (matches CMake: IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>          // must be included before any GL headers

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "state/SimulationState.h"
#include "state/SaveAdapters.h"
#include "ScreenAPI.h"         // or include/ui/AppSingleton.h if that's where App() lives
#include "ui/MainMenuUI.h"
#include "ui/GameScreen.h"
#include "ui/Theme.h"
#include "io/SaveLoad.h"       // using SaveLoadManager API

static bool InitSDL(SDL_Window** window, SDL_GLContext* gl_context, int w = 1280, int h = 720) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* win = SDL_CreateWindow("Mechanica Imperii",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) return false;

    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) { SDL_DestroyWindow(win); return false; }
    SDL_GL_MakeCurrent(win, ctx);
    SDL_GL_SetSwapInterval(1); // vsync

    // ---- GLEW init (must be after a current GL context)
    glewExperimental = GL_TRUE;                 // helps with core profile
    if (glewInit() != GLEW_OK) {
        // If needed, log the error here
        SDL_GL_DeleteContext(ctx);
        SDL_DestroyWindow(win);
        return false;
    }

    *window = win;
    *gl_context = ctx;
    return true;
}

int main(int, char**) {
    SDL_Window* window = nullptr;
    SDL_GLContext gl_context = nullptr;
    if (!InitSDL(&window, &gl_context)) return 1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ui::SetupImGuiStyle()
    ; core::InstallSaveLoadAdapters();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 150"); // GL 3.2 core

    ui::AppState& app = ui::App();
    app.screen = ui::Screen::MainMenu;

    bool done = false;
    while (!done && !app.requestExit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) { done = true; break; }

            if (e.type == SDL_KEYDOWN) {
                const bool ctrl = (e.key.keysym.mod & KMOD_CTRL) != 0;
                if (app.screen == ui::Screen::Game) {
                    if (e.key.keysym.sym == SDLK_ESCAPE) {
                        app.pauseMenuOpen = !app.pauseMenuOpen;
                    }
                    // Respect ImGui text input focus; otherwise handle hotkeys.
                    if (!ImGui::GetIO().WantCaptureKeyboard) {
                        if (ctrl && e.key.keysym.sym == SDLK_s) {
                            io::SaveLoadManager::I().quickSave(); // Ctrl+S
                                  }
                        else if (e.key.keysym.sym == SDLK_F9) {
                            io::SaveLoadManager::I().loadByName("quick.misave"); // F9 quick-load                                          
                        }
                    }
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        
        switch (app.screen) {
        case ui::Screen::MainMenu:  ui::DrawMainMenu(app);   break;
        case ui::Screen::NewGame:   ui::DrawNewGame(app);    break;
        case ui::Screen::Settings:  ui::DrawSettings(app);   break;
        case ui::Screen::Credits:   ui::DrawCredits(app);    break;
        case ui::Screen::Game:      ui::DrawGameScreen(app); break;
        case ui::Screen::Quit:      done = true;             break;
        }


        ImGui::Render();
        const ImGuiIO& io = ImGui::GetIO();
        glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
        glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
