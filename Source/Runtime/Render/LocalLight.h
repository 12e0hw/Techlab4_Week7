#pragma once

#include "Render/LightInfo.h"
#include "Containers/Array.h"

class FLocalLight
{
public:
	void UploadLights(const TArray<FPointLightInfo>& PointLights, const TArray<FSpotLightInfo>& SpotLights);

	ID3D11ShaderResourceView* GetPointLightSRV() const { return PointLightSRV.Get(); }
	ID3D11ShaderResourceView* GetSpotLightSRV() const { return SpotLightSRV.Get(); }
	uint32 GetNumPointLights() const { return NumPointLights; }
	uint32 GetNumSpotLights() const { return NumSpotLights; }

private:
	bool EnsurePointLightCapacity(uint32 RequiredCount);
	bool EnsureSpotLightCapacity(uint32 RequiredCount);

	ComPtr<ID3D11Buffer> PointLightBuffer;
	ComPtr<ID3D11ShaderResourceView> PointLightSRV;
	ComPtr<ID3D11Buffer> SpotLightBuffer;
	ComPtr<ID3D11ShaderResourceView> SpotLightSRV;

	uint32 PointLightCapacity = 0;
	uint32 SpotLightCapacity = 0;

	uint32 NumPointLights = 0;
	uint32 NumSpotLights = 0;
};
