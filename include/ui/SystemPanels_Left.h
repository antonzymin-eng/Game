#pragma once
namespace core { class SimulationState; }
namespace ui {
void DrawFactionsPanel(core::SimulationState& sim);
void DrawDiplomacyPanel(core::SimulationState& sim);
void DrawEspionagePanel(core::SimulationState& sim);
void DrawTradePanel(core::SimulationState& sim);
void DrawEconomyPanel(core::SimulationState& sim);
void DrawTechPanel(core::SimulationState& sim);
void DrawEventsPanel(core::SimulationState& sim);
void DrawMilitaryPanel(core::SimulationState& sim);
void DrawPopulationPanel(core::SimulationState& sim);
void DrawTradeRoutesPanel(core::SimulationState& sim);
void DrawWeatherTerrainPanel(core::SimulationState& sim);
void DrawNavalPanel(core::SimulationState& sim);
} // namespace ui