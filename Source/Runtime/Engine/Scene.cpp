#include "EnginePCH.h"
#include "Scene.h"
#include "LightSceneProxy.h"
#include "Render/ColorWorkflow.h"

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

void FScene::AddLight(ULightComponentBase* Component)
{
	if (!Component || Component->SceneProxy) return;

	FLightSceneProxy* Proxy = Component->CreateSceneProxy();
	if (!Proxy) return;

	Proxy->Scene = this;
	Component->SceneProxy = Proxy;
	// 다음 UpdateAllTransforms 전에 그려져도 기본값이 아닌 실제 값을 쓰도록 바로 채운다.
	Proxy->UpdateFromComponent();
	Lights.Add(Proxy);
}

void FScene::RemoveLight(ULightComponentBase* Component)
{
	if (!Component || !Component->SceneProxy) return;

	FLightSceneProxy* Proxy = Component->SceneProxy;

	const int32 Index = Lights.Find(Proxy);
	// Ambient·Directional은 등록 순서상 마지막 것을 쓰므로 순서를 유지한다.
	if (Index != INDEX_NONE) Lights.RemoveAt(Index);

	if (Proxy->bQueuedForUpdate)
	{
		const int32 Found = DirtyLights.Find(Proxy);
		if (Found != INDEX_NONE) DirtyLights.RemoveAtSwap(Found);
	}

	Component->SceneProxy = nullptr;
	delete Proxy;
}

void FScene::RemoveAllLights()
{
	for (FLightSceneProxy* Proxy : Lights)
	{
		Proxy->GetComponent()->SceneProxy = nullptr;
		delete Proxy;
	}
	Lights.Reset();
	DirtyLights.Reset();
}

void FScene::MarkLightDirty(FLightSceneProxy* Proxy)
{
	if (Proxy->bQueuedForUpdate) return;
	Proxy->bQueuedForUpdate = true;
	DirtyLights.Add(Proxy);
}

void FScene::BuildLightingConstants(FLightingConstants& Out) const
{
	Out = FLightingConstants{};

	for (const FLightSceneProxy* Proxy : Lights)
	{
		if (!Proxy->IsVisible()) continue;

		switch (Proxy->GetLightType())
		{
		case ELightType::Ambient:
			Out.Ambient.Color = static_cast<const FAmbientLightSceneProxy*>(Proxy)->GetInfo().Color;
			break;
		case ELightType::Directional:
			Out.Directional = static_cast<const FDirectionalLightSceneProxy*>(Proxy)->GetInfo();
			break;
		case ELightType::Point:
			if (Out.NumPointLights < MaxPointLights)
				Out.PointLights[Out.NumPointLights++] = static_cast<const FPointLightSceneProxy*>(Proxy)->GetInfo();
			break;
		case ELightType::Spot:
			if (Out.NumSpotLights < MaxSpotLights)
				Out.SpotLights[Out.NumSpotLights++] = static_cast<const FSpotLightSceneProxy*>(Proxy)->GetInfo();
			break;
		}
	}
}

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

	if (bLightColorsLinear != ColorWorkflow::bLinear)
	{
		bLightColorsLinear = ColorWorkflow::bLinear;
		for (FLightSceneProxy* Proxy : Lights)
			MarkLightDirty(Proxy);
	}

	for (FLightSceneProxy* Proxy : DirtyLights)
	{
		Proxy->UpdateFromComponent();
		Proxy->bQueuedForUpdate = false;
	}
	DirtyLights.Reset();

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

void FScene::BuildStructuredLight(TArray<FPointLightInfo>& OutPointLight, TArray<FSpotLightInfo>& OutSpotLight) const
{
	OutPointLight.Reset();
	OutSpotLight.Reset();

	for (FLightSceneProxy* Proxy : Lights)
	{
		if (!Proxy->IsVisible())
		{
			continue;
		}

		switch (Proxy->GetLightType())
		{
		case ELightType::Point:
			OutPointLight.Add(static_cast<FPointLightSceneProxy*>(Proxy)->GetInfo());
			break;
		case ELightType::Spot:
			OutSpotLight.Add(static_cast<FSpotLightSceneProxy*>(Proxy)->GetInfo());
			break;
		default:
			break;
		}
	}

}
