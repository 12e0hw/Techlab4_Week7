#pragma once

#include "LightComponentBase.h"

class ULightComponent : public ULightComponentBase
{
	DECLARE_CLASS(ULightComponent, ULightComponentBase)

public:
	ULightComponent() = default;
	virtual ~ULightComponent() override = default;

private:
};
