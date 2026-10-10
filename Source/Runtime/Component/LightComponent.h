#pragma once

#include "SceneComponent.h"

enum class ELightType : uint8
{
	Directional,
	Point,
	Spot,
	Ambient
};

class ULightComponent : public USceneComponent
{
	DECLARE_CLASS(ULightComponent, USceneComponent)
	REFLECT_START(ULightComponent)
		PROPERTY(Intensity)
		PROPERTY_TYPE(LightColor, Color)
		PROPERTY(bIsEnabled)
	REFLECT_END()

public:
	ULightComponent() = default;
	virtual ~ULightComponent() override = default;

	float GetIntensity() const { return Intensity; }
	void SetIntensity(float InIntensity) { Intensity = InIntensity; }

	const FVector4& GetLightColor() const { return LightColor; }
	void SetLightColor(const FVector4& InColor) { LightColor = InColor; }

	bool IsLightEnabled() const { return bIsEnabled; }
	void SetLightEnabled(bool bInEnabled) { bIsEnabled = bInEnabled; }

	virtual ELightType GetLightType() const { return ELightType::Point; }

protected:
	float Intensity = 1.0f;
	FVector4 LightColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	bool bIsEnabled = true;
};