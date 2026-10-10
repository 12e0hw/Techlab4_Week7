#include "EnginePCH.h"
#include "Color.h"

FLinearColor::FLinearColor(const FColor& C) : R(SRGBToLinear(C.R / 255.f)), G(SRGBToLinear(C.G / 255.f)), B(SRGBToLinear(C.B / 255.f)), A(C.A / 255.f) {}

// true = 감마 변환
FColor FLinearColor::ToFColor(bool bSRGB) const
{
	auto ToUint8 = [](float V)
		{
			V = V < 0.f ? 0.f : (V > 1.f ? 1.f : V);
			return static_cast<uint8>(V * 255.f + 0.5f);
		};

	auto ToColor = [&](float V) {
		V = bSRGB ? LinearToSRGB(V) : V;
		return ToUint8(V);
		};

	return FColor(ToColor(R), ToColor(G), ToColor(B), ToUint8(A));
}

FLinearColor FLinearColor::operator*(float S) const
{
	return FLinearColor(R * S, G * S, B * S, A * S);
}

float FLinearColor::SRGBToLinear(float C)
{
	return C <= 0.04045f ? C / 12.92f : std::pow((C + 0.055f) / 1.055f, 2.4f);
}

float FLinearColor::LinearToSRGB(float L)
{
	return L <= 0.0031308f ? L * 12.92f : 1.055f * std::pow(L, 1.0f / 2.4f) - 0.055f;
}

// 감마 변환 없음
FLinearColor FColor::ReinterpretAsLinear() const
{
	return FLinearColor(R / 255.f, G / 255.f, B / 255.f, A / 255.f);
}
