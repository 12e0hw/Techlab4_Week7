#pragma once

// 엔진이 동작하는 데 반드시 필요한 파일 경로. 모두 Resources 아래에 있어야 한다.
// Assets 폴더가 없어도 엔진이 떠야 하므로, 여기 있는 파일은 Assets를 가리키면 안 된다.
// 에디터 전용 경로는 Editor/HitoriEd/EditorResourcePaths.h에 둔다.
namespace ResourcePaths
{
	constexpr const char* SplashImage = "Resources/Splash/Splash.png";

	// 에디터 UI와 새로 만든 텍스트 컴포넌트가 함께 쓰는 기본 폰트
	constexpr const char* DefaultFont = "Resources/Fonts/Pretendard.json";
	constexpr const char* DefaultFontAtlas = "Resources/Fonts/Pretendard.png";

	constexpr const char* DefaultSkyPanorama = "Resources/SkySphere/Sky.jpg";
	constexpr const char* DefaultParticleTexture = "Resources/SubUV/StarParticle.png";
}
