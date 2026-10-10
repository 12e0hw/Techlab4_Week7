#pragma once

#include "Shader.h"

class FRenderResourceManager
{
public:
	static FRenderResourceManager& Get()
	{
		static FRenderResourceManager* Instance = new FRenderResourceManager();
		return *Instance;
	}

	static void Init()
	{
		Get().ScanShaders("Resources/Shader");
		Get().LoadUberLitPermutations();
	}

	void ScanShaders(const fs::path& ShaderRoot);

	static void Shutdown();

	static FShaderProgram* GetShaderProgram(const FString& InPath);
private:
	// Path + PermutationSuffix가 GetShaderProgram 키가 된다.
	void LoadOrCompileShader(const FString& Path, const FString& PermutationSuffix = "", const D3D_SHADER_MACRO* Defines = nullptr);
	void LoadUberLitPermutations();

	TMap<FString, TUniquePtr<FVertexShader>> VertexShaderMap;
	TMap<FString, TUniquePtr<FPixelShader>>  PixelShaderMap;
	TMap<FString, TUniquePtr<FShaderProgram>> ShaderProgramMap;
}; 