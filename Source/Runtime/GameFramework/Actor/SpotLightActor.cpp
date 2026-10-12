#include "EnginePCH.h"
#include "SpotLightActor.h"
#include "Core/ResourcePaths.h"

ASpotLightActor::ASpotLightActor()
{
	LightComponent = CreateDefaultSubobject<USpotLightComponent>("USpotLightComponent");
	SetRootComponent(LightComponent);

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("UBillboardComponent");
	BillboardComponent->SetupAttachment(LightComponent);
	BillboardComponent->SetHiddenInDetails(true);
	BillboardComponent->SetTexture(ResourcePaths::SpotLightIcon);
}
