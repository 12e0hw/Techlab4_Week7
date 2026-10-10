#include "EnginePCH.h"
#include "PointLightComponent.h"
#include "Render/LineBatcher.h"

void UPointLightComponent::DrawDebug(FLineBatcher* LineBatcher) const
{
	if (!LineBatcher || AttenuationRadius <= 0.0f)
	{
		return;
	}

	const FVector Center = GetWorldLocation();
	constexpr int32 SEGMENTS = 24;
	const float Radius = AttenuationRadius;

	for (int32 i = 0; i < SEGMENTS; ++i)
	{
		const float a1 = (2.0f * PI * i) / SEGMENTS;
		const float a2 = (2.0f * PI * (i + 1)) / SEGMENTS;
		const float c1 = std::cosf(a1) * Radius;
		const float s1 = std::sinf(a1) * Radius;
		const float c2 = std::cosf(a2) * Radius;
		const float s2 = std::sinf(a2) * Radius;

		// XY 평면 원
		LineBatcher->AddLine(Center + FVector(c1, s1, 0.0f), Center + FVector(c2, s2, 0.0f), LightColor);
		// XZ 평면 원
		LineBatcher->AddLine(Center + FVector(c1, 0.0f, s1), Center + FVector(c2, 0.0f, s2), LightColor);
		// YZ 평면 원
		LineBatcher->AddLine(Center + FVector(0.0f, c1, s1), Center + FVector(0.0f, c2, s2), LightColor);
	}
}
