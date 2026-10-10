#pragma once
#include "Render/PipelineState.h"
class FTexture2D;

// Linear 씬 컬러를 화면 표시용 sRGB로 인코딩하는 후처리. 언리얼에서는 톤매퍼 패스가 이 일을 같이 한다.
class FGammaEncodeRenderer
{
public:
	FGammaEncodeRenderer() = default;
	~FGammaEncodeRenderer() = default;

	bool Init();
	// 현재 바인딩된 렌더 타깃에 SceneColor를 인코딩해 그린다. 렌더 패스는 호출하는 쪽이 연다.
	void OnRender(FTexture2D* SceneColor);

	FGammaEncodeRenderer(const FGammaEncodeRenderer&) = delete;
	FGammaEncodeRenderer& operator=(const FGammaEncodeRenderer&) = delete;

private:
	FShaderProgram* Shader = nullptr;
	FPipelineState PipelineState;
};
