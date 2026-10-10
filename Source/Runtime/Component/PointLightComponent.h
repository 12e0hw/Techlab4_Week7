#pragma once

#include "LightComponent.h"

class UPointLightComponent : public ULightComponent
{
	DECLARE_CLASS(UPointLightComponent, ULightComponent)
	REFLECT_START(UPointLightComponent)
		PROPERTY(AttenuationRadius)
	REFLECT_END()

public:
	UPointLightComponent() = default;
	virtual ~UPointLightComponent() override = default;

	float GetAttenuationRadius() const { return AttenuationRadius; }

	virtual void PostEditChangeProperty(const FProperty& Property) override;

protected:
	float AttenuationRadius = 10.0f;
};
