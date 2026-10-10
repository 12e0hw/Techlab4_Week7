#pragma once

#include "LightComponentBase.h"

class UAmbientLightComponent : public ULightComponentBase
{
	DECLARE_CLASS(UAmbientLightComponent, ULightComponentBase)

public:
	UAmbientLightComponent() = default;
	virtual ~UAmbientLightComponent() override = default;
private:
};
