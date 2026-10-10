#include "EnginePCH.h"
#include "AmbientLightActor.h"

AAmbientLightActor::AAmbientLightActor()
{
	LightComponent = CreateDefaultSubobject<UAmbientLightComponent>("UAmbientLightComponent");
	SetRootComponent(LightComponent);

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("UBillboardComponent");
	BillboardComponent->SetupAttachment(LightComponent);
	BillboardComponent->SetHiddenInDetails(true);
}
