#pragma once
#include "Render/PipelineState.h"
#include "Render/Buffer.h"
class FTexture2D;

struct FGammaEncodeConstants
{
	uint32 bEncode;
	uint32 bToneMap;
	float Exposure;
	uint32 Padding;
};

// Linear 씬 컬러를 ACES로 톤매핑하고 화면 표시용 sRGB로 인코딩하는 후처리. 언리얼의 톤매퍼 패스에 해당한다.
class FGammaEncodeRenderer
{
public:
	FGammaEncodeRenderer() = default;
	~FGammaEncodeRenderer() = default;

	bool Init();
	// 현재 바인딩된 렌더 타깃에 SceneColor를 그린다. bToneMap·bEncode가 모두 false면 값을 그대로 옮긴다.
	// ExposureEV는 톤매핑할 때만 쓴다. 렌더 패스는 호출하는 쪽이 연다.
	void OnRender(FTexture2D* SceneColor, bool bEncode, bool bToneMap, float ExposureEV);

	FGammaEncodeRenderer(const FGammaEncodeRenderer&) = delete;
	FGammaEncodeRenderer& operator=(const FGammaEncodeRenderer&) = delete;

private:
	FShaderProgram* Shader = nullptr;
	FPipelineState PipelineState;
	TUniquePtr<FConstantBuffer> ConstantBuffer;
};
