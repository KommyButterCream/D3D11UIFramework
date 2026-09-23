#include "pch.h"
#include "UISlider.h"

#include "../../../Module/D3D11EngineInterface/IRenderContext.h"

// Rect2f::Center() 가 Point2f 를 값으로 돌려준다. Rect2f.h 는 전방 선언만
// 하고 있으므로 쓰는 쪽에서 정의를 가져와야 한다. (UISplitBar.cpp 와 같다)
#include "../../../Module/Core/ShapeType/Point2f.h"

#include <algorithm>

namespace
{
	float Clamp01(float value)
	{
		return (value < 0.0f) ? 0.0f : ((value > 1.0f) ? 1.0f : value);
	}
}

UISlider::~UISlider()
{
	Shutdown();
}

bool UISlider::AcquireDeviceResources(IRenderContext* context, bool reset)
{
	return CreateBrushes(context, reset);
}

void UISlider::Shutdown()
{
	SafeRelease(m_trackBrush);
	SafeRelease(m_fillBrush);
	SafeRelease(m_secondaryBrush);
	SafeRelease(m_thumbBrush);

	m_dragging = false;

	UIElementBase::Shutdown();
}

void UISlider::DiscardDeviceResources()
{
	SafeRelease(m_trackBrush);
	SafeRelease(m_fillBrush);
	SafeRelease(m_secondaryBrush);
	SafeRelease(m_thumbBrush);
}

// 애니메이션이 없다. 값이 바뀌는 순간에만 다시 그리면 되고, 그 시점은
// 호스트가 안다(SetValue 를 부른 쪽이다).
bool UISlider::Update(float)
{
	return false;
}

bool UISlider::Render()
{
	if (!IsVisible() || !m_context)
		return false;

	if (!m_trackBrush || !m_fillBrush || !m_secondaryBrush || !m_thumbBrush)
		return false;

	ID2D1DeviceContext* d2dContext = m_context->GetD2DDeviceContext();
	if (!d2dContext)
		return false;

	if (m_updateColor)
	{
		m_trackBrush->SetColor(m_trackColor);
		m_fillBrush->SetColor(m_fillColor);
		m_secondaryBrush->SetColor(m_secondaryColor);
		m_thumbBrush->SetColor(m_thumbColor);
		m_updateColor = false;
	}

	const Rect2f track = TrackRect();
	const float radius = m_trackThickness * 0.5f;

	// 트랙 전체
	{
		const D2D1_ROUNDED_RECT rounded = D2D1::RoundedRect(
			D2D1::RectF(track.left, track.top, track.right, track.bottom), radius, radius);
		d2dContext->FillRoundedRectangle(rounded, m_trackBrush);
	}

	// 두 번째 구간을 먼저 그린다. 값 구간이 그 위에 덮여야 한다 —
	// 버퍼보다 재생 위치가 앞설 수는 없으므로 순서를 뒤집으면 가려진다.
	if (m_hasSecondaryValue)
	{
		const float ratio = ValueToRatio(m_secondaryValue);
		if (ratio > 0.0f)
		{
			D2D1_RECT_F filled = {};
			if (m_orientation == UISliderOrientation::Horizontal)
			{
				filled = D2D1::RectF(track.left, track.top,
					track.left + (track.right - track.left) * ratio, track.bottom);
			}
			else
			{
				// 세로는 아래가 0 이다. 볼륨 슬라이더의 관례를 따른다.
				filled = D2D1::RectF(track.left,
					track.bottom - (track.bottom - track.top) * ratio, track.right, track.bottom);
			}

			d2dContext->FillRoundedRectangle(
				D2D1::RoundedRect(filled, radius, radius), m_secondaryBrush);
		}
	}

	// 값 구간
	const float valueRatio = ValueToRatio(m_value);
	if (valueRatio > 0.0f)
	{
		D2D1_RECT_F filled = {};
		if (m_orientation == UISliderOrientation::Horizontal)
		{
			filled = D2D1::RectF(track.left, track.top,
				track.left + (track.right - track.left) * valueRatio, track.bottom);
		}
		else
		{
			filled = D2D1::RectF(track.left,
				track.bottom - (track.bottom - track.top) * valueRatio, track.right, track.bottom);
		}

		d2dContext->FillRoundedRectangle(
			D2D1::RoundedRect(filled, radius, radius), m_fillBrush);
	}

	// 썸. 반지름 0 이면 표시 전용 바이므로 그리지 않는다.
	if (m_thumbRadius > 0.0f)
	{
		const auto& layout = LayoutData();
		D2D1_POINT_2F center = {};

		if (m_orientation == UISliderOrientation::Horizontal)
		{
			center.x = track.left + (track.right - track.left) * valueRatio;
			center.y = layout.Center().y;
		}
		else
		{
			center.x = layout.Center().x;
			center.y = track.bottom - (track.bottom - track.top) * valueRatio;
		}

		d2dContext->FillEllipse(
			D2D1::Ellipse(center, m_thumbRadius, m_thumbRadius), m_thumbBrush);
	}

	return true;
}

