#include "EnginePCH.h"
#include "DirectionalLightActor.h"
#include "Core/ResourcePaths.h"

ADirectionalLightActor::ADirectionalLightActor()
{
	LightComponent = CreateDefaultSubobject<UDirectionalLightComponent>("UDirectionalLightComponent");
	SetRootComponent(LightComponent);

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("UBillboardComponent");
	BillboardComponent->SetupAttachment(LightComponent);
	BillboardComponent->SetHiddenInDetails(true);
	BillboardComponent->SetTexture(ResourcePaths::DirectionalLightIcon);
}
