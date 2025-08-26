#pragma once
namespace ui {
// Global "open panel" one-shot signals consumed by HUDPanels
extern bool g_signal_open_economy;
extern bool g_signal_open_military;
extern bool g_signal_open_agents;

inline void OpenEconomy() { g_signal_open_economy = true; }
inline void OpenMilitary() { g_signal_open_military = true; }
inline void OpenAgents()  { g_signal_open_agents  = true;  }
} // namespace ui
