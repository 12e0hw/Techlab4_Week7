#pragma once

#include "SceneComponent.h"

class ULightComponentBase : public USceneComponent
{
	DECLARE_CLASS(ULightComponentBase, USceneComponent)
	REFLECT_START(ULightComponentBase)
		PROPERTY(LightColor)
		PROPERTY(Intensity)
		PROPERTY(bVisible)
	REFLECT_END()

public:
	ULightComponentBase() = default;
	virtual ~ULightComponentBase() override = default;

	const FColor& GetLightColor() const { return LightColor; }
	void SetLightColor(const FColor& InColor) { LightColor = InColor; }

	float GetIntensity() const { return Intensity; }
	void SetIntensity(float InIntensity) { Intensity = InIntensity; }

	bool IsLightVisible() const { return bVisible; }
	void SetLightVisible(bool bInVisible) { bVisible = bInVisible; }

protected:
	FColor LightColor = FColor(255, 255, 255);
	float Intensity = 1.0f;
	bool bVisible = true;
};
