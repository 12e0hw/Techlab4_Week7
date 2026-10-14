#pragma once

#include "MaterialInterface.h"
#include "Render/RenderStates.h"
#include "Render/Texture2D.h"
#include "Math/Color.h"

class FShader;



enum class EMaterialParamLayout
{
	None,
	StaticMesh,
	ParticleSubUV,
	Billboard
};

struct FStaticMeshMaterialParams
{
	FLinearColor BaseColor;
	FVector2 UVScrollOffset;
	uint32 bOpaque; // 1이면 PS가 알파를 1로 출력한다
	float Shininess;
	FVector SpecularColor;
	uint32 bFlipNormalGreen;
};
static_assert(sizeof(FStaticMeshMaterialParams) == 48);

struct FBillboardMaterialParams
{
	FLinearColor TintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
};
static_assert(sizeof(FBillboardMaterialParams) == 16);

class UMaterial : public UMaterialInterface
{
	DECLARE_CLASS(UMaterial, UMaterialInterface)

public:
	UMaterial() :SortID(NextSortID++) {};
	virtual ~UMaterial() override = default;

	EMaterialParamLayout ParamLayout = EMaterialParamLayout::None;
	FShaderProgram* Shader;
	TArray<UTexture2D*> Textures;
	TUniquePtr<FConstantBuffer> ParamBuffer;
	EBlendState BlendState = EBlendState::Opaque;
	EDepthStencilState DepthStencilState = EDepthStencilState::Default;
	ESamplerState SamplerState = ESamplerState::LinearClamp;

	// 셰이더가 텍스처와 그대로 곱하는 Linear 값 (언리얼 머티리얼 벡터 파라미터와 같다)
	FLinearColor BaseColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	FVector2 UVScrollSpeed = FVector2(0.0f, 0.0f);
	float Shininess = 32.0f;
	FLinearColor SpecularColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
	// OpenGL(Y+) 규칙으로 만든 노멀맵을 DirectX(Y-) 규칙으로 읽도록 G를 뒤집는다 (언리얼 Flip Green Channel)
	bool bFlipNormalGreen = false;

	bool bIsInstance = false;

	// 인스턴스가 복제된 원본. 인스턴스는 경로가 없어서, 저장할 때 원본을 따라가 기준 에셋을 찾는다
	const UMaterial* Parent = nullptr;

	// Source의 내용을 복사한 편집용 복제본을 만든다
	static UMaterial* CreateInstance(const UMaterial* Source);

	// 경로가 있는(=에셋으로 등록된) 가장 가까운 원본. 자신이 에셋이면 자신
	const UMaterial* GetBaseAsset() const;

	static json SaveMaterial(const UMaterial* Material);
	static UMaterial* LoadMaterial(const json& In);

	const uint16 SortID;

	using Super::Serialize;
	// 인스턴스의 값(색, 텍스처, 상태). 객체는 이미 만들어져 있어야 한다.
	virtual void Serialize(FStructuredArchive::FRecord Record) override;

	// 머티리얼 참조 하나를 Slot에 읽고 쓴다.
	// 에셋이면 경로만, 인스턴스면 기준 에셋 + 값을 그 자리에 쓰고, 불러올 때 인스턴스를 새로 만든다.
	// null이면 빈 Record({})로 기록되고, 불러오면 nullptr이 된다.
	static void SerializeMaterialReference(FStructuredArchive::FSlot Slot, UMaterial*& Material);
private:
	inline static uint16 NextSortID = 0;
};