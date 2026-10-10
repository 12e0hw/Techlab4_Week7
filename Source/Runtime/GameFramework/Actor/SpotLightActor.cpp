#include "EnginePCH.h"
#include "SpotLightActor.h"

ASpotLightActor::ASpotLightActor()
{
	LightComponent = CreateDefaultSubobject<USpotLightComponent>("USpotLightComponent");
	SetRootComponent(LightComponent);

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("UBillboardComponent");
	BillboardComponent->SetupAttachment(LightComponent);
	BillboardComponent->SetHiddenInDetails(true);
}
