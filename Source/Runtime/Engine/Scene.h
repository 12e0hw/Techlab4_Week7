#pragma once

#include "PrimitiveSceneProxy.h"
#include "Component/BillboardComponent.h"
#include "Component/PrimitiveComponent.h"
#include "Math/Frustum.h"
#include "Math/BVH.h"
#include "Render/FogInfo.h"
#include "Render/LightInfo.h"
#include "Render/Renderer.h"

class FLightSceneProxy;
class ULightComponentBase;

struct FExponentialHeightFogSceneInfo
{
	uint32 Id;
	FFogInfo FogInfo;
};

class FScene
{
public:
	void AddPrimitive(UPrimitiveComponent* Component);
	void RemovePrimitive(UPrimitiveComponent* Component);
	// 모든 프록시를 한 번에 지운다. 액터를 통째로 지우기 전(ClearWorld)에 불러야 지워진 컴포넌트를 가리키는 프록시가 남지 않는다.
	void RemoveAllPrimitives();

	void AddFogInfo(uint32 Id, const FFogInfo& FogInfo);
	void RemoveFogInfo(uint32 Id);
	void RemoveAllFogInfo();
	void UpdateFogInfo(uint32 Id, const FFogInfo& FogInfo);

	void AddLight(ULightComponentBase* Component);
	void RemoveLight(ULightComponentBase* Component);
	void RemoveAllLights();
	void MarkLightDirty(FLightSceneProxy* Proxy);
	// Ambient·Directional은 마지막으로 등록된 것 하나, Point·Spot은 MaxPointLights·MaxSpotLights개까지 담는다.
	void BuildLightingConstants(FLightingConstants& Out) const;

	void UpdateAllTransforms();

	void BuildBVH();

	void MarkDirty(FPrimitiveSceneProxy* Proxy);
	void MarkRenderStateDirty(FPrimitiveSceneProxy* Proxy);

	void BuildStructuredLight(TArray<FPointLightInfo>& OutPointLight, TArray<FSpotLightInfo>& OutSpotLight) const;

	TArray<FPrimitiveSceneProxy*> Proxies;
	TArray<FPrimitiveSceneProxy*> DirtyProxies;
	TArray<FPrimitiveSceneProxy*> RenderStateDirtyProxies;
	TArray<FAABB> PrimitiveBounds;
	TArray<uint8> PrimitiveFlags;
	
	TArray<FExponentialHeightFogSceneInfo> FogInfos;

	TArray<FLightSceneProxy*> Lights;
	TArray<FLightSceneProxy*> DirtyLights;
	// 프록시에 담긴 라이트 색이 어느 작업 흐름으로 계산됐는지. ColorWorkflow::bLinear와 다르면 전부 다시 계산한다.
	bool bLightColorsLinear = true;

	// 컬링 결과로 프록시를 바로 내보내 컴포넌트를 역참조하지 않는다. 경계는 Build/Refit 때만 계산한다.
	TBVH<FPrimitiveSceneProxy*> BVH{
		[](const FPrimitiveSceneProxy* Proxy) -> FBox
		{
			const FAABB& B = Proxy->GetBounds();
			return FBox{ B.Center - B.Extent, B.Center + B.Extent };
		}
	};
	bool bElementListChanged = false;
};