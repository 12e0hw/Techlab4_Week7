#pragma once
#include "LightComponent.h"

class UDirectionalLightComponent : public ULightComponent
{
	DECLARE_CLASS(UDirectionalLightComponent, ULightComponent)
	REFLECT_START(UDirectionalLightComponent)
	REFLECT_END()
public:
	UDirectionalLightComponent()
	{
		Intensity = 1.0f;
		LightColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	virtual ~UDirectionalLightComponent() override = default;

	virtual ELightType GetLightType() const override { return ELightType::Directional; }

	// 빛이 나아가는 방향 (Transform의 Forward 벡터)
	FVector GetLightDirection() const
	{
		FTransform WorldTransform;
		WorldTransform.Rotation = GetWorldRotation();
		return WorldTransform.GetForward();
	}
};

