#pragma once
#include "imgui.h"

namespace ui {

struct AppState;

void DrawMapViewport(AppState&);
bool tryLoadAssets();

void SetMapTexture(ImTextureID tex, float width, float height);
void SetRightPanelReserve(float px);

void SetProvinceIndexBuffer(const unsigned char* rgba, int width, int height, int strideBytes);

using ProvinceSelectedFn = void(*)(int provinceId);
void SetProvinceSelectionCallback(ProvinceSelectedFn fn);

int  PickProvinceAtWorld(float worldX, float worldY, unsigned* outColorRGBA = nullptr);
int  GetHoveredProvince();
int  GetSelectedProvince();

} // namespace ui
