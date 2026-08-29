#include "pch.h"
#include "UIElementBase.h"

#include "../../../Module/Core/ShapeType/Point2f.h"
#include "../../../Module/D3D11EngineInterface/IRenderContext.h"

UIElementBase::UIElementBase()
{
}

UIElementBase::~UIElementBase()
{
}

bool UIElementBase::Initialize(IRenderContext* context)
{
	return AcquireDeviceResources(context, true);
}

bool UIElementBase::RestoreDeviceResources(IRenderContext* context)
{
	return AcquireDeviceResources(context, false);
}

bool UIElementBase::AcquireDeviceResources(IRenderContext* context, bool reset)
{
	// 잎 요소의 최소 동작. 컨텍스트를 붙이는 것 외에 만들 리소스가 없다.
	return BindRenderContext(context, reset);
}

void UIElementBase::Shutdown()
{
	m_context = nullptr;
}

bool UIElementBase::Prepare()
{
	return true;
}

bool UIElementBase::HitTest(float x, float y) const
{
	if (!IsVisible())
		return false;

	return LayoutData().Contains({ x, y });
}

// 모든 잎 요소가 공유하는 단 하나의 상태 머신.
//
// 예전에는 UIElementBase / UILabel / UIButton 이 이 switch 를 각자 복제하고
// 있었고 미묘하게 달랐다(base 만 Pressed 중에도 Hovered 로 덮어썼다).
// 지금은 여기 하나뿐이고, 파생 클래스는 OnActivated() 훅만 재정의한다.
bool UIElementBase::OnMouseEvent(UIMouseEventType type, float x, float y)
{
	if (!IsVisible())
		return false;

	const bool hit = HitTest(x, y);

	switch (type)
	{
	case UIMouseEventType::Move:
		if (hit)
		{
			if (!m_mouseOver)
			{
				m_mouseOver = true;

				// 누른 채로 벗어났다 돌아온 경우 Pressed 를 유지해야 한다.
				if (m_state != UIElementState::Pressed)
				{
					SetState(UIElementState::Hovered);
				}
			}
		}
		else if (m_mouseOver)
		{
			m_mouseOver = false;
			SetState(UIElementState::Normal);
		}
		return hit;

	case UIMouseEventType::LButtonDown:
		if (hit)
		{
			SetState(UIElementState::Pressed);
		}
		return hit;

	case UIMouseEventType::LButtonUp:
		if (m_state == UIElementState::Pressed)
		{
			// 누른 곳에서 뗐을 때만 활성화다. 밖에서 떼면 취소된다.
			if (hit)
			{
				OnActivated();
			}

			SetState(hit ? UIElementState::Hovered : UIElementState::Normal);
			return hit;
		}
		return false;

	case UIMouseEventType::Leave:
		m_mouseOver = false;
		SetState(UIElementState::Normal);

		// Leave 에는 소비 개념이 없다. 정리 통지일 뿐이다.
		return false;

	default:
		return false;
	}
}

void UIElementBase::OnActivated()
{
}

UIElementState UIElementBase::GetState() const
{
	return m_state;
}

void UIElementBase::SetState(UIElementState state)
{
	if (m_state == state)
		return;

	UIElementState old = m_state;
	m_state = state;

	OnStateChanged(old, m_state);
}

bool UIElementBase::IsVisible() const
{
	return m_visible;
}

void UIElementBase::SetVisible(bool visible)
{
	if (m_visible == visible)
		return;

	m_visible = visible;

	if (!IsVisible())
	{
		m_mouseOver = false;
		SetState(UIElementState::Normal);
	}
}

void UIElementBase::SetLayout(const Rect2f& rect)
{
	LayoutData() = rect;
}

const Rect2f& UIElementBase::GetLayout() const
{
	return LayoutData();
}

void UIElementBase::SetStyle(const UIStyle& style)
{
	m_style = style;
	OnStyleChanged();
}

UIStyle& UIElementBase::GetStyle()
{
	return m_style;
}

const UIStyle& UIElementBase::GetStyle() const
{
	return m_style;
}

void UIElementBase::OnStateChanged(UIElementState oldState, UIElementState newState)
{
}

void UIElementBase::OnStyleChanged()
{
}

void UIElementBase::OnLayoutChanged()
{
}

void UIElementBase::DiscardDeviceResources()
{
}

bool UIElementBase::BindRenderContext(IRenderContext* context, bool resetState)
{
	if (!context)
		return false;

	m_context = context;

	if (resetState)
	{
		m_visible = true;
		m_state = UIElementState::Normal;
		m_mouseOver = false;
	}

	return true;
}
