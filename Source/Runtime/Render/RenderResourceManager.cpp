#include "EnginePCH.h"
#include "RenderResourceManager.h"

#include "RenderCommand.h"

#include "RenderUtil.h"



static const D3D_SHADER_MACRO GouraudMacros[] = {
	{ "SHADING_MODEL_GOURAUD", "1" },
	{ nullptr, nullptr }
};

static const D3D_SHADER_MACRO LambertMacros[] = {
	{ "SHADING_MODEL_LAMBERT", "1" },
	{ nullptr, nullptr }
};

static const D3D_SHADER_MACRO BlinnPhongMacros[] = {
	{ "SHADING_MODEL_BLINN_PHONG", "1" },
	{ nullptr, nullptr }
};

struct FShadingModelPermutationDef
{
	EShadingModel Model;
	const char* Suffix;
	const D3D_SHADER_MACRO* Macros;
};

static const FShadingModelPermutationDef ShadingModelPermutations[] = {
	{ EShadingModel::Gouraud,    "_Gouraud",    GouraudMacros },
	{ EShadingModel::Lambert,    "_Lambert",    LambertMacros },
	{ EShadingModel::BlinnPhong, "_BlinnPhong", BlinnPhongMacros },
};

FString FRenderResourceManager::GetPermutationKey(const FString& InPath, EShadingModel ShadingModel)
{
	switch (ShadingModel)
	{
	case EShadingModel::Gouraud:    return InPath + "_Gouraud";
	case EShadingModel::Lambert:    return InPath + "_Lambert";
	case EShadingModel::BlinnPhong: return InPath + "_BlinnPhong";
	default:                        return InPath;
	}
}

void FRenderResourceManager::ScanShaders(const fs::path& ShaderRoot)
{
	std::error_code ErrorCode;

	if (!fs::exists(ShaderRoot, ErrorCode))
	{
		HTR_LOG(Error, "[Shader] Scan Root Not Found : {}", ShaderRoot.generic_string());
		return;
	}

	for (const fs::directory_entry& Entry : fs::recursive_directory_iterator(ShaderRoot))
	{
		if (!Entry.is_regular_file()) continue;
		if (Entry.path().extension() != ".hlsl") continue;

		FString Path = Entry.path().generic_string();
		if (Entry.path().filename() == "StaticMeshShader.hlsl")
		{
			LoadOrCompilePermutations(Path);
		}
		else
		{
			LoadOrCompileShader(Path);
		}
	}
}

void FRenderResourceManager::Shutdown()
{
	Get().VertexShaderMap.Empty();
	Get().PixelShaderMap.Empty();
	Get().ShaderProgramMap.Empty();
}

FShaderProgram* FRenderResourceManager::GetShaderProgram(const FString& InPath)
{
	if (TUniquePtr<FShaderProgram>* Found = Get().ShaderProgramMap.Find(InPath))
		return Found->get();

	FString DefaultKey = GetPermutationKey(InPath, EShadingModel::Lambert);
	if (TUniquePtr<FShaderProgram>* FoundPermutation = Get().ShaderProgramMap.Find(DefaultKey))
		return FoundPermutation->get();

	HTR_LOG(Error, "[Shader] not found: {}", InPath);

	if (TUniquePtr<FShaderProgram>* Fallback =
		Get().ShaderProgramMap.Find("Resources/Shader/DefaultShader.hlsl"))
		return Fallback->get();

	return nullptr;
}

FShaderProgram* FRenderResourceManager::GetShaderProgram(const FString& InPath, EShadingModel ShadingModel)
{
	FString Key = GetPermutationKey(InPath, ShadingModel);
	if (TUniquePtr<FShaderProgram>* Found = Get().ShaderProgramMap.Find(Key))
		return Found->get();

	return GetShaderProgram(InPath);
}

void FRenderResourceManager::LoadOrCompileShader(const FString& Path)
{ 
	FString VSCSOPath;
	FString PSCSOPath;
	FShaderByteCode VSCode = RenderUtil::GetOrCompile(Path, "mainVS", EShaderType::Vertex, VSCSOPath);
	FShaderByteCode PSCode = RenderUtil::GetOrCompile(Path, "mainPS", EShaderType::Pixel, PSCSOPath);

	if (!VSCode.IsValid() || !PSCode.IsValid())
	{
		HTR_LOG(Error, "[Shader] compile failed: {}", Path);
		return;
	}

	TUniquePtr<FVertexShader> Vs = RenderCommand::CreateVertexShader(VSCode);
	TUniquePtr<FPixelShader>  Ps = RenderCommand::CreatePixelShader(PSCode);

	if (!Vs || !Vs->IsValid() || !Ps || !Ps->IsValid())
	{
		HTR_LOG(Error, "[Shader] device create failed: {}", Path);
		return;
	}

	FVertexShader* VsRaw = Vs.get();
	FPixelShader* PsRaw = Ps.get();

	VertexShaderMap[VSCSOPath] = std::move(Vs);
	PixelShaderMap[PSCSOPath] = std::move(Ps);
	ShaderProgramMap[Path] = MakeUnique<FShaderProgram>(VsRaw, PsRaw);

	HTR_LOG(Info, "[Shader] loaded: {}", Path);
}

void FRenderResourceManager::LoadOrCompilePermutations(const FString& Path)
{
	for (const auto& Perm : ShadingModelPermutations)
	{
		FString VSCSOPath;
		FString PSCSOPath;
		FShaderByteCode VSCode = RenderUtil::GetOrCompile(Path, "mainVS", EShaderType::Vertex, VSCSOPath, Perm.Macros, Perm.Suffix);
		FShaderByteCode PSCode = RenderUtil::GetOrCompile(Path, "mainPS", EShaderType::Pixel, PSCSOPath, Perm.Macros, Perm.Suffix);

		if (!VSCode.IsValid() || !PSCode.IsValid())
		{
			HTR_LOG(Error, "[Shader] compile permutation failed: {} ({})", Path, Perm.Suffix);
			continue;
		}

		TUniquePtr<FVertexShader> Vs = RenderCommand::CreateVertexShader(VSCode);
		TUniquePtr<FPixelShader>  Ps = RenderCommand::CreatePixelShader(PSCode);

		if (!Vs || !Vs->IsValid() || !Ps || !Ps->IsValid())
		{
			HTR_LOG(Error, "[Shader] device create permutation failed: {} ({})", Path, Perm.Suffix);
			continue;
		}

		FVertexShader* VsRaw = Vs.get();
		FPixelShader* PsRaw = Ps.get();

		VertexShaderMap[VSCSOPath] = std::move(Vs);
		PixelShaderMap[PSCSOPath] = std::move(Ps);

		FString Key = GetPermutationKey(Path, Perm.Model);
		ShaderProgramMap[Key] = MakeUnique<FShaderProgram>(VsRaw, PsRaw);

		// 기본 Path(Lambert)도 매핑하여 기존 호출부 하위 호환성 유지
		if (Perm.Model == EShadingModel::Lambert)
		{
			ShaderProgramMap[Path] = MakeUnique<FShaderProgram>(VsRaw, PsRaw);
		}

		HTR_LOG(Info, "[Shader] loaded permutation: {} ({})", Path, Perm.Suffix);
	}
}
