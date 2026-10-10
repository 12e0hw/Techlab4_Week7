#pragma once

#include "LightComponent.h"

class UAmbientLightComponent : public ULightComponent
{
	DECLARE_CLASS(UAmbientLightComponent, ULightComponent)

public:
	UAmbientLightComponent() = default;
	virtual ~UAmbientLightComponent() override = default;
private:
};
