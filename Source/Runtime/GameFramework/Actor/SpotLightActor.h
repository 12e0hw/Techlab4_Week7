#pragma once

#include "GameFramework/Actor.h"
#include "Component/BillboardComponent.h"
#include "Component/SpotLightComponent.h"

class ASpotLightActor final : public AActor
{
	DECLARE_CLASS(ASpotLightActor, AActor)

public:
	ASpotLightActor();
	virtual ~ASpotLightActor() override = default;

	USpotLightComponent* GetLightComponent() const { return LightComponent; }

private:
	USpotLightComponent* LightComponent = nullptr;
	UBillboardComponent* BillboardComponent = nullptr;
};
