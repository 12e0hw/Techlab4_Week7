#pragma once

enum class EShaderType
{
	Vertex,
	Pixel,
};

enum class EShadingModel : uint8
{
	Gouraud = 0,
	Lambert = 1,
	BlinnPhong = 2,
	Count
};