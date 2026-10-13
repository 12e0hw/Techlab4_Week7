#include "EnginePCH.h"
#include "AmbientLightComponent.h"
#include "Engine/LightSceneProxy.h"

FAmbientLightInfo UAmbientLightComponent::GetAmbientLightInfo() const
{
	FAmbientLightInfo Info;
	Info.Color = GetShaderColorWithIntensity();
	return Info;
}

FLightSceneProxy* UAmbientLightComponent::CreateSceneProxy()
{
	return new FAmbientLightSceneProxy(this);
}
