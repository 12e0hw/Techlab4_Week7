#include "EnginePCH.h"
#include "RenderResourceManager.h"

#include "RenderCommand.h"

#include "RenderUtil.h"



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
		if (Entry.path().filename() == "UberLit.hlsl") continue;

		FString Path = Entry.path().generic_string();
		LoadOrCompileShader(Path);
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

	HTR_LOG(Error, "[Shader] not found: {}", InPath);

	if (TUniquePtr<FShaderProgram>* Fallback =
		Get().ShaderProgramMap.Find("Resources/Shader/DefaultShader.hlsl"))
		return Fallback->get();

	return nullptr;
}

void FRenderResourceManager::LoadOrCompileShader(const FString& Path, const FString& PermutationSuffix, const D3D_SHADER_MACRO* Defines)
{
	const FString Key = Path + PermutationSuffix;

	FString VSCSOPath;
	FString PSCSOPath;
	FShaderByteCode VSCode = RenderUtil::GetOrCompile(Path, "mainVS", EShaderType::Vertex, VSCSOPath, PermutationSuffix, Defines);
	FShaderByteCode PSCode = RenderUtil::GetOrCompile(Path, "mainPS", EShaderType::Pixel, PSCSOPath, PermutationSuffix, Defines);

	if (!VSCode.IsValid() || !PSCode.IsValid())
	{
		HTR_LOG(Error, "[Shader] compile failed: {}", Key);
		return;
	}

	TUniquePtr<FVertexShader> Vs = RenderCommand::CreateVertexShader(VSCode);
	TUniquePtr<FPixelShader>  Ps = RenderCommand::CreatePixelShader(PSCode);

	if (!Vs || !Vs->IsValid() || !Ps || !Ps->IsValid())
	{
		HTR_LOG(Error, "[Shader] device create failed: {}", Key);
		return;
	}

	FVertexShader* VsRaw = Vs.get();
	FPixelShader* PsRaw = Ps.get();

	VertexShaderMap[VSCSOPath] = std::move(Vs);
	PixelShaderMap[PSCSOPath] = std::move(Ps);
	ShaderProgramMap[Key] = MakeUnique<FShaderProgram>(VsRaw, PsRaw);

	HTR_LOG(Info, "[Shader] loaded: {}", Key);

}

void FRenderResourceManager::LoadUberLitPermutations()
{
	const FString Path = "Resources/Shader/UberLit.hlsl";

	const D3D_SHADER_MACRO UnlitDefines[] = {
		{ "UNLIT", "1" },
		{ nullptr, nullptr }          // 배열 끝
	};
	LoadOrCompileShader(Path, "_UNLIT", UnlitDefines);

	const char* Digits[] = { "0", "1", "2" };

	for (int32 LM = 0; LM < 3; ++LM)          // Lambert, Phong, Blinn-Phong
	{
		for (int32 SM = 0; SM < 2; ++SM)      // Gouraud, Phong
		{
			const D3D_SHADER_MACRO Defines[] = {
				{ "UNLIT",          "0" },
				{ "LIGHTING_MODEL", Digits[LM] },
				{ "SHADING_MODEL",  Digits[SM] },
				{ nullptr, nullptr }
			};
			const FString Suffix = FString("_LM") + Digits[LM] + "_SM" + Digits[SM];
			LoadOrCompileShader(Path, Suffix, Defines);
		}
	}
}
