#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

// 이 헤더는 D2D1_COLOR_F 와 DWRITE_* 열거형을 값으로 쓴다. 예전에는 이걸
// pch.h 가 먼저 들어와 있다는 전제에 기대고 있어서, pch 를 쓰지 않는 번역
// 단위(예: Test/UIFrameworkTest.cpp)에서는 이 헤더 하나만 include 하면
// 컴파일이 깨졌다. 자기가 쓰는 것은 자기가 include 한다.
#include <d2d1_3.h>
#include <dwrite_2.h>

#include "../../../Module/Core/ShapeType/Rect2f.h"
#include "../../../Module/D3D11EngineInterface/IUIRenderLayer.h"
#include "../Resource/ColorRGBA8.h"

using namespace Core::ShapeType;

struct UIColorSet
{
	D2D1_COLOR_F fill = { 0.0f, 0.0f, 0.0f, 0.0f };
	D2D1_COLOR_F border = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct UIStyle
{
	UIColorSet normal = {};
	UIColorSet hover = {};
	UIColorSet pressed = {};
	UIColorSet disabled = {};

	float borderThickness = 1.0f;
};

struct UITextStyle
{
	UIColorSet normal = {};
	UIColorSet hover = {};
	UIColorSet pressed = {};
	UIColorSet disabled = {};

	wchar_t fontName[50] = {};
	float fontSize = 14.0f;

	DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;
	DWRITE_TEXT_ALIGNMENT hAlign = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER;
	DWRITE_PARAGRAPH_ALIGNMENT vAlign = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
};

class IRenderContext;

// C4251: dllexport 클래스가 export 되지 않은 타입(Rect2f)을 멤버로 갖는다는 경고.
// Rect2f 는 Core 정적 라이브러리의 POD 레이아웃 타입이라 DLL 과 클라이언트가
// 같은 정의를 그대로 컴파일한다. 정적 데이터도, 가상 함수도 없어서
// 레이아웃 불일치가 생길 수 없으므로 이 경고는 실질적 위험이 없다.
#pragma warning(push)
#pragma warning(disable : 4251)

class D3D11_UI_FRAMEWORK_INTERFACE_API UIElementBase : public IUIRenderLayer
{
public:
	UIElementBase();
	virtual ~UIElementBase();

	UIElementBase(const UIElementBase&) = delete;
	UIElementBase& operator=(const UIElementBase&) = delete;
	UIElementBase(UIElementBase&&) = delete;
	UIElementBase& operator=(UIElementBase&&) = delete;

	// IRenderLayer Override
	//
	// Initialize 와 RestoreDeviceResources 는 final 이다. 둘 다 하는 일이
	// 같아서(디바이스 리소스 확보) 파생 클래스마다 거의 같은 함수 두 개가
	// 복제돼 있었고, 한쪽만 고쳐서 디바이스 로스트 후에만 깨지는 버그를
	// 만들기 쉬웠다. 재정의 지점은 AcquireDeviceResources 하나뿐이다.
	bool Initialize(IRenderContext* context) final;
	virtual bool RestoreDeviceResources(IRenderContext* context) final;

	void Shutdown() override;
	bool Render() override = 0;
	bool Prepare() override;

	// IUIRenderLayer Override
	bool HitTest(float x, float y) const override;

	// 잎 요소의 상태 머신은 여기 하나뿐이다. 클릭에 반응해야 하는 요소는
	// 이걸 재정의하지 말고 OnActivated() 를 재정의한다.
	bool OnMouseEvent(UIMouseEventType type, float x, float y) override;

	// 애니메이션을 dt 만큼 진행한다.
	//
	// 반환값은 성공/실패가 아니라 "아직 움직이는 중인가" 다.
	//   true  = 목표값에 도달하지 못했다. 호스트는 다음 프레임을 더 그려야 한다.
	//   false = 정지 상태다. 이 요소 때문에 다시 그릴 이유는 없다.
	//
	// 숨겨진 요소도 false 를 돌려준다. "안 보인다" 와 "다 멈췄다" 는 다른
	// 사실이지만 호출자가 이 값으로 하는 판단(프레임을 더 그릴까?)은 같으므로
	// 구분할 필요가 없다.
	virtual bool Update(float dt) = 0;
	virtual UIElementState GetState() const override;
	virtual void SetState(UIElementState state) override;

	virtual bool IsVisible() const override;
	virtual void SetVisible(bool visible) override;

	virtual void OnLayoutChanged();
	virtual void DiscardDeviceResources();

public:
	void SetLayout(const Core::ShapeType::Rect2f& rect);
	const Core::ShapeType::Rect2f& GetLayout() const;

	void SetStyle(const UIStyle& style);
	UIStyle& GetStyle();
	const UIStyle& GetStyle() const;

protected:
	UIElementState m_state = UIElementState::Normal;
	bool m_visible = true;
	bool m_mouseOver = false;

	UIStyle m_style = {};

	IRenderContext* m_context = nullptr;

protected:
	// 디바이스 리소스를 확보한다. 파생 클래스는 이 함수 하나만 재정의한다.
	//
	//   reset == true  : 최초 Initialize. 논리 상태(가시성, hover, 자식 배치)도
	//                    처음부터 세운다.
	//   reset == false : 디바이스 로스트 복구. 논리 상태는 그대로 두고
	//                    브러시/비트맵 같은 GPU 리소스만 다시 만든다.
	//
	// 실패하면 false 를 돌려준다. 부분적으로 만들어진 리소스를 되돌리는 것은
	// 호출자(Shutdown / DiscardDeviceResources)의 몫이다.
	virtual bool AcquireDeviceResources(IRenderContext* context, bool reset);

	bool BindRenderContext(IRenderContext* context, bool resetState);
	// 파생 클래스가 자주 쓰는 접근자. 값은 바로 아래 m_layout 이다.
	Core::ShapeType::Rect2f& LayoutData() { return m_layout; }
	const Core::ShapeType::Rect2f& LayoutData() const { return m_layout; }
	virtual void OnStateChanged(UIElementState oldState, UIElementState newState);

	// 스타일이 통째로 교체됐다. 색을 SmoothColor 로 캐시해 두는 파생 클래스는
	// 여기서 목표값을 다시 걸어야 한다. 이게 없으면 SetStyle 은 다음 상태
	// 전이가 일어날 때까지 화면에 반영되지 않는다.
	//
	// GetStyle() 로 참조를 받아 직접 고치는 경로는 이 훅을 타지 않는다.
	// 그쪽은 Initialize 이전 구성 단계에서만 쓰는 용도다.
	virtual void OnStyleChanged();

	// 누른 자리에서 버튼을 뗐다. 클릭이 성립한 지점이다.
	// UIButton 이 여기서 커맨드를 디스패치한다.
	virtual void OnActivated();

protected:
	// 예전에는 이 한 필드만 pimpl 뒤에 숨어 있었다. 나머지 상태(m_state,
	// m_visible, m_style, m_context)는 전부 protected 로 드러나 있어서
	// 일관성이 없었고, 간접 참조와 널 체크만 늘었다.
	Core::ShapeType::Rect2f m_layout = {};
};

#pragma warning(pop)
