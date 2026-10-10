#include "EnginePCH.h"
#include "Texture2D.h"

#include "RenderCommand.h"

namespace
{
	// sRGB 포맷은 리소스를 TYPELESS로 만들어 디코딩하는 SRV와 그대로 읽는 SRV를 함께 만든다.
	struct FSRGBFormatFamily
	{
		DXGI_FORMAT Typeless;
		DXGI_FORMAT Raw;
	};

	bool GetSRGBFormatFamily(DXGI_FORMAT Format, FSRGBFormatFamily& OutFamily)
	{
		switch (Format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			OutFamily = { DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_UNORM };
			return true;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			OutFamily = { DXGI_FORMAT_B8G8R8A8_TYPELESS, DXGI_FORMAT_B8G8R8A8_UNORM };
			return true;
		default:
			return false;
		}
	}
}

FTexture2D::FTexture2D(ID3D11Device* Device, const D3D11_TEXTURE2D_DESC& InDesc, const FImageData& Image)
	: FTexture2D(Device, InDesc, Image.IsValid() ? Image.Pixels.GetData() : nullptr, Image.GetRowPitch())
{
}

FTexture2D::FTexture2D(ID3D11Device* Device, const D3D11_TEXTURE2D_DESC& InDesc, const void* InitialData, uint32 RowPitch)
{
	Width = InDesc.Width;
	Height = InDesc.Height;
	Depth = 1;
	Format = InDesc.Format;
	Dimension = ETextureDimension::Texture2D;

	if (InitialData && RowPitch == 0)
	{
		const uint32 BytesPerPixel = FormatToBytes(InDesc.Format);
		if (BytesPerPixel == 0) { /* 기존 에러 처리 그대로 */ return; }
		RowPitch = InDesc.Width * BytesPerPixel;
	}

	const bool bGenerateMips = (InDesc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS) != 0;

	D3D11_SUBRESOURCE_DATA SubData{ InitialData, RowPitch, 0 };
	const D3D11_SUBRESOURCE_DATA* InitPtr = (InitialData && !bGenerateMips) ? &SubData : nullptr;

	D3D11_TEXTURE2D_DESC ResourceDesc = InDesc;
	FSRGBFormatFamily SRGBFamily;
	if (GetSRGBFormatFamily(InDesc.Format, SRGBFamily))
		ResourceDesc.Format = SRGBFamily.Typeless;

	HRESULT hr = Device->CreateTexture2D(&ResourceDesc, InitPtr, (ID3D11Texture2D**)Texture.GetAddressOf());
	if (FAILED(hr)) { /* 기존 로그 그대로 */ return; }

	CreateViews(Device, InDesc);

	if (bGenerateMips && InitialData && SRV)
	{
		ComPtr<ID3D11DeviceContext> Context;
		Device->GetImmediateContext(Context.GetAddressOf());
		Context->UpdateSubresource(Texture.Get(), 0, nullptr, InitialData, RowPitch, 0); // 0번 레벨
		Context->GenerateMips(SRV.Get());                                              // 나머지 레벨
	}
}
FTexture2D::FTexture2D(ID3D11Device* Device, ComPtr<ID3D11Resource> SwapchainTexture, const D3D11_TEXTURE2D_DESC& InDesc)
{
	Texture = std::move(SwapchainTexture);

	Width = InDesc.Width;
	Height = InDesc.Height;
	Depth = 1;
	Format = InDesc.Format;
	Dimension = ETextureDimension::Texture2D;

	CreateViews(Device, InDesc);
}

void FTexture2D::CreateViews(ID3D11Device* Device, const D3D11_TEXTURE2D_DESC& InDesc)
{
	HRESULT hr;

	// TYPELESS 깊이 텍스처는 뷰마다 해석할 포맷을 지정해야 한다 (nullptr이면 생성 실패)
	const bool bTypelessDepth = (InDesc.Format == DXGI_FORMAT_R24G8_TYPELESS);
	// sRGB 텍스처도 리소스가 TYPELESS라 뷰 포맷을 지정해야 한다
	FSRGBFormatFamily SRGBFamily;
	const bool bSRGB = GetSRGBFormatFamily(InDesc.Format, SRGBFamily);

	if (InDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
	{
		if (bSRGB)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc{};
			SrvDesc.Format = InDesc.Format;                     // 샘플링할 때 sRGB -> Linear 디코딩
			SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			SrvDesc.Texture2D.MipLevels = static_cast<UINT>(-1);  // 밉 전체

			hr = Device->CreateShaderResourceView(Texture.Get(), &SrvDesc, SRV.GetAddressOf());
			if (FAILED(hr))
				HTR_LOG(Error, "[Texture2D] CreateShaderResourceView (sRGB) failed (hr=0x{:08X})", (uint32)hr);

			SrvDesc.Format = SRGBFamily.Raw;                    // 저장된 값 그대로
			hr = Device->CreateShaderResourceView(Texture.Get(), &SrvDesc, RawSRV.GetAddressOf());
			if (FAILED(hr))
				HTR_LOG(Error, "[Texture2D] CreateShaderResourceView (raw) failed (hr=0x{:08X})", (uint32)hr);
		}
		else
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc{};
			SrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;   // 깊이 24bit만 float로 읽는다
			SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			SrvDesc.Texture2D.MipLevels = 1;

			hr = Device->CreateShaderResourceView(Texture.Get(), bTypelessDepth ? &SrvDesc : nullptr, SRV.GetAddressOf());
			if (FAILED(hr))
				HTR_LOG(Error, "[Texture2D] CreateShaderResourceView failed (hr=0x{:08X})", (uint32)hr);
		}
	}

	if ((InDesc.BindFlags & D3D11_BIND_RENDER_TARGET) &&
		!(InDesc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS))
	{
		D3D11_RENDER_TARGET_VIEW_DESC RtvDesc{};
		RtvDesc.Format = InDesc.Format;                         // sRGB면 쓸 때 Linear -> sRGB 인코딩
		RtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		hr = Device->CreateRenderTargetView(Texture.Get(), bSRGB ? &RtvDesc : nullptr, RTV.GetAddressOf());
		if (FAILED(hr))
			HTR_LOG(Error, "[Texture2D] CreateRenderTargetView failed (hr=0x{:08X})", (uint32)hr);
	}

	if (InDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC DsvDesc{};
		DsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		DsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

		hr = Device->CreateDepthStencilView(Texture.Get(), bTypelessDepth ? &DsvDesc : nullptr, DSV.GetAddressOf());
		if (FAILED(hr))
			HTR_LOG(Error, "[Texture2D] CreateDepthStencilView failed (hr=0x{:08X})", (uint32)hr);
	}
}

UTexture2D::UTexture2D()
{

}
UTexture2D::~UTexture2D()
{
}

