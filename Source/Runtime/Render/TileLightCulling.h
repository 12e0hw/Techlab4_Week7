#pragma once

#include "Render/LightInfo.h"
#include "Containers/Array.h"
#include "LocalLight.h"

struct FCullConstants
{
    FMatrix InverseViewProjection;
    FVector CameraPosition;
    float Padding0 = 0.0f;
    FVector2 ScreenSize;
    uint32 TileCountX;
    uint32 TileCountY;
    uint32 NumPointLights;
    uint32 NumSpotLights;
    float Padding1[2] = {};
};

class FTileLightCulling
{
public:
    bool Init();
    void Dispatch(const FLocalLight& Lights, const FMatrix& ViewProjection, const FVector& CameraPosition, uint32 Width, uint32 Height);
    
    bool EnsureTileCapacity(uint32 Width, uint32 Height);
    void RenderDebugOverlay();

    ID3D11ShaderResourceView* GetPointTileListSRV() const { return PointTileListSRV.Get(); }
    ID3D11ShaderResourceView* GetSpotTileListSRV() const { return SpotTileListSRV.Get(); }
    uint32 GetTileCountX() const { return TileCountX; }
    uint32 GetTileCountY() const { return TileCountY; }
    ID3D11Buffer* GetCullConstantBuffer() const { return CullLightCB.Get(); }

private:
    bool CreatePointTileList(ID3D11Device* Device, uint32 TileCapacity);
    bool CreateSpotTileList(ID3D11Device* Device, uint32 TileCapacity);
    void ReleaseTileLists();

    ComPtr<ID3D11ComputeShader> CullLightsCS;
    ComPtr<ID3D11VertexShader> DebugVS;
    ComPtr<ID3D11PixelShader> DebugPS;

    ComPtr<ID3D11Buffer> PointTileListBuffer;
    ComPtr<ID3D11ShaderResourceView> PointTileListSRV;
    ComPtr<ID3D11UnorderedAccessView> PointTileListUAV;

    ComPtr<ID3D11Buffer> SpotTileListBuffer;
    ComPtr<ID3D11ShaderResourceView> SpotTileListSRV;
    ComPtr<ID3D11UnorderedAccessView> SpotTileListUAV;

    ComPtr<ID3D11Buffer> CullLightCB;

    uint32 TileCountX = 0;
    uint32 TileCountY = 0;
    uint32 TileCapacity = 0;
};
