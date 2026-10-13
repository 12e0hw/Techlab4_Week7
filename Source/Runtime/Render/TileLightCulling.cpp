#include "EnginePCH.h"

#include "TileLightCulling.h"
#include "RenderCommand.h"
#include "Core/EngineLog.h"

namespace
{
	constexpr uint32 TileSize = 16;
	constexpr uint32 MaxPointLightsPerTile = 64;
	constexpr uint32 MaxSpotLightsPerTile = 64;
	constexpr uint32 PointTileStride = MaxPointLightsPerTile + 1;
	constexpr uint32 SpotTileStride = MaxSpotLightsPerTile + 1;
}

bool FTileLightCulling::Init()
{
	ID3D11Device* Device = RenderCommand::GetDevice();
	ComPtr<ID3DBlob> ShaderBlob;
	ComPtr<ID3DBlob> ErrorBlob;

	HRESULT hr = D3DCompileFromFile(
		L"Resources/Shader/TileLightCulling.hlsl", 
		nullptr,                                 
		D3D_COMPILE_STANDARD_FILE_INCLUDE,        
		"CullLightsCS",                           
		"cs_5_0",                                
		D3DCOMPILE_ENABLE_STRICTNESS,             
		0,                                      
		ShaderBlob.GetAddressOf(),               
		ErrorBlob.GetAddressOf()                  
	);

	if (FAILED(hr))
	{
		if (ErrorBlob)
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
		}
		return false;
	}

	hr = Device->CreateComputeShader(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		CullLightsCS.GetAddressOf()
	);
	if (FAILED(hr)) return false;

	// Compile Debug VS
	ComPtr<ID3DBlob> VSBlob;
	ErrorBlob.Reset();
	hr = D3DCompileFromFile(
		L"Resources/Shader/TileDebugShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"mainVS",
		"vs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		VSBlob.GetAddressOf(),
		ErrorBlob.GetAddressOf()
	);
	if (FAILED(hr))
	{
		if (ErrorBlob)
		{
			HTR_LOG(Error, "[TileDebugShader] VS compile failed: {}", (const char*)ErrorBlob->GetBufferPointer());
		}
		return false;
	}
	hr = Device->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), nullptr, DebugVS.GetAddressOf());
	if (FAILED(hr)) return false;

	// Compile Debug PS
	ComPtr<ID3DBlob> PSBlob;
	ErrorBlob.Reset();
	hr = D3DCompileFromFile(
		L"Resources/Shader/TileDebugShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"mainPS",
		"ps_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		PSBlob.GetAddressOf(),
		ErrorBlob.GetAddressOf()
	);
	if (FAILED(hr))
	{
		if (ErrorBlob)
		{
			HTR_LOG(Error, "[TileDebugShader] PS compile failed: {}", (const char*)ErrorBlob->GetBufferPointer());
		}
		return false;
	}
	hr = Device->CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), nullptr, DebugPS.GetAddressOf());
	if (FAILED(hr)) return false;

	D3D11_BUFFER_DESC Desc{};
	Desc.ByteWidth = sizeof(FCullConstants);
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Device->CreateBuffer(&Desc, nullptr, CullLightCB.GetAddressOf());

	return true;
}

void FTileLightCulling::RenderDebugOverlay()
{
	if (!DebugVS || !DebugPS || !PointTileListSRV || !SpotTileListSRV)
		return;

	ID3D11DeviceContext* Context = RenderCommand::GetContext();

	Context->VSSetShader(DebugVS.Get(), nullptr, 0);
	Context->PSSetShader(DebugPS.Get(), nullptr, 0);

	Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Context->PSSetConstantBuffers(0, 1, CullLightCB.GetAddressOf());
	ID3D11ShaderResourceView* SRVs[] = { PointTileListSRV.Get(), SpotTileListSRV.Get() };
	Context->PSSetShaderResources(0, 2, SRVs);

	Context->Draw(3, 0);

	ID3D11ShaderResourceView* NullSRVs[] = { nullptr, nullptr };
	Context->PSSetShaderResources(0, 2, NullSRVs);
	ID3D11Buffer* NullCB = nullptr;
	Context->PSSetConstantBuffers(0, 1, &NullCB);
	Context->VSSetShader(nullptr, nullptr, 0);
	Context->PSSetShader(nullptr, nullptr, 0);
}

void FTileLightCulling::Dispatch(const FLocalLight& Lights, const FMatrix& ViewProjection, const FVector& CameraPosition, uint32 Width, uint32 Height)
{
	ID3D11DeviceContext* Context = RenderCommand::GetContext();
	Context->CSSetConstantBuffers(0, 1, CullLightCB.GetAddressOf());
	Context->CSSetShader(CullLightsCS.Get(), nullptr, 0);

	FCullConstants Constants{};
	Constants.InverseViewProjection = ViewProjection.Inverse();
	Constants.CameraPosition = CameraPosition;
	Constants.ScreenSize = FVector2(static_cast<float>(Width), static_cast<float>(Height));
	Constants.NumPointLights = Lights.GetNumPointLights();
	Constants.NumSpotLights = Lights.GetNumSpotLights();
	Constants.TileCountX = TileCountX;
	Constants.TileCountY = TileCountY;
	Context->UpdateSubresource(CullLightCB.Get(), 0, nullptr, &Constants, 0, 0);

	ID3D11ShaderResourceView* SRVs[] = {
	Lights.GetPointLightSRV(),
	Lights.GetSpotLightSRV(),
	// DepthSRV 
	};
	Context->CSSetShaderResources(0, 2, SRVs);

	ID3D11UnorderedAccessView* UAVs[] = {
	PointTileListUAV.Get(),
	SpotTileListUAV.Get()
	};

	Context->CSSetUnorderedAccessViews(0, 2, UAVs, nullptr);

	Context->Dispatch(TileCountX, TileCountY, 1);

	ID3D11UnorderedAccessView* NullUAV[] = { nullptr, nullptr };
	Context->CSSetUnorderedAccessViews(0, 2, NullUAV, nullptr);
	ID3D11ShaderResourceView* NullSRVs[] = { nullptr, nullptr };
	Context->CSSetShaderResources(0, 2, NullSRVs);
	Context->CSSetShader(nullptr, nullptr, 0);
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