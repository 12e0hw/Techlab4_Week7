#include "EnginePCH.h"
#include "DirectionalLightComponent.h"
#include "Engine/LightSceneProxy.h"

FDirectionalLightInfo UDirectionalLightComponent::GetDirectionalLightInfo() const
{
	FDirectionalLightInfo Info;
	Info.Color = GetLinearColorWithIntensity();
	Info.Direction = GetLightDirection();
	return Info;
}

FLightSceneProxy* UDirectionalLightComponent::CreateSceneProxy()
{
	return new FDirectionalLightSceneProxy(this);
}
