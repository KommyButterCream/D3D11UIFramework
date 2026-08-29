#include "pch.h"
#include "UIButton.h"

#include "../../../Module/D3D11Engine/Font/FontManager.h"
#include "../../../Module/D3D11EngineInterface/IRenderContext.h"
#include "../Resource/UISVGResource.h"
#include "../Resource/UIIcon.h"
#include "../Event/UIEventDispatcher.h"

UIButton::~UIButton()
{
	Shutdown();
}

bool UIButton::AcquireDeviceResources(IRenderContext* context, bool reset)
{
	if (!UILabel::AcquireDeviceResources(context, reset))
		return false;

	m_icon.EnsureLoaded(m_context);

	// 확보 직후 아이콘 색/배율을 현재 상태에 맞춘다.
	m_icon.SnapToState(m_state, 1.0f);

	return true;
}

void UIButton::Shutdown()
{
	m_icon.Reset();

	m_dispatcher = nullptr;

	UILabel::Shutdown();
}

void UIButton::DiscardDeviceResources()
{
	m_icon.ReleaseDeviceResources();

	UILabel::DiscardDeviceResources();
}

bool UIButton::Update(float dt)
{
	if (!IsVisible())
		return false;

	const bool labelAnimating = UILabel::Update(dt);
	const bool iconAnimating = m_icon.UpdateAnimation(dt, 14.0f, 12.0f);

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

	if (m_icon.IsLoaded())
	{
		const auto& layout = LayoutData();
		m_icon.Draw(d2dContext, { layout.left, layout.top, layout.right, layout.bottom });
	}

	if (m_hasText && m_textLayout)
	{
		const auto& layout = LayoutData();
		DrawD2DText(d2dContext, { layout.left, layout.top, layout.right, layout.bottom });
	}

	return true;
}

// 클릭이 성립한 지점. 상태 머신은 UIElementBase 가 전부 처리하고
// 버튼은 "그래서 무엇을 하는가" 만 담당한다.
void UIButton::OnActivated()
{
	OnClick();

	if (m_dispatcher)
	{
		m_dispatcher->Dispatch(m_command);
	}
}

void UIButton::OnStateChanged(UIElementState oldState, UIElementState newState)
{
	UILabel::OnStateChanged(oldState, newState);

	// hover 에 살짝, 누르면 조금 더 커진다.
	m_icon.ApplyState(newState, 1.0f, 1.1f, 1.2f);
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
	m_icon.SetPath(path);
}

void UIButton::SetIconScale(float scale)
{
	m_icon.SetScale(scale);
}

void UIButton::SetIconStyle(const UIStyle& style)
{
	m_icon.SetStyle(style);
}

UIStyle& UIButton::GetIconStyle()
{
	return m_icon.Style();
}

const UIStyle& UIButton::GetIconStyle() const
{
	return m_icon.Style();
}
