#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "../Base/UIElementBase.h"
#include "../../../Module/D3D11Engine/util/SmoothValue.h"
#include "UIIconShape.h"
#include "UIIconRenderer.h"

class IRenderContext;
class UISVGResource;
struct ID2D1DeviceContext5;

// 위젯 하나가 들고 있는 아이콘.
//
// 예전에는 UIIconHelper 라는 정적 함수 모음이었고, 상태(경로, 리소스, 로드
// 여부, 배율, 애니메이션 값 두 개)는 위젯마다 멤버로 복제돼 있었다.
// UIButton 과 UIIconLabel 이 같은 멤버 5개와 같은 메서드를 각자 갖고 있었고,
// 헬퍼 함수는 그걸 참조로 받느라 인자가 8개까지 늘어났다.
//
// 상태와 동작을 한곳에 묶으면 위젯은 멤버 하나만 들면 된다.
class D3D11_UI_FRAMEWORK_INTERFACE_API UIIcon
{
public:
	UIIcon() = default;
	~UIIcon();

	// 리소스를 소유하므로 복사를 막는다.
	UIIcon(const UIIcon&) = delete;
	UIIcon& operator=(const UIIcon&) = delete;

	// 내장 도형. 기본 경로다.
	//
	// 파일이 아니라 코드로 그리므로 배포에서 빠질 수 없다.
	void SetShape(UIIconShape shape);
	UIIconShape GetShape() const;

	// SVG 파일. 호스트가 자기 아이콘을 쓰고 싶을 때의 옵션이다.
	//
	// 경로가 설정돼 있으면 내장 도형보다 우선한다.
	// 내장 도형으로 되돌리려면 SetPath(nullptr) 를 부른다.
	void SetPath(const wchar_t* path);
	bool HasPath() const;

	// 그릴 게 있는가(도형이든 SVG 이든).
	bool HasIcon() const;

	void SetStyle(const UIStyle& style);
	UIStyle& Style();
	const UIStyle& Style() const;

	// 아이콘이 영역을 채우는 비율. 1.0 이면 영역에 꽉 찬다.
	void SetScale(float scale);
	float GetScale() const;

	// 아직 로드하지 않았으면 지금 로드한다. 이미 로드했으면 아무것도 하지 않는다.
	bool EnsureLoaded(IRenderContext* context);
	bool IsLoaded() const;

	// 디바이스 리소스만 버린다. 경로는 남으므로 EnsureLoaded 로 되살릴 수 있다.
	void ReleaseDeviceResources();

	// 경로까지 포함해 전부 버린다.
	void Reset();

	// 애니메이션 없이 현재 상태 색/배율로 즉시 맞춘다(초기화 직후용).
	void SnapToState(UIElementState state, float scale);

	// 상태 전이 시 목표값을 건다.
	void ApplyState(UIElementState state,
		float normalScale, float hoveredScale, float pressedScale);

	// 애니메이션 진행. 아직 목표에 도달하지 않았으면 true.
	bool UpdateAnimation(float dt, float colorSpeed, float scaleSpeed);

	// drawRect 안에 종횡비를 지키며 가운데 정렬해 그린다.
	//
	// scaleOverride 가 양수면 SetScale 대신 그 값을 쓴다.
	// (컨텍스트 메뉴의 체크 표시가 본래 아이콘보다 작게 그려야 해서 필요하다)
	bool Draw(ID2D1DeviceContext5* d2dContext, const D2D1_RECT_F& drawRect,
		float scaleOverride = -1.0f) const;

private:
	UIStyle m_style = {};

	UIIconShape m_shape = UIIconShape::None;
	UIIconRenderer m_shapeRenderer = {};
	bool m_shapeReady = false;

	wchar_t* m_path = nullptr;
	UISVGResource* m_resource = nullptr;
	bool m_loaded = false;

	float m_scale = 0.6f;

	SmoothColor m_smoothColor = {};
	SmoothValue m_smoothScale = {};
};
