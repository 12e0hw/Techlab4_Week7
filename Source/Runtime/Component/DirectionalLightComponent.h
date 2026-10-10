#pragma once

#include "LightComponent.h"

class UDirectionalLightComponent : public ULightComponent
{
	DECLARE_CLASS(UDirectionalLightComponent, ULightComponent)
	REFLECT_START(UDirectionalLightComponent)
		REFLECT_END()

public:
	UDirectionalLightComponent() = default;
	virtual ~UDirectionalLightComponent() override = default;

private:
};
