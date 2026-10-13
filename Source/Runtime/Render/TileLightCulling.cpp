#include "EnginePCH.h"

#include "RenderCommand.h"
#include "TileLightCulling.h"
#include "RenderUtil.h"

namespace
{
	constexpr uint32 TileSize = 16;
	constexpr uint32 MaxPointLightsPerTile = 64;
	constexpr uint32 MaxSpotLightsPerTile = 64;
	constexpr uint32 PointTileStride = MaxPointLightsPerTile + 1;
	constexpr uint32 SpotTileStride = MaxSpotLightsPerTile + 1;
}

void FTileLightCulling::Init()
{
	CullingConstantBuffer = RenderCommand::CreateConstantBuffer(sizeof(FTileLightCullingConstants));

	FString CSOPath;
	FShaderByteCode ByteCode = RenderUtil::GetOrCompile("Resources/Shader/TileLightCulling.hlsl", "mainCS", EShaderType::Compute, CSOPath);

	if (!ByteCode.IsValid())
	{
		HTR_LOG(Error, "TileLightCulling shader compile failed");
		return;
	}

	ID3D11Device* Device = RenderCommand::GetDevice();
	if (FAILED(Device->CreateComputeShader(ByteCode.GetData(), ByteCode.GetSize(), nullptr, CullLightsCS.GetAddressOf())))
	{
		HTR_LOG(Error, "TileLightCulling shader creation failed");
		return;
	}
}

void FTileLightCulling::Dispatch(const FLocalLight& Lights, const FMatrix& ViewProjection, uint32 Width, uint32 Height, bool bVisualizeLightCount)
{
	if (!CullingConstantBuffer || !CullLightsCS || !PointTileListUAV || !SpotTileListUAV)
		return;

	// 이전 View에서 PS가 읽던 TileList를 이번 Dispatch의 UAV로 다시 쓰기 전에 해제한다.
	UnbindForShading();

	FTileLightCullingConstants Constants{};
	Constants.InverseViewProjection = ViewProjection.Inverse();
	Constants.ScreenWidth = Width;
	Constants.ScreenHeight = Height;
	Constants.TileCountX = TileCountX;
	Constants.TileCountY = TileCountY;
	Constants.NumPointLights = Lights.GetNumPointLights();
	Constants.NumSpotLights = Lights.GetNumSpotLights();
	Constants.PointTileStride = PointTileStride;
	Constants.SpotTileStride = SpotTileStride;
	Constants.bVisualizeLightCount = bVisualizeLightCount ? 1u : 0u;
	RenderCommand::UpdateBufferData(CullingConstantBuffer.get(), &Constants);
	RenderCommand::BindConstantBuffer(0, CullingConstantBuffer.get(), EShaderBindFlagBits::Compute);

	ID3D11DeviceContext* Context = RenderCommand::GetContext();

	Context->CSSetShader(CullLightsCS.Get(), nullptr, 0);

	ID3D11ShaderResourceView* LightSRVs[2] = { Lights.GetPointLightSRV(), Lights.GetSpotLightSRV() };
	Context->CSSetShaderResources(0, 2, LightSRVs);

	ID3D11UnorderedAccessView* LightUAVs[2] = { PointTileListUAV.Get(), SpotTileListUAV.Get()};
	Context->CSSetUnorderedAccessViews(0, 2, LightUAVs, nullptr);

	Context->Dispatch(TileCountX, TileCountY, 1);

	ID3D11Buffer* NullBuffer = nullptr;

	ID3D11ShaderResourceView* NullSRVs[2] = {};
	ID3D11UnorderedAccessView* NullUAVs[2] = {};
	Context->CSSetShaderResources(0, 2, NullSRVs);
	Context->CSSetUnorderedAccessViews(0, 2, NullUAVs, nullptr);
	Context->CSSetConstantBuffers(0, 1, &NullBuffer);
	Context->CSSetShader(nullptr, nullptr, 0);

	BindForShading(Lights);
}

void FTileLightCulling::BindForShading(const FLocalLight& Lights)
{
	ID3D11ShaderResourceView* SRVs[4] = {
		Lights.GetPointLightSRV(),
		Lights.GetSpotLightSRV(),
		PointTileListSRV.Get(),
		SpotTileListSRV.Get()
	};
	RenderCommand::GetContext()->PSSetShaderResources(8, 4, SRVs);
	RenderCommand::BindConstantBuffer(4, CullingConstantBuffer.get(), EShaderBindFlagBits::Pixel);
}

void FTileLightCulling::UnbindForShading()
{
	ID3D11ShaderResourceView* NullSRVs[4] = {};
	ID3D11Buffer* NullBuffer = nullptr;
	ID3D11DeviceContext* Context = RenderCommand::GetContext();
	Context->PSSetShaderResources(8, 4, NullSRVs);
	Context->PSSetConstantBuffers(4, 1, &NullBuffer);
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
