#include "pch.h"
#include "UIElementBase.h"

#include "../../../Module/Core/ShapeType/Point2f.h"
#include "../../../Module/D3D11EngineInterface/IRenderContext.h"

class UIElementBaseImpl
{
public:
	Rect2f layout = {};
};

UIElementBase::UIElementBase()
	: m_impl(new UIElementBaseImpl())
{
}

UIElementBase::~UIElementBase()
{
	delete m_impl;
	m_impl = nullptr;
}

bool UIElementBase::Initialize(IRenderContext* context)
{
	return BindRenderContext(context, true);
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

void UIElementBase::OnMouseEvent(UIMouseEventType type, float x, float y)
{
	if (!IsVisible())
		return;

	const bool hit = HitTest(x, y);

	switch (type)
	{
	case UIMouseEventType::Move:
		if (hit && !m_mouseOver)
		{
			m_mouseOver = true;
			SetState(UIElementState::Hovered);
		}
		else if (!hit && m_mouseOver)
		{
			m_mouseOver = false;
			SetState(UIElementState::Normal);
		}
		break;

	case UIMouseEventType::LButtonDown:
		if (hit)
		{
			SetState(UIElementState::Pressed);
		}
		break;

	case UIMouseEventType::LButtonUp:
		if (m_state == UIElementState::Pressed)
		{
			SetState(hit ? UIElementState::Hovered : UIElementState::Normal);
		}
		break;

	case UIMouseEventType::Leave:
		m_mouseOver = false;
		SetState(UIElementState::Normal);
		break;

	default:
		break;
	}
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
}

UIStyle& UIElementBase::GetStyle()
{
	return m_style;
}

const UIStyle& UIElementBase::GetStyle() const
{
	return m_style;
}

Rect2f& UIElementBase::LayoutData()
{
	return m_impl->layout;
}

const Rect2f& UIElementBase::LayoutData() const
{
	return m_impl->layout;
}

void UIElementBase::OnStateChanged(UIElementState oldState, UIElementState newState)
{
}

void UIElementBase::OnLayoutChanged()
{
}

void UIElementBase::DiscardDeviceResources()
{
}

bool UIElementBase::RestoreDeviceResources(IRenderContext* context)
{
	return BindRenderContext(context, false);
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
