#include "EnginePCH.h"
#include "Scene.h"
#include "Component/DirectionalLightComponent.h"
#include "Component/PointLightComponent.h"
#include "Component/SpotLightComponent.h"
#include "Component/AmbientLightComponent.h"

void FScene::AddPrimitive(UPrimitiveComponent* Component)
{
	if (!Component || Component->SceneProxy) return;

	FPrimitiveSceneProxy* Proxy = new FPrimitiveSceneProxy(Component);
	Proxy->Scene = this;
	Proxy->PackedIndex = Proxies.Num();
	Component->SceneProxy = Proxy;
	MarkDirty(Proxy);
	MarkRenderStateDirty(Proxy);

	Proxies.Add(Proxy);
	PrimitiveBounds.Add(FAABB{});
	PrimitiveFlags.Add(0);

	bElementListChanged = true;
}

void FScene::RemovePrimitive(UPrimitiveComponent* Component)
{
	if (!Component || !Component->SceneProxy) return;

	FPrimitiveSceneProxy* Proxy = Component->SceneProxy;
	if (!Proxy) return;

	const uint32 Index = static_cast<uint32>(Proxy->PackedIndex);

	Proxies.RemoveAtSwap(Index);
	PrimitiveBounds.RemoveAtSwap(Index);
	PrimitiveFlags.RemoveAtSwap(Index);

	if (Index < static_cast<uint32>(Proxies.Num()))
		Proxies[Index]->PackedIndex = static_cast<int32>(Index);

	if (Proxy->bQueuedForUpdate)
	{
		const int32 Found = DirtyProxies.Find(Proxy);   // 프로젝트 TArray의 Find 이름에 맞게
		if (Found != INDEX_NONE) DirtyProxies.RemoveAtSwap(Found);
	}

	if (Proxy->bRenderStateQueued)
	{
		const int32 Found = RenderStateDirtyProxies.Find(Proxy);
		if (Found != INDEX_NONE) RenderStateDirtyProxies.RemoveAtSwap(Found);
	}

	Component->SceneProxy = nullptr;
	delete Proxy;

	bElementListChanged = true;
}

void FScene::RemoveAllPrimitives()
{
	// 하나씩 RemovePrimitive하면 대기열 검색(Find)이 물체 수만큼 반복되므로 통째로 비운다.
	for (FPrimitiveSceneProxy* Proxy : Proxies)
	{
		if (UPrimitiveComponent* Component = Proxy->GetComponent())
			Component->SceneProxy = nullptr;
		delete Proxy;
	}
	Proxies.Reset();
	PrimitiveBounds.Reset();
	PrimitiveFlags.Reset();
	DirtyProxies.Reset();
	RenderStateDirtyProxies.Reset();
	BVH.Clear();
	bElementListChanged = true;
}

void FScene::AddFogInfo(uint32 Id, const FFogInfo& FogInfo)
{
	FExponentialHeightFogSceneInfo sceneFogInfo;
	sceneFogInfo.Id = Id;
	sceneFogInfo.FogInfo = FogInfo;


	FogInfos.Add(sceneFogInfo);
}

void FScene::RemoveFogInfo(uint32 Id)
{
	// 렌더러는 FogInfos[0]만 쓰므로, 순서를 유지하려고 RemoveAtSwap 대신 RemoveAt을 쓴다.
	for (int32 i = 0; i < FogInfos.Num(); ++i)
	{
		if (FogInfos[i].Id == Id)
		{
			FogInfos.RemoveAt(i);
			return;
		}
	}
}

void FScene::RemoveAllFogInfo()
{
	FogInfos.Reset();

}

void FScene::UpdateFogInfo(uint32 Id, const FFogInfo& FogInfo)
{
	for (FExponentialHeightFogSceneInfo& SceneFogInfo : FogInfos)
	{
		if (SceneFogInfo.Id == Id)
		{
			SceneFogInfo.FogInfo = FogInfo;
			return;
		}


	}
}

//void FScene::RemoveFogInfo(uint32 Id)
//{
//	for (int32 i = 0; i < FogInfos.Num(); ++i)
//	{
//		if (FogInfos[i].Id == Id)
//		{
//			FogInfos.RemoveAtSwap(i);
//			return;
//		}
//	}
//}


