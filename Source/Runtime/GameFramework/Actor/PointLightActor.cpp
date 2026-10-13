#include "EnginePCH.h"
#include "PointLightActor.h"
#include "Core/ResourcePaths.h"

APointLightActor::APointLightActor()
{
	LightComponent = CreateDefaultSubobject<UPointLightComponent>("UPointLightComponent");
	SetRootComponent(LightComponent);

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("UBillboardComponent");
	BillboardComponent->SetupAttachment(LightComponent);
	BillboardComponent->SetHiddenInDetails(false);
	BillboardComponent->SetTexture(ResourcePaths::PointLightIcon);
}
