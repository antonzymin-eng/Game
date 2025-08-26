
#pragma once
namespace ui { namespace SaveLoadUI {

// Render any open Save/Load modals; call once per frame on any screen.
void RenderModals();

// Open modals (used by Main Menu and in-game Menu)
void OpenSaveModal(const char* source = "InGame");
void OpenLoadModal(const char* source = "InGame");

// Optional: signal that a load completed so caller can react.
bool ConsumeLoadedFlag();

}} // namespace ui::SaveLoadUI