bool UISlider::OnMouseEvent(UIMouseEventType type, float x, float y)
{
	if (!IsVisible())
		return false;

	if (!m_interactive || m_state == UIElementState::Disabled)
	{
		// 표시 전용이어도 영역 안이면 소비한다. 바 위를 눌렀는데 그 클릭이
		// 뒤의 이미지로 새어 팬이 시작되면 안 된다.
		return (type != UIMouseEventType::Leave) && HitTest(x, y);
	}

	// 드래그 중에는 커서가 요소를 벗어나도 값이 따라와야 한다.
	// 기반 클래스의 Move 처리는 HitTest 로 걸러내므로 그 앞에서 가로챈다.
	if (m_dragging)
	{
		switch (type)
		{
		case UIMouseEventType::Move:
			ApplyValue(PositionToValue(x, y), true);
			return true;

		case UIMouseEventType::LButtonUp:
			ApplyValue(PositionToValue(x, y), true);
			m_dragging = false;
			break;

		case UIMouseEventType::Leave:
			// 창을 벗어났다. 값은 마지막 것으로 두고 드래그만 끝낸다.
			m_dragging = false;
			break;

		default:
			break;
		}
	}
	else if (type == UIMouseEventType::LButtonDown && HitTest(x, y))
	{
		// 트랙 아무 데나 눌러도 그 위치로 값이 간다. 썸을 정확히 집을
		// 필요가 없어야 손이 편하다.
		m_dragging = true;
		ApplyValue(PositionToValue(x, y), true);
	}

	// 상태(hover / pressed) 전이는 기반 클래스에 맡긴다.
	const bool consumed = UIElementBase::OnMouseEvent(type, x, y);

	return consumed || m_dragging;
}

void UISlider::SetRange(float minimum, float maximum)
{
	if (minimum >= maximum)
		return;

	m_minimum = minimum;
	m_maximum = maximum;

	ApplyValue(m_value, false);
}

void UISlider::SetValue(float value)
{
	ApplyValue(value, false);
}

float UISlider::GetValue() const
{
	return m_value;
}

void UISlider::SetSecondaryValue(float value)
{
	m_secondaryValue = std::clamp(value, m_minimum, m_maximum);
	m_hasSecondaryValue = true;
}

void UISlider::ClearSecondaryValue()
{
	m_hasSecondaryValue = false;
}

void UISlider::SetInteractive(bool interactive)
{
	m_interactive = interactive;

	if (!interactive)
	{
		m_dragging = false;
	}
}

bool UISlider::IsInteractive() const
{
	return m_interactive;
}

void UISlider::SetOrientation(UISliderOrientation orientation)
{
	m_orientation = orientation;
}

void UISlider::SetTrackThickness(float thickness)
{
	m_trackThickness = (thickness > 0.0f) ? thickness : 1.0f;
}

void UISlider::SetThumbRadius(float radius)
{
	m_thumbRadius = (radius > 0.0f) ? radius : 0.0f;
}

