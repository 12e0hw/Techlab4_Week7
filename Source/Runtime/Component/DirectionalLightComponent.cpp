#include "EnginePCH.h"
#include "DirectionalLightComponent.h"
#include "Engine/LightSceneProxy.h"
#include "Render/LineBatcher.h"

FDirectionalLightInfo UDirectionalLightComponent::GetDirectionalLightInfo() const
{
	FDirectionalLightInfo Info;
	Info.Color = GetShaderColorWithIntensity();
	Info.Direction = GetLightDirection();
	return Info;
}

FLightSceneProxy* UDirectionalLightComponent::CreateSceneProxy()
{
	return new FDirectionalLightSceneProxy(this);
}

void UDirectionalLightComponent::DrawDebug(FLineBatcher* LineBatcher) const
{
    if (!LineBatcher) return;

    const FVector Center = GetWorldLocation();
    const FVector Forward = GetLightDirection().GetSafeNormal();

    // Forward가 위/아래(Z축)를 똑바로 바라보고 있으면 Y축을 기준으로, 아니면 Z축을 기준으로 삼음
    const FVector UpGuide = (std::abs(Forward.Z) > 0.99f) ? FVector(0.0f, 1.0f, 0.0f) : FVector(0.0f, 0.0f, 1.0f);

    const FVector Right = FVector::Cross(Forward, UpGuide).GetSafeNormal();
    const FVector Up = FVector::Cross(Right, Forward).GetSafeNormal();

    const FLinearColor ColorLinear = LightColor.ReinterpretAsLinear();
    const FVector4 Color = FVector4(ColorLinear.R, ColorLinear.G, ColorLinear.B, 1.0f);
    const float Length = 3.0f;
    const float thickness = 0.1f;
    const float Spacing = 0.5f;

    LineBatcher->AddArrow(Center, Forward, Length, thickness, Spacing, Color);
}
