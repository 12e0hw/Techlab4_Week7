#pragma once

#include <cstddef>
#include <string_view>

enum class EViewMode
{
	Lit, Unlit, Wireframe, WorldNormal
};

enum class ELightingModel
{
	Lambert, Phong, BlinnPhong,
};

enum class EShadingModel
{
	Gouraud, Phong
};

struct FViewRenderMode
{
	EViewMode ViewMode = EViewMode::Lit;
	ELightingModel LightingModel = ELightingModel::BlinnPhong;
	EShadingModel ShadingModel = EShadingModel::Phong;
};

inline constexpr const char* ViewModeNames[] = { "Lit", "Unlit", "Wireframe", "WorldNormal"};
inline constexpr const char* LightingModelNames[] = { "Lambert", "Phong", "BlinnPhong" };
inline constexpr const char* ShadingModelNames[] = { "Gouraud", "Phong" };

template <typename TEnum, size_t N>
bool TryParseModeName(std::string_view Name, const char* const (&Names)[N], TEnum& Out)
{
	for (size_t Index = 0; Index < N; ++Index)
	{
		if (Name == Names[Index])
		{
			Out = static_cast<TEnum>(Index);
			return true;
		}
	}
	return false;
}