void FScene::UpdateAllTransforms()
{
	for (FPrimitiveSceneProxy* Proxy : RenderStateDirtyProxies)
	{
		Proxy->UpdateRenderState();
		Proxy->bRenderStateQueued = false;
	}
	RenderStateDirtyProxies.Reset();

	for (FPrimitiveSceneProxy* Proxy : DirtyProxies)
	{
		Proxy->UpdateTransform();
		PrimitiveBounds[Proxy->PackedIndex] = Proxy->GetBounds();
		PrimitiveFlags[Proxy->PackedIndex] = Proxy->GetComponent()->IsVisible() ? 1 : 0;
		Proxy->bQueuedForUpdate = false;
	}
	
	//const int32 Count = Proxies.Num();
	//for (int32 i = 0; i < Count; ++i)
	//{
	//	FPrimitiveSceneProxy* Proxy = Proxies[i];
	//	Proxy->UpdateTransform();

	//	const FAABB& Bounds = Proxy->GetBounds();
	//	if (PrimitiveBounds[i].Center != Bounds.Center || PrimitiveBounds[i].Extent != Bounds.Extent)
	//	{
	//		bBoundsChanged = true;
	//	}

	//	PrimitiveBounds[i] = Proxy->GetBounds();
	//	PrimitiveFlags[i] = Proxy->GetComponent()->IsVisible() ? 1 : 0;
	//}

	const bool bAnyMoved = DirtyProxies.Num() > 0;
	DirtyProxies.Reset();
	

	if (bElementListChanged)
	{
		BuildBVH();
		bElementListChanged = false;
	}
	else if (bAnyMoved)
	{
		BVH.Refit();
	}
}

void FScene::BuildBVH()
{
	BVH.Clear();
	// BillboardComponents.Reset();

	TArray<FPrimitiveSceneProxy*> Elements;
	Elements.Reserve(Proxies.Num());
	for (FPrimitiveSceneProxy* Proxy : Proxies)
	{
		if (Proxy && Proxy->GetComponent())
			Elements.Add(Proxy);
	}

	BVH.Build(std::span(Elements.GetData(), Elements.Num()));
}

void FScene::MarkDirty(FPrimitiveSceneProxy* Proxy)
{
	if (Proxy->bQueuedForUpdate) return;            // 중복 추가 방지
	Proxy->bQueuedForUpdate = true;
	DirtyProxies.Add(Proxy);
}

void FScene::MarkRenderStateDirty(FPrimitiveSceneProxy* Proxy)
{
	if (!Proxy || Proxy->bRenderStateQueued) return;
	Proxy->bRenderStateQueued = true;
	RenderStateDirtyProxies.Add(Proxy);
}


