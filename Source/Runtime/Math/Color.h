#pragma once

#include <cmath>

#include "Core/Types.h"

struct FColor;

struct FLinearColor
{
	float R = 0.f, G = 0.f, B = 0.f, A = 1.f;

	FLinearColor() = default;
	constexpr FLinearColor(float InR, float InG, float InB, float InA = 1.f) : R(InR), G(InG), B(InB), A(InA) {}
	explicit FLinearColor(const FColor& C);

	FColor ToFColor(bool bSRGB) const;
	FLinearColor operator*(float S) const;
	static float SRGBToLinear(float C);
	static float LinearToSRGB(float L);
};

struct FColor
{
	uint8 B = 0, G = 0, R = 0, A = 255;

	FColor() = default;
	constexpr FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255) : B(InB), G(InG), R(InR), A(InA) {}

	FLinearColor ReinterpretAsLinear() const;
};
