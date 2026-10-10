#pragma once
#include "LightComponent.h"

class UAmbientLightComponent : public ULightComponent
{
	DECLARE_CLASS(UAmbientLightComponent, ULightComponent)
	REFLECT_START(UAmbientLightComponent)
		REFLECT_END()
public:
	UAmbientLightComponent()
	{
		Intensity = 0.2f;
		LightColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	virtual ~UAmbientLightComponent() override = default;

	virtual ELightType GetLightType() const override { return ELightType::Ambient; }
};