void FScene::UpdateLights(FRenderer* Renderer)
{
	if (!Renderer)
		return;

	FLightConstants Constants{};

	// 1. 등록된 ULightComponent 순회
	for (ULightComponent* Light : LightComponents)
	{
		if (!Light || !Light->IsLightEnabled())
			continue;

		switch (Light->GetLightType())
		{
		case ELightType::Directional:
		{
			if (UDirectionalLightComponent* DirLight = Cast<UDirectionalLightComponent>(Light))
			{
				const FVector Dir = DirLight->GetLightDirection();
				Constants.DirectionalLight.Direction = FVector4(Dir.X, Dir.Y, Dir.Z, 1.0f);
				Constants.DirectionalLight.ColorIntensity = FVector4(
					DirLight->GetLightColor().X,
					DirLight->GetLightColor().Y,
					DirLight->GetLightColor().Z,
					DirLight->GetIntensity()
				);
				Constants.DirectionalLightCount = 1;
			}
			break;
		}
		case ELightType::Ambient:
		{
			if (UAmbientLightComponent* AmbLight = Cast<UAmbientLightComponent>(Light))
			{
				Constants.AmbientLight.ColorIntensity = FVector4(
					AmbLight->GetLightColor().X,
					AmbLight->GetLightColor().Y,
					AmbLight->GetLightColor().Z,
					AmbLight->GetIntensity()
				);
				Constants.AmbientLightCount = 1;
			}
			break;
		}
		case ELightType::Point:
		{
			if (Constants.PointLightCount < MaxPointLights)
			{
				if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(Light))
				{
					const FVector Pos = PointLight->GetWorldLocation();
					const FVector4 Col = PointLight->GetLightColor();
					FPointLightData& Data = Constants.PointLights[Constants.PointLightCount++];

					Data.PositionRadius = FVector4(Pos.X, Pos.Y, Pos.Z, std::max<float>(PointLight->GetAttenuationRadius(), 0.001f));
					Data.ColorIntensity = FVector4(Col.X, Col.Y, Col.Z, std::max<float>(PointLight->GetIntensity(), 0.0f));
					Data.FalloffEnabled = FVector4(std::max<float>(PointLight->GetFalloff(), 0.001f), 1.0f, 0.0f, 0.0f);
				}
			}
			break;
		}
		case ELightType::Spot:
		{
			if (Constants.SpotLightCount < MaxSpotLights)
			{
				if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(Light))
				{
					const FVector Pos = SpotLight->GetWorldLocation();
					FTransform WorldTransform;
					WorldTransform.Rotation = SpotLight->GetWorldRotation();
					const FVector Dir = WorldTransform.GetForward();
					const FVector4 Col = SpotLight->GetLightColor();

					const float InnerRad = FMath::DegreesToRadians(SpotLight->GetInnerConeAngle());
					const float OuterRad = FMath::DegreesToRadians(SpotLight->GetOuterConeAngle());

					FSpotLightData& Data = Constants.SpotLights[Constants.SpotLightCount++];
					Data.PositionRadius = FVector4(Pos.X, Pos.Y, Pos.Z, std::max<float>(SpotLight->GetAttenuationRadius(), 0.001f));
					Data.DirectionEnabled = FVector4(Dir.X, Dir.Y, Dir.Z, 1.0f);
					Data.ColorIntensity = FVector4(Col.X, Col.Y, Col.Z, std::max<float>(SpotLight->GetIntensity(), 0.0f));
					Data.ConeAnglesFalloff = FVector4(std::cosf(InnerRad), std::cosf(OuterRad), std::max<float>(SpotLight->GetFalloff(), 0.001f), 0.0f);
				}
			}
			break;
		}
		}
	}

	// 2. 레거시 FireBallComponents 호환 (PointLight로 등록)
	for (UFireBallComponent* FireBall : FireBallComponents)
	{
		if (Constants.PointLightCount >= MaxPointLights)
			break;
		if (!FireBall)
			continue;

		const FVector Pos = FireBall->GetWorldLocation();
		const FVector4 Col = FireBall->GetLightColor();
		FPointLightData& Data = Constants.PointLights[Constants.PointLightCount++];

		Data.PositionRadius = FVector4(Pos.X, Pos.Y, Pos.Z, std::max<float>(FireBall->GetRadius(), 0.001f));
		Data.ColorIntensity = FVector4(Col.X, Col.Y, Col.Z, std::max<float>(FireBall->GetIntensity(), 0.0f));
		Data.FalloffEnabled = FVector4(std::max<float>(FireBall->GetRadiusFalloff(), 0.001f), 1.0f, 0.0f, 0.0f);
	}

	Renderer->SetLightConstants(Constants);
}

void FScene::RegisterLight(ULightComponent* Light)
{
	if (!Light)
		return;

	for (ULightComponent* Existing : LightComponents)
	{
		if (Existing == Light)
			return;
	}
	LightComponents.Add(Light);
}

void FScene::UnregisterLight(ULightComponent* Light)
{
	for (uint32 i = 0; i < LightComponents.Num(); ++i)
	{
		if (LightComponents[i] == Light)
		{
			LightComponents.RemoveAt(i, 1);
			return;
		}
	}
}

void FScene::RegisterFireBall(UFireBallComponent* FireBall)
{
	if (!FireBall)
		return;

	for (UFireBallComponent* Existing : FireBallComponents)
	{
		if (Existing == FireBall)
			return;
	}
	FireBallComponents.Add(FireBall);
}
void FScene::UnregisterFireBall(UFireBallComponent* FireBall)
{
	for (uint32 i = 0; i < FireBallComponents.Num(); ++i)
	{
		if (FireBallComponents[i] == FireBall)
		{
			FireBallComponents.RemoveAt(i, 1);
			return;
		}
	}
}