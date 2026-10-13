#pragma once

#include "Render/Buffer.h"

#include "Render/LightInfo.h"
#include "Containers/Array.h"
#include "LocalLight.h"

struct FTileLightCullingConstants
{
    FMatrix InverseViewProjection;
    uint32 ScreenWidth;
    uint32 ScreenHeight;
    uint32 TileCountX;
    uint32 TileCountY;
    uint32 NumPointLights;
    uint32 NumSpotLights;
    uint32 PointTileStride;
    uint32 SpotTileStride;
    uint32 bVisualizeLightCount;
    uint32 Padding[3];
};

class FTileLightCulling
{
public:
    void Init();
    void Dispatch(const FLocalLight& Lights, const FMatrix& ViewProjection, uint32 Width, uint32 Height, bool bVisualizeLightCount);
    void BindForShading(const FLocalLight& Lights);
    void UnbindForShading();
    
    bool EnsureTileCapacity(uint32 Width, uint32 Height);
    ID3D11ShaderResourceView* GetPointTileListSRV() const { return PointTileListSRV.Get(); }
    ID3D11ShaderResourceView* GetSpotTileListSRV() const { return SpotTileListSRV.Get(); }
    uint32 GetTileCountX() const { return TileCountX; }
    uint32 GetTileCountY() const { return TileCountY; }

private:
    bool CreatePointTileList(ID3D11Device* Device, uint32 TileCapacity);
    bool CreateSpotTileList(ID3D11Device* Device, uint32 TileCapacity);
    void ReleaseTileLists();

    TUniquePtr<FConstantBuffer> CullingConstantBuffer;

    ComPtr<ID3D11ComputeShader> CullLightsCS;

    ComPtr<ID3D11Buffer> PointTileListBuffer;
    ComPtr<ID3D11ShaderResourceView> PointTileListSRV;
    ComPtr<ID3D11UnorderedAccessView> PointTileListUAV;

    ComPtr<ID3D11Buffer> SpotTileListBuffer;
    ComPtr<ID3D11ShaderResourceView> SpotTileListSRV;
    ComPtr<ID3D11UnorderedAccessView> SpotTileListUAV;

    uint32 TileCountX = 0;
    uint32 TileCountY = 0;
    uint32 TileCapacity = 0;
};
