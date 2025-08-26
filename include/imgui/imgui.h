#pragma once

// ===== Dear ImGui wrapper =====
// Tries to include a real Dear ImGui installation first. If none is found,
// falls back to a tiny, syntactically-safe stub with the handful of symbols
// your UI code touches. No macros; only inline functions to avoid surprises.

#if defined(__has_include)
  #if __has_include("external/imgui/imgui.h")
    #include "external/imgui/imgui.h"
  #elif __has_include(<imgui.h>)
    #include <imgui.h>
  #else
    // ---------- Minimal stub (header-only) ----------
    namespace ImGui {
      // Basic types that show up in signatures
      struct ImVec2 {
        float x, y;
        ImVec2(float _x = 0.0f, float _y = 0.0f) : x(_x), y(_y) {}
      };

      using ImGuiWindowFlags = int;
      using ImGuiSliderFlags = int;

      // Windows
      inline bool Begin(const char*, bool* = nullptr, ImGuiWindowFlags = 0) { return true; }
      inline void End() {}

      // Widgets
      inline void Text(const char*, ...) {}
      inline bool Checkbox(const char*, bool*) { return false; }
      inline bool SliderFloat(const char*, float*, float, float, const char* = nullptr, ImGuiSliderFlags = 0) { return false; }

      // Layout helpers
      inline void SameLine() {}
      inline void Separator() {}
    } // namespace ImGui
    // ---------- End stub ----------
  #endif
#else
  // Older compilers: best effort include
  #include <imgui.h>
#endif
