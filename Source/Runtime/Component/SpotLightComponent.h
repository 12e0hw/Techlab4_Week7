#pragma once

#include "PointLightComponent.h"

class USpotLightComponent : public UPointLightComponent
{
	DECLARE_CLASS(USpotLightComponent, UPointLightComponent)
	REFLECT_START(USpotLightComponent)
		PROPERTY(InnerConeAngle)
		PROPERTY(OuterConeAngle)
	REFLECT_END()

public:
	USpotLightComponent() = default;
	virtual ~USpotLightComponent() override = default;

	float GetInnerConeAngle() const { return InnerConeAngle; }
	float GetOuterConeAngle() const { return OuterConeAngle; }

	FSpotLightInfo GetSpotLightInfo() const;
	virtual FLightSceneProxy* CreateSceneProxy() override;

	virtual void PostEditChangeProperty(const FProperty& Property) override;

protected:
	float InnerConeAngle = 0.0f;
	float OuterConeAngle = 44.0f;
};
