#include "pch.h"
#include "UIButton.h"

#include "../../../Module/D3D11Engine/Font/FontManager.h"
#include "../../../Module/D3D11EngineInterface/IRenderContext.h"
#include "../Resource/UISVGResource.h"
#include "../Resource/UIIconHelper.h"
#include "../Event/UIEventDispatcher.h"

UIButton::~UIButton()
{
	Shutdown();
}

bool UIButton::Initialize(IRenderContext* context)
{
	if (!UILabel::Initialize(context))
		return false;

	m_smoothIconColor.Snap(m_iconStyle.normal.fill);
	m_smoothIconScale.Snap(1.0f);

	EnsureIconLoaded();

	return true;
}

void UIButton::Shutdown()
{
	UIIconHelper::ReleaseIconState(m_iconPath, m_icon, m_iconLoaded);

	m_dispatcher = nullptr;

	UILabel::Shutdown();
}

void UIButton::DiscardDeviceResources()
{
	UIIconHelper::ReleaseIcon(m_icon, m_iconLoaded);

	UILabel::DiscardDeviceResources();
}

bool UIButton::RestoreDeviceResources(IRenderContext* context)
{
	if (!UILabel::RestoreDeviceResources(context))
		return false;

	EnsureIconLoaded();
	OnStateChanged(m_state, m_state);

	return true;
}

bool UIButton::Update(float dt)
{
	if (!IsVisible())
		return false;

	const bool labelAnimating = UILabel::Update(dt);
	const bool iconAnimating = UIIconHelper::UpdateIconAnimation(
		m_iconLoaded,
		dt,
		14.0f,
		12.0f,
		m_smoothIconColor,
		m_smoothIconScale);

	return
		labelAnimating ||
		iconAnimating;
}

bool UIButton::Render()
{
	ID2D1DeviceContext5* d2dContext = nullptr;
	if (!ResolveRenderContext(&d2dContext))
		return false;

	DrawBackground(d2dContext);

	if (m_iconLoaded && m_icon)
	{
		const auto& layout = LayoutData();

		const float buttonWidth = layout.right - layout.left;
		const float buttonHeight = layout.bottom - layout.top;

		const float iconWidth = m_icon->GetViewBoxWidth();
		const float iconHeight = m_icon->GetViewBoxHeight();

		if (iconWidth <= 0.0f || iconHeight <= 0.0f)
		{
			return false;
		}

		const float targetSize = min(buttonWidth, buttonHeight) * m_iconScale;
		const float scale = min(targetSize / iconWidth, targetSize / iconHeight) * m_smoothIconScale.GetCurrent();
		if (!UIIconHelper::DrawCenteredIcon(
			d2dContext,
			m_icon,
			{ layout.left, layout.top, layout.right, layout.bottom },
			m_smoothIconColor.GetColor(),
			scale))
		{
			return false;
		}
	}

	if (m_hasText && m_textLayout)
	{
		const auto& layout = LayoutData();
		DrawD2DText(d2dContext, { layout.left, layout.top, layout.right, layout.bottom });
	}

	return true;
}

void UIButton::OnMouseEvent(UIMouseEventType type, float x, float y)
{
	if (!IsVisible())
		return;

	const bool hit = HitTest(x, y);

	switch (type)
	{
	case UIMouseEventType::Move:
		if (hit)
		{
			if (!m_mouseOver)
			{
				m_mouseOver = true;
				if (m_state != UIElementState::Pressed)
					SetState(UIElementState::Hovered);
			}
		}
		else
		{
			if (m_mouseOver)
			{
				m_mouseOver = false;
				SetState(UIElementState::Normal);
			}
		}
		break;

	case UIMouseEventType::LButtonDown:
		if (hit)
			SetState(UIElementState::Pressed);
		break;

	case UIMouseEventType::LButtonUp:
		if (m_state == UIElementState::Pressed)
		{
			if (hit)
				OnClick();

			if (hit && m_dispatcher)
				m_dispatcher->Dispatch(m_command);

			SetState(hit ? UIElementState::Hovered
				: UIElementState::Normal);
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

void UIButton::OnStateChanged(UIElementState oldState, UIElementState newState)
{
	UILabel::OnStateChanged(oldState, newState);

	UIIconHelper::ApplyIconStateTargets(
		newState,
		m_iconStyle,
		m_iconLoaded,
		m_smoothIconColor,
		m_smoothIconScale,
		1.0f,
		1.1f,
		1.2f);
}

void UIButton::OnLayoutChanged()
{
	UpdateDrawRectCache();
}

void UIButton::OnClick()
{
}

void UIButton::SetCommand(UICommand command)
{
	m_command = command;
}

void UIButton::SetEventDispatcher(UIEventDispatcher* dispatcher)
{
	m_dispatcher = dispatcher;
}

void UIButton::SetIcon(const wchar_t* path)
{
	UIIconHelper::ResetIconPath(m_iconPath, path, m_icon, m_iconLoaded);
}

void UIButton::SetIconScale(float scale)
{
	m_iconScale = scale;
}

void UIButton::SetIconStyle(const UIStyle& style)
{
	m_iconStyle = style;
}

UIStyle& UIButton::GetIconStyle()
{
	return m_iconStyle;
}

const UIStyle& UIButton::GetIconStyle() const
{
	return m_iconStyle;
}

void UIButton::EnsureIconLoaded()
{
	UIIconHelper::EnsureIconLoaded(m_context, m_iconPath, m_icon, m_iconLoaded);
}
