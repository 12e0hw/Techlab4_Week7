#pragma once

enum class EShaderType
{
	Vertex,
	Pixel,
};

enum class EShadingModel : uint8
{
	Default = 0,
	Gouraud = 1,
	Lambert = 2,
	BlinnPhong = 3,
	Count
};