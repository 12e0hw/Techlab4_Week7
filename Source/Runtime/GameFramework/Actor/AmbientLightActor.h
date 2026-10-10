#pragma once

#include "GameFramework/Actor.h"
#include "Component/BillboardComponent.h"
#include "Component/AmbientLightComponent.h"

class AAmbientLightActor final : public AActor
{
	DECLARE_CLASS(AAmbientLightActor, AActor)

public:
	AAmbientLightActor();
	virtual ~AAmbientLightActor() override = default;

	UAmbientLightComponent* GetLightComponent() const { return LightComponent; }

private:
	UAmbientLightComponent* LightComponent = nullptr;
	UBillboardComponent* BillboardComponent = nullptr;
};
