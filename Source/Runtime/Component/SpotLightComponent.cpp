#include "EnginePCH.h"
#include "SpotLightComponent.h"
#include "Engine/LightSceneProxy.h"

void USpotLightComponent::PostEditChangeProperty(const FProperty& Property)
{
	Super::PostEditChangeProperty(Property);

	// 언리얼처럼 Outer는 1~80도로, Inner는 0~Outer로 제한한다. Inner가 Outer보다 크면 경계 계산이 뒤집힌다.
	OuterConeAngle = FMath::Clamp(OuterConeAngle, 1.0f, 80.0f);
	InnerConeAngle = FMath::Clamp(InnerConeAngle, 0.0f, OuterConeAngle);
}

FSpotLightInfo USpotLightComponent::GetSpotLightInfo() const
{
	FSpotLightInfo Info;
	Info.Color = GetShaderColorWithIntensity();
	Info.AttenuationRadius = AttenuationRadius;
	Info.Position = GetWorldLocation();
	Info.CosInnerCone = std::cos(FMath::DegreesToRadians(InnerConeAngle));
	Info.CosOuterCone = std::cos(FMath::DegreesToRadians(OuterConeAngle));
	Info.Direction = GetLightDirection();
	return Info;
}

FLightSceneProxy* USpotLightComponent::CreateSceneProxy()
{
	return new FSpotLightSceneProxy(this);
}
