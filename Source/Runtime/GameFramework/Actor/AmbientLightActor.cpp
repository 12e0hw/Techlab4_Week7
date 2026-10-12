#include "EnginePCH.h"
#include "AmbientLightActor.h"
#include "Core/ResourcePaths.h"

AAmbientLightActor::AAmbientLightActor()
{
	LightComponent = CreateDefaultSubobject<UAmbientLightComponent>("UAmbientLightComponent");
	SetRootComponent(LightComponent);

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("UBillboardComponent");
	BillboardComponent->SetupAttachment(LightComponent);
	BillboardComponent->SetHiddenInDetails(true);
	BillboardComponent->SetTexture(ResourcePaths::SkyLightIcon);
}
