#include <SDL.h>
#include <SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "state/SimulationState.h"   // make sure this is present

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_GAMECONTROLLER)!=0) return 1;
    SDL_Window* w = SDL_CreateWindow("Mechanica Imperii (Rebuilt)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
    if(!w) return 1;
    SDL_GLContext ctx = SDL_GL_CreateContext(w); if(!ctx) return 1; SDL_GL_MakeCurrent(w,ctx); SDL_GL_SetSwapInterval(1);
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(w, ctx); ImGui_ImplOpenGL3_Init("#version 150");
    bool done=false; while(!done){
        SDL_Event e; while(SDL_PollEvent(&e)){ ImGui_ImplSDL2_ProcessEvent(&e); if(e.type==SDL_QUIT) done=true; }
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplSDL2_NewFrame(); ImGui::NewFrame();
        ImGui::Begin("Mechanica Imperii - Minimal UI");
        ImGui::Checkbox("Paused", &meta.paused); ImGui::SliderInt("Speed (x)", &meta.speed, 1, 4);
        ImGui::End();
        ImGui::Render(); int wpx=1280,hpx=720; glViewport(0,0,wpx,hpx); glClearColor(0.1f,0.12f,0.15f,1.0f); glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); SDL_GL_SwapWindow(w);
    }
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplSDL2_Shutdown(); ImGui::DestroyContext();
    SDL_GL_DeleteContext(ctx); SDL_DestroyWindow(w); SDL_Quit(); return 0;
}