#pragma once

#include "SceneComponent.h"
#include "Math/Color.h"
#include "Render/LightInfo.h"

class FLightSceneProxy;

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

	FVector GetLinearColorWithIntensity() const
	{
		const FLinearColor Linear = FLinearColor(LightColor) * Intensity;
		return FVector(Linear.R, Linear.G, Linear.B);
	}

	virtual FLightSceneProxy* CreateSceneProxy() { return nullptr; }
	FLightSceneProxy* GetSceneProxy() const { return SceneProxy; }
	void MarkSceneProxyDirty();

	virtual void PostEditChangeProperty(const FProperty& Property) override;
	virtual void OnTransformDirty() override;

protected:
	FColor LightColor = FColor(255, 255, 255);
	float Intensity = 1.0f;
	bool bVisible = true;

private:
	friend class FScene;
	FLightSceneProxy* SceneProxy = nullptr;
};
