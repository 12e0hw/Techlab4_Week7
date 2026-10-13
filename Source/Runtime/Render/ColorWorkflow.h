#pragma once

#include "Math/Color.h"

// Linear 작업 흐름의 효과를 비교하기 위한 전역 토글.
// false면 sRGB 디코딩·인코딩을 모두 건너뛰어, sRGB 값 그대로 라이팅·블렌딩하는 감마 공간 렌더링이 된다.
// 두 모드 모두 무조명 결과는 같은 화면이 되므로, 차이는 라이팅·블렌딩·필터링을 어느 공간에서 했는지에서만 나온다.
namespace ColorWorkflow
{
	inline bool bLinear = true;

	// 에디터 색은 Linear로 저장되고 선택기에는 sRGB로 보인다. 감마 모드에서는 선택기에 보이는 sRGB 값을 셰이더에 넘긴다.
	inline FLinearColor ToShaderColor(const FLinearColor& C)
	{
		if (bLinear)
			return C;
		return FLinearColor(FLinearColor::LinearToSRGB(C.R), FLinearColor::LinearToSRGB(C.G), FLinearColor::LinearToSRGB(C.B), C.A);
	}
}
