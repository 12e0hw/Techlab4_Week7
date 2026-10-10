#include "EnginePCH.h"
#include "SpotLightComponent.h"

void USpotLightComponent::PostEditChangeProperty(const FProperty& Property)
{
	Super::PostEditChangeProperty(Property);

	// 언리얼처럼 Outer는 1~80도로, Inner는 0~Outer로 제한한다. Inner가 Outer보다 크면 경계 계산이 뒤집힌다.
	OuterConeAngle = FMath::Clamp(OuterConeAngle, 1.0f, 80.0f);
	InnerConeAngle = FMath::Clamp(InnerConeAngle, 0.0f, OuterConeAngle);
}
