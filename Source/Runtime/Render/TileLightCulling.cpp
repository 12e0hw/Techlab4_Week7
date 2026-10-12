#include "EnginePCH.h"

#include "TileLightCulling.h"
#include "RenderCommand.h"

namespace
{
	constexpr uint32 TileSize = 16;
	constexpr uint32 MaxPointLightsPerTile = 64;
	constexpr uint32 MaxSpotLightsPerTile = 64;
	constexpr uint32 PointTileStride = MaxPointLightsPerTile + 1;
	constexpr uint32 SpotTileStride = MaxSpotLightsPerTile + 1;
}

bool FTileLightCulling::EnsureTileCapacity(uint32 Width, uint32 Height)
{
	TileCountX = (Width + TileSize - 1) / TileSize;
	TileCountY = (Height + TileSize - 1) / TileSize;

	uint32 RequiredTileCount = TileCountX * TileCountY;

	const bool bResourcesValid = 
		PointTileListBuffer && PointTileListSRV && PointTileListUAV 
		&& SpotTileListBuffer && SpotTileListSRV && SpotTileListUAV;

	if (RequiredTileCount <= TileCapacity && bResourcesValid)
	{
		return true;
	}

	uint32 NewCapacity = std::max(TileCapacity, 1024u);
	while (NewCapacity < RequiredTileCount)
	{
		NewCapacity *= 2;
	}

	ID3D11Device* Device = RenderCommand::GetDevice();

	ReleaseTileLists();
	if (!CreatePointTileList(Device, NewCapacity) || !CreateSpotTileList(Device, NewCapacity))
	{
		ReleaseTileLists();
		return false;
	}

	TileCapacity = NewCapacity;
	return true;
}

bool FTileLightCulling::CreatePointTileList(ID3D11Device* Device, uint32 TileCapacity)
{
	uint32  PointElementCount = TileCapacity * PointTileStride;

	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	BufferDesc.ByteWidth = PointElementCount * sizeof(uint32);
	BufferDesc.StructureByteStride = sizeof(uint32);
	if (FAILED(Device->CreateBuffer(&BufferDesc, nullptr, PointTileListBuffer.GetAddressOf())))
	{
		return false;
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
	UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	UAVDesc.Buffer.FirstElement = 0;
	UAVDesc.Buffer.NumElements = PointElementCount;
	UAVDesc.Buffer.Flags = 0;
	if (FAILED(Device->CreateUnorderedAccessView(PointTileListBuffer.Get(), &UAVDesc, PointTileListUAV.GetAddressOf())))
	{
		PointTileListBuffer.Reset();
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Buffer.NumElements = PointElementCount;
	if (FAILED(Device->CreateShaderResourceView(PointTileListBuffer.Get(), &SRVDesc, PointTileListSRV.GetAddressOf())))
	{
		PointTileListUAV.Reset();
		PointTileListBuffer.Reset();
		return false;
	}

	return true;
}

bool FTileLightCulling::CreateSpotTileList(ID3D11Device* Device, uint32 TileCapacity)
{
	uint32 SpotElementCount = TileCapacity * SpotTileStride;

	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	BufferDesc.ByteWidth = SpotElementCount * sizeof(uint32);
	BufferDesc.StructureByteStride = sizeof(uint32);
	if (FAILED(Device->CreateBuffer(&BufferDesc, nullptr, SpotTileListBuffer.GetAddressOf())))
	{
		return false;
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
	UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	UAVDesc.Buffer.FirstElement = 0;
	UAVDesc.Buffer.NumElements = SpotElementCount;
	UAVDesc.Buffer.Flags = 0;
	if (FAILED(Device->CreateUnorderedAccessView(SpotTileListBuffer.Get(), &UAVDesc, SpotTileListUAV.GetAddressOf())))
	{
		SpotTileListBuffer.Reset();
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Buffer.NumElements = SpotElementCount;
	if (FAILED(Device->CreateShaderResourceView(SpotTileListBuffer.Get(), &SRVDesc, SpotTileListSRV.GetAddressOf())))
	{
		SpotTileListUAV.Reset();
		SpotTileListBuffer.Reset();
		return false;
	}

	return true;
}

void FTileLightCulling::ReleaseTileLists()
{
	PointTileListBuffer.Reset();
	PointTileListSRV.Reset();
	PointTileListUAV.Reset();

	SpotTileListBuffer.Reset();
	SpotTileListSRV.Reset();
	SpotTileListUAV.Reset();

	TileCapacity = 0;
}