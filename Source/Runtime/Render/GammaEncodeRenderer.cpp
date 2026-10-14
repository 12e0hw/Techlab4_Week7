#include "EnginePCH.h"

#include "GammaEncodeRenderer.h"
#include "RenderCommand.h"
#include "RenderResourceManager.h"
#include "Texture2D.h"

bool FGammaEncodeRenderer::Init()
{
	Shader = FRenderResourceManager::GetShaderProgram("Resources/Shader/GammaEncodeShader.hlsl");
	if (!Shader)
	{
		HTR_LOG(Error, "[GammaEncode] shader not found");
		return false;
	}

	ConstantBuffer = RenderCommand::CreateConstantBuffer(sizeof(FGammaEncodeConstants));

	PipelineState.Shader = Shader;
	PipelineState.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	// 출력 타깃 전체를 덮어쓰는 후처리라 블렌딩·깊이 테스트가 필요 없다
	PipelineState.RasterizerState = ERasterizerState::SolidNone;
	PipelineState.BlendState = EBlendState::Opaque;
	PipelineState.DepthStencilState = EDepthStencilState::Disabled;
	return true;
}

void FGammaEncodeRenderer::OnRender(FTexture2D* SceneColor, bool bEncode, bool bToneMap, float ExposureEV)
{
	if (!Shader || !SceneColor)
		return;

	RenderCommand::BindPipelineState(PipelineState);
	RenderCommand::BindShaderResource(0, SceneColor, EShaderBindFlagBits::Pixel);

	FGammaEncodeConstants Constants{};
	Constants.bEncode = bEncode ? 1u : 0u;
	Constants.bToneMap = bToneMap ? 1u : 0u;
	Constants.Exposure = std::exp2(ExposureEV);
	RenderCommand::UpdateBufferData(ConstantBuffer.get(), &Constants);
	RenderCommand::BindConstantBuffer(0, ConstantBuffer.get(), EShaderBindFlagBits::Pixel);

	RenderCommand::Draw(3);
}
