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

	PipelineState.Shader = Shader;
	PipelineState.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	// 출력 타깃 전체를 덮어쓰는 후처리라 블렌딩·깊이 테스트가 필요 없다
	PipelineState.RasterizerState = ERasterizerState::SolidNone;
	PipelineState.BlendState = EBlendState::Opaque;
	PipelineState.DepthStencilState = EDepthStencilState::Disabled;
	return true;
}

void FGammaEncodeRenderer::OnRender(FTexture2D* SceneColor)
{
	if (!Shader || !SceneColor)
		return;

	RenderCommand::BindPipelineState(PipelineState);
	RenderCommand::BindShaderResource(0, SceneColor, EShaderBindFlagBits::Pixel);
	RenderCommand::Draw(3);
}
