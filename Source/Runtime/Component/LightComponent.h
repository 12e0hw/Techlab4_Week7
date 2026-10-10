#pragma once

#include "LightComponentBase.h"

class ULightComponent : public ULightComponentBase
{
	DECLARE_CLASS(ULightComponent, ULightComponentBase)

public:
	ULightComponent() = default;
	virtual ~ULightComponent() override = default;

	FVector GetLightDirection() const
	{
		const FMatrix World = GetWorldMatrix();
		return FVector(World[0][0], World[0][1], World[0][2]).GetSafeNormal();
	}

private:
};
