#pragma once

#include "RenderEnums.h"

struct FShaderByteCode
{
	ComPtr<ID3DBlob> Blob;

	const void* GetData() const { return Blob ? Blob->GetBufferPointer() : nullptr; }
	SIZE_T      GetSize() const { return Blob ? Blob->GetBufferSize() : 0; }
	bool        IsValid() const { return Blob != nullptr; }
};

class RenderUtil
{
public:
	static FShaderByteCode CompileShader(FString Path, const char* EntryPoint, EShaderType ShaderType, const D3D_SHADER_MACRO* Defines = nullptr);
	static void SaveByteCode(FString TargetPath, const FShaderByteCode& ByteCode);
	static FShaderByteCode LoadByteCode(const FString& CsoPath);

	static FString GetCsoPath(const FString& ShaderPath, EShaderType ShaderType, const FString& PermutationSuffix = "");
	static bool IsCsoUpToDate(const FString& ShaderPath, const FString& CsoPath);
	// PermutationSuffix는 Defines 조합마다 달라야 한다. 같은 접미사면 .cso 캐시를 공유한다.
	static FShaderByteCode GetOrCompile(const FString& ShaderPath, const char* EntryPoint, EShaderType ShaderType, FString& OutCSOPath,
		const FString& PermutationSuffix = "", const D3D_SHADER_MACRO* Defines = nullptr);
};