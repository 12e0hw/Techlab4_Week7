#pragma once

#include "GameFramework/Actor.h"
#include "Component/BillboardComponent.h"
#include "Component/PointLightComponent.h"

class APointLightActor final : public AActor
{
	DECLARE_CLASS(APointLightActor, AActor)

public:
	APointLightActor();
	virtual ~APointLightActor() override = default;

	UPointLightComponent* GetLightComponent() const { return LightComponent; }

private:
	UPointLightComponent* LightComponent = nullptr;
	UBillboardComponent* BillboardComponent = nullptr;
};
