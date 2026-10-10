#pragma once

// 에디터 UI가 쓰는 파일 경로. 모두 Resources 아래에 있어야 한다.
// 엔진 공용 경로는 Runtime/Core/ResourcePaths.h에 있다.
namespace EditorResourcePaths
{
	// ImGui 한글 UI 폰트
	constexpr const char* UIFont = "Resources/Fonts/HMKMRHD.TTF";

	constexpr const char* FolderIcon = "Resources/Icons/Folder.png";
	constexpr const char* FileIcon = "Resources/Icons/File.png";

	constexpr const char* ToolbarSaveIcon = "Resources/Icons/Toolbar/Save.png";
	constexpr const char* ToolbarPlayIcon = "Resources/Icons/Toolbar/Play.png";
	constexpr const char* ToolbarPauseIcon = "Resources/Icons/Toolbar/Pause.png";
	constexpr const char* ToolbarStepIcon = "Resources/Icons/Toolbar/Step.png";
	constexpr const char* ToolbarStopIcon = "Resources/Icons/Toolbar/Stop.png";
}
