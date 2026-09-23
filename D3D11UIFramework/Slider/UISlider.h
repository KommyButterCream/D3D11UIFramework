#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "../Base/UIElementBase.h"

enum class UISliderOrientation
{
	Horizontal,
	Vertical
};

// 값 하나를 트랙 위에서 고르는 요소.
//
// 두 가지 용도를 하나로 덮는다.
//
//   상호작용  : 볼륨처럼 사용자가 끌어서 바꾸는 값
//   표시 전용 : 버퍼 점유나 지연처럼 보여주기만 하는 값 (SetInteractive(false))
//
// 표시 전용을 별도 요소로 두지 않는 이유는 그리는 것이 같기 때문이다.
// 다른 것은 입력을 받느냐뿐이고, 그건 플래그 하나로 갈린다.
class D3D11_UI_FRAMEWORK_INTERFACE_API UISlider : public UIElementBase
{
public:
	// 드래그 중에도 매 갱신마다 불린다. 드래그가 끝나는 시점은
	// UIMouseEventType::LButtonUp 을 받은 호스트가 안다.
	using ValueChangedCallback = void (*)(float value, void* userData);

	UISlider() = default;
	virtual ~UISlider();

public:
	// IRenderLayer Override
	void Shutdown() override;
	bool Update(float dt) override;
	bool Render() override;
	void DiscardDeviceResources() override;

	// IUIRenderLayer Override
	//
	// 기반 클래스의 상태 머신을 그대로 쓰되, 드래그 구간만 앞에서 가로챈다.
	// 누른 채 요소 밖으로 나가도 값이 따라와야 하므로 그 동안은 HitTest 와
	// 무관하게 Move 를 소비한다.
	bool OnMouseEvent(UIMouseEventType type, float x, float y) override;

public:
	// 최솟값이 최댓값 이상이면 무시한다. 범위가 뒤집히면 비율 계산이
	// 음수가 되어 썸이 트랙 밖으로 나간다.
	void SetRange(float minimum, float maximum);
	void SetValue(float value);
	float GetValue() const;

	// 값과 별개로 채워 두는 두 번째 구간.
	//
	// 스트리밍에서 "지금 재생 위치" 와 "확보된 버퍼" 가 다른 것처럼,
	// 한 트랙에 두 가지를 겹쳐 보여야 하는 경우에 쓴다.
	// 설정하지 않으면 그리지 않는다.
	void SetSecondaryValue(float value);
	void ClearSecondaryValue();

	void SetInteractive(bool interactive);
	bool IsInteractive() const;

	void SetOrientation(UISliderOrientation orientation);
	void SetTrackThickness(float thickness);

	// 0 이면 썸을 그리지 않는다. 표시 전용 바에서 쓴다.
	void SetThumbRadius(float radius);

	void SetTrackColor(const D2D1_COLOR_F& color);
	void SetFillColor(const D2D1_COLOR_F& color);
	void SetSecondaryColor(const D2D1_COLOR_F& color);
	void SetThumbColor(const D2D1_COLOR_F& color);

	void SetValueChangedCallback(ValueChangedCallback callback, void* userData);

	// 지금 끌고 있는 중인가. 부모가 Move 를 계속 보내야 하는지 판단한다.
	bool IsDragging() const;

protected:
	// UIElementBase Override
	bool AcquireDeviceResources(IRenderContext* context, bool reset) override;

private:
	bool CreateBrushes(IRenderContext* context, bool resetState);

	// 트랙이 실제로 차지하는 사각형. 레이아웃에서 두께만큼만 가운데로 좁힌다.
	Core::ShapeType::Rect2f TrackRect() const;

	// 값 -> 0..1. 범위가 0 이면 0 을 돌려준다(0 나누기 방지).
	float ValueToRatio(float value) const;

	// 커서 위치 -> 값. 트랙 밖이면 양 끝으로 물린다.
	float PositionToValue(float x, float y) const;

	void ApplyValue(float value, bool notify);

private:
	float m_minimum = 0.0f;
	float m_maximum = 1.0f;
	float m_value = 0.0f;

	float m_secondaryValue = 0.0f;
	bool m_hasSecondaryValue = false;

	bool m_interactive = true;
	bool m_dragging = false;

	UISliderOrientation m_orientation = UISliderOrientation::Horizontal;
	float m_trackThickness = 4.0f;
	float m_thumbRadius = 6.0f;

	// 색을 바꿔도 브러시가 아직 없을 수 있다(Initialize 이전 구성 단계).
	// 그때는 값만 들고 있다가 브러시를 만들 때 함께 반영한다.
	bool m_updateColor = true;
	D2D1_COLOR_F m_trackColor = { 0.35f, 0.35f, 0.35f, 1.0f };
	D2D1_COLOR_F m_fillColor = { 0.90f, 0.90f, 0.90f, 1.0f };
	D2D1_COLOR_F m_secondaryColor = { 0.55f, 0.55f, 0.55f, 1.0f };
	D2D1_COLOR_F m_thumbColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	ID2D1SolidColorBrush* m_trackBrush = nullptr;
	ID2D1SolidColorBrush* m_fillBrush = nullptr;
	ID2D1SolidColorBrush* m_secondaryBrush = nullptr;
	ID2D1SolidColorBrush* m_thumbBrush = nullptr;

	ValueChangedCallback m_valueChangedCallback = nullptr;
	void* m_valueChangedUserData = nullptr;
};
