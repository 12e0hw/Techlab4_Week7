#pragma once

#include <format>
#include "Editor/EditorUI/EditorPanel.h"
#include "Math/Quat.h"
#include "Math/Vector.h"
#include "Render/ViewMode.h"
class FMultipleViewportsAdapter;

class UWorld;

struct FEditorSettings
{
	// 에디터 렌더 옵션과 Multiple Viewports 레이아웃의 영구 설정을 한곳에 담는다.
	// Show: 화면에 무엇을 그릴지
	bool bDrawPrimitives = true;
	bool bDrawBoundingBox = false;
	bool bShowUUID = false;
	bool bDrawGrid = true;
	bool bExponentialHeightFog = true;
	bool bVisualizeTileLightCount = false;

	// Rendering
	bool bEnableFXAA = true;
	bool bDepthView = false;
	// 끄면 감마 공간 렌더링이 되어 Linear 작업 흐름과 비교할 수 있다 (ColorWorkflow.h)
	bool bLinearWorkflow = true;
	// HDR 씬 컬러를 표시 범위로 압축하는 단계라 Linear 작업 흐름의 Lit 뷰에서만 적용된다
	bool bToneMapping = true;
	// ACES fitted는 중간 회색(0.18)을 어둡게 내리므로, +0.6EV에서 톤매핑 전과 비슷한 밝기가 된다
	float ExposureEV = 0.6f;

	// Grid
	int32 GridSpacing = 1;

	// Camera
	float CameraSpeed = 1.0f;
	float MouseSensitivity = 1.0f;

	// Multiple Viewports
	float MultipleViewportsHorizontal = 0.5f;
	float MultipleViewportsVertical = 0.5f;
	bool bMultipleViewportsSingle = false;
	int32 MultipleViewportsSingleViewIndex = 0;
    // 미저장 표식은 초기 카메라 값을 유지한다. 위치·회전은 저장하지 않는다.
    float ViewFov[4]{};
    float ViewOrthoWidth[4]{};
    int32 ViewPreset[4]{-1,-1,-1,-1};
    // 키가 없으면 FViewRenderMode 기본값(Lit)을 쓴다.
    FViewRenderMode ViewRenderMode[4]{};
    // View별 카메라 Transform은 Quaternion 그대로 저장해 재실행 후 같은 시점을 복원한다.
    // 구형 editor.ini에 키가 없으면 Saved가 false로 남아 Adapter의 초기 Transform을 유지한다.
    FVector ViewLocation[4]{};
    FQuat ViewRotation[4]{};
    bool bViewLocationSaved[4]{};
    bool bViewRotationSaved[4]{};
};


class FSettingsPanel : public IEditorPanel
{
public:
	FSettingsPanel() = default;
	~FSettingsPanel();

	bool Init() override;
	void Tick(float DeltaTime)override;
	void OnRender() override;
	const char* GetPanelName() const override { return "Settings"; }

	void SetWorld(UWorld* InWorld) { World = InWorld; }

	const FEditorSettings& GetSettings() const { return Settings; }
	// 실행 중 레이아웃 변경을 editor.ini 저장 대상과 동일한 설정 객체에 반영한다.
	FEditorSettings& GetMutableSettings() { return Settings; }
	// 알려진 모든 설정 섹션을 한 번에 다시 써서 editor.ini 값이 서로 덮어쓰이지 않게 저장한다.
	bool SaveSettings() const;
	// editor.ini의 기존 렌더·에디터·Multiple Viewports 값을 같은 설정 객체로 복원한다.
	bool LoadSettings();
    // 초기화된 Adapter에 저장 설정을 적용한다.
    void SetViewportAdapter(FMultipleViewportsAdapter* Value);
    // 종료 저장은 마지막 프레임의 복사본을 사용해 Adapter 수명과 분리한다.
    void CaptureViewportSettings();

private:
	static constexpr float SectionGap = 10.0f;
	static constexpr float SubsectionGap = 4.0f;

    // View별 Transform·투영·표시 설정을 읽어 저장용 스냅샷에 반영한다.
    void ReadViewportSettings(FEditorSettings& Out) const;
    void ApplyViewportSettings();
    FMultipleViewportsAdapter* ViewportAdapter = nullptr;
	UWorld* World;
	FEditorSettings Settings;
};
