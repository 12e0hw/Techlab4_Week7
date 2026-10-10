#include "EnginePCH.h"
#include "PointLightComponent.h"

void UPointLightComponent::PostEditChangeProperty(const FProperty& Property)
{
	Super::PostEditChangeProperty(Property);

	// 음수 반경은 감쇠 계산을 뒤집으므로 막는다.
	if (AttenuationRadius < 0.0f)
		AttenuationRadius = 0.0f;
}
