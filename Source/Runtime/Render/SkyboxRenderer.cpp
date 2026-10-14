#include "EnginePCH.h"
#include "SkyboxRenderer.h"

#include "RenderCommand.h"
#include "RenderResourceManager.h"
#include "ImageLoader.h"
#include "LightInfo.h"

bool FSkyboxRenderer::Init(const FString& PanoramaPath)
{
	Shader = FRenderResourceManager::GetShaderProgram("Resources/Shader/SkyboxShader.hlsl");
	if (!Shader)
	{
		HTR_LOG(Error, "[Skybox] shader not found");
		return false;
	}

	FImageData Image = ImageLoader::LoadAuto(PanoramaPath);
	if (!Image.IsValid())
	{
		HTR_LOG(Error, "[Skybox] panorama load failed: {}", PanoramaPath);
		return false;
	}

	D3D11_TEXTURE2D_DESC Desc{};
	Desc.Width = Image.Width;
	Desc.Height = Image.Height;
	Desc.MipLevels = 1;
	Desc.ArraySize = 1;
	Desc.Format = Image.Format;          // .hdr이면 R32G32B32A32_FLOAT
	// .jpg·.png 파노라마는 sRGB로 저장돼 있으므로 샘플링할 때 Linear로 디코딩한다
	if (Desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM)
		Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	Desc.SampleDesc.Count = 1;
	Desc.Usage = D3D11_USAGE_IMMUTABLE;
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	PanoramaTexture = RenderCommand::CreateTexture2D(Desc, Image);
	if (!PanoramaTexture) return false;

	ConstantBuffer = RenderCommand::CreateConstantBuffer(sizeof(FSkyboxConstants));

	PipelineState.Shader = Shader;
	PipelineState.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	// 전체 화면 삼각형이라 컬링 방향을 따질 필요가 없다.
	PipelineState.RasterizerState = ERasterizerState::SolidNone;
	PipelineState.BlendState = EBlendState::Opaque;
	// 깊이 테스트/쓰기 모두 끈다. 배경을 먼저 깔고 물체가 그 위에 그려지게 한다.
	PipelineState.DepthStencilState = EDepthStencilState::Disabled;

	HTR_LOG(Info, "[Skybox] loaded: {} ({}x{})", PanoramaPath, Image.Width, Image.Height);
	return true;
}

void FSkyboxRenderer::OnRender(const FMatrix& ViewProjection, const FVector& CameraPosition, const FDirectionalLightInfo& DirectionalLight)
{
	if (!IsValid()) return;

	FSkyboxConstants Constants;
	Constants.InverseViewProjection = ViewProjection.Inverse();
	Constants.CameraPosition = CameraPosition;

	if (DirectionalLight.Direction.SizeSquared() > 0.0001f)
	{
		Constants.SunDirection = -DirectionalLight.Direction.GetSafeNormal();
		Constants.SunColor = DirectionalLight.Color;
		Constants.SunIntensity = 15.0f;
		Constants.SunRadius = 0.025f;
	}
	else
	{
		Constants.SunDirection = FVector(0.0f, 0.0f, 1.0f);
		Constants.SunColor = FVector(1.0f, 1.0f, 1.0f);
		Constants.SunIntensity = 0.0f;
	}

	RenderCommand::BindPipelineState(PipelineState);
	RenderCommand::UpdateBufferData(ConstantBuffer.get(), &Constants);
	RenderCommand::BindConstantBuffer(0, ConstantBuffer.get(), EShaderBindFlagBits::Pixel);
	RenderCommand::BindShaderResource(0, PanoramaTexture.get(), EShaderBindFlagBits::Pixel);
	// 경도(U)가 0<->1에서 감기므로 Clamp가 아니라 Wrap이어야 이음매가 안 보인다.
	RenderCommand::BindSamplerState(0, ESamplerState::LinearWrap, EShaderBindFlagBits::Pixel);

	// 정점 버퍼 없이 3개. VS가 SV_VertexID로 삼각형을 만든다.
	RenderCommand::Draw(3);
}                               
