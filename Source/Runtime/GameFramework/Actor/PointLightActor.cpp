#include "EnginePCH.h"
#include "PointLightActor.h"

APointLightActor::APointLightActor()
{
	LightComponent = CreateDefaultSubobject<UPointLightComponent>("UPointLightComponent");
	SetRootComponent(LightComponent);

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("UBillboardComponent");
	BillboardComponent->SetupAttachment(LightComponent);
	BillboardComponent->SetHiddenInDetails(true);
}
