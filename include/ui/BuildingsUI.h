#pragma once
#include "imgui.h"
#include <string>
#include <vector>
#include <unordered_map>
namespace ui{
struct BuildingType{const char* key; const char* name; const char* desc; int cost; float econDelta; float popDelta;};
void BuildingsUI_Show(bool* open);
void BuildingsUI_ShowForProvince(int provinceId, const char* provinceName, bool* open);
void Buildings_PlanBuildProvince(int provinceId, const char* buildingKey);
void Buildings_PlanBuildNation(const std::vector<int>& provinceIds, const char* buildingKey);
std::vector<std::string> Buildings_GetPlanned(int provinceId);
const std::vector<BuildingType>& Buildings_List();
}