void UISlider::SetTrackColor(const D2D1_COLOR_F& color)
{
	m_trackColor = color;
	m_updateColor = true;
}

void UISlider::SetFillColor(const D2D1_COLOR_F& color)
{
	m_fillColor = color;
	m_updateColor = true;
}

void UISlider::SetSecondaryColor(const D2D1_COLOR_F& color)
{
	m_secondaryColor = color;
	m_updateColor = true;
}

void UISlider::SetThumbColor(const D2D1_COLOR_F& color)
{
	m_thumbColor = color;
	m_updateColor = true;
}

void UISlider::SetValueChangedCallback(ValueChangedCallback callback, void* userData)
{
	m_valueChangedCallback = callback;
	m_valueChangedUserData = userData;
}

bool UISlider::IsDragging() const
{
	return m_dragging;
}

bool UISlider::CreateBrushes(IRenderContext* context, bool resetState)
{
	if (!BindRenderContext(context, resetState))
		return false;

	ID2D1DeviceContext* d2dContext = m_context->GetD2DDeviceContext();
	if (!d2dContext)
		return false;

	SafeRelease(m_trackBrush);
	SafeRelease(m_fillBrush);
	SafeRelease(m_secondaryBrush);
	SafeRelease(m_thumbBrush);

	if (FAILED(d2dContext->CreateSolidColorBrush(m_trackColor, &m_trackBrush)))
		return false;

	if (FAILED(d2dContext->CreateSolidColorBrush(m_fillColor, &m_fillBrush)))
		return false;

	if (FAILED(d2dContext->CreateSolidColorBrush(m_secondaryColor, &m_secondaryBrush)))
		return false;

	if (FAILED(d2dContext->CreateSolidColorBrush(m_thumbColor, &m_thumbBrush)))
		return false;

	m_updateColor = false;
	return true;
}

Rect2f UISlider::TrackRect() const
{
	const auto& layout = LayoutData();
	const float half = m_trackThickness * 0.5f;

	Rect2f track = {};

	if (m_orientation == UISliderOrientation::Horizontal)
	{
		const float centerY = layout.Center().y;

		// 썸이 양 끝에서 잘리지 않도록 트랙을 반지름만큼 안으로 들인다.
		track.left = layout.left + m_thumbRadius;
		track.right = layout.right - m_thumbRadius;
		track.top = centerY - half;
		track.bottom = centerY + half;
	}
	else
	{
		const float centerX = layout.Center().x;

		track.left = centerX - half;
		track.right = centerX + half;
		track.top = layout.top + m_thumbRadius;
		track.bottom = layout.bottom - m_thumbRadius;
	}

	return track;
}

float UISlider::ValueToRatio(float value) const
{
	const float span = m_maximum - m_minimum;
	if (span <= 0.0f)
		return 0.0f;

	return Clamp01((value - m_minimum) / span);
}

float UISlider::PositionToValue(float x, float y) const
{
	const Rect2f track = TrackRect();
	float ratio = 0.0f;

	if (m_orientation == UISliderOrientation::Horizontal)
	{
		const float width = track.right - track.left;
		ratio = (width > 0.0f) ? Clamp01((x - track.left) / width) : 0.0f;
	}
	else
	{
		const float height = track.bottom - track.top;
		ratio = (height > 0.0f) ? Clamp01((track.bottom - y) / height) : 0.0f;
	}

	return m_minimum + (m_maximum - m_minimum) * ratio;
}

void UISlider::ApplyValue(float value, bool notify)
{
	const float clamped = std::clamp(value, m_minimum, m_maximum);
	if (clamped == m_value)
		return;

	m_value = clamped;

	// 통지는 사용자가 움직인 경우에만 한다. 호스트가 SetValue 로 넣은 값을
	// 되돌려 주면 호스트가 자기 변경에 반응하게 되고, 그 경로가 다시
	// SetValue 를 부르면 순환한다.
	if (notify && m_valueChangedCallback)
	{
		m_valueChangedCallback(m_value, m_valueChangedUserData);
	}
}
