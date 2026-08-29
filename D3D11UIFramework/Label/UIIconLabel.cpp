#include "pch.h"
#include "UIIconLabel.h"

#include "../../../Module/D3D11Engine/Font/FontManager.h"
#include "../Resource/UISVGResource.h"
#include "../Resource/UIIcon.h"

UIIconLabel::UIIconLabel()
{
}

UIIconLabel::~UIIconLabel()
{
	Shutdown();
}

bool UIIconLabel::AcquireDeviceResources(IRenderContext* context, bool reset)
{
	if (!UILabel::AcquireDeviceResources(context, reset))
		return false;

	m_icon.EnsureLoaded(m_context);

	// 확보 직후 아이콘 색/배율을 현재 상태에 맞춘다.
	m_icon.SnapToState(m_state, 1.0f);

	return true;
}

void UIIconLabel::Shutdown()
{
	m_icon.Reset();

	UILabel::Shutdown();
}

void UIIconLabel::DiscardDeviceResources()
{
	m_icon.ReleaseDeviceResources();

	UILabel::DiscardDeviceResources();
}

bool UIIconLabel::Update(float dt)
{
	if (!IsVisible())
		return false;

	constexpr float animationSpeed = 14.0f;
	const bool iconAnimating = m_icon.UpdateAnimation(dt, animationSpeed, animationSpeed);

	const bool animating =
		UILabel::Update(dt) ||
		iconAnimating;

	return animating;
}

bool UIIconLabel::Render()
{
	ID2D1DeviceContext5* d2dContext = nullptr;
	if (!ResolveRenderContext(&d2dContext))
		return false;

	DrawBackground(d2dContext);

	// ==================================
	// ICON
	// ==================================
	m_icon.Draw(d2dContext, m_iconLabellayout.iconRect);

	// ==================================
	// TEXT (LEFT ALIGNED)
	// ==================================
	if (m_hasText && m_textLayout)
	{
		DrawD2DText(d2dContext, m_iconLabellayout.textRect);
	}

	return true;
}

void UIIconLabel::OnStateChanged(UIElementState oldState, UIElementState newState)
{
	m_icon.ApplyState(newState, 1.0f, 1.1f, 1.2f);

	UILabel::OnStateChanged(oldState, newState);
}

void UIIconLabel::SetIconAreaWidth(float width)
{
	m_iconAreaWidth = width;
}

void UIIconLabel::SetTextAreaWidth(float width)
{
	m_textAreaWidth = width;
}

void UIIconLabel::SetIconShape(UIIconShape shape)
{
	m_icon.SetShape(shape);
}

void UIIconLabel::SetIcon(const wchar_t* path)
{
	m_icon.SetPath(path);
}

void UIIconLabel::SetIconScale(float scale)
{
	m_icon.SetScale(scale);
}

void UIIconLabel::SetIconStyle(const UIStyle& style)
{
	m_icon.SetStyle(style);
}

UIStyle& UIIconLabel::GetIconStyle()
{
	return m_icon.Style();
}

const UIStyle& UIIconLabel::GetIconStyle() const
{
	return m_icon.Style();
}

void UIIconLabel::UpdateTextLayout()
{
	if (!m_fontManager || !m_text || !m_textFormat)
		return;

	SafeRelease(m_textLayout);

	const auto& layout = LayoutData();
	const float contentWidth = layout.Width();
	const float contentHeight = layout.Height();

	if (contentWidth <= 0.0f || contentHeight <= 0.0f)
		return;

	m_iconLabellayout = ComputeLayout();

	IDWriteFactory* factory = m_fontManager->GetDWriteFactory();

	factory->CreateTextLayout(
		m_text,
		static_cast<UINT32>(wcslen(m_text)),
		m_textFormat,
		(m_iconLabellayout.textRect.right - m_iconLabellayout.textRect.left)
		- (m_textPadding.left + m_textPadding.right),
		(m_iconLabellayout.textRect.bottom - m_iconLabellayout.textRect.top)
		- (m_textPadding.top + m_textPadding.bottom),
		&m_textLayout
	);

	m_textLayout->SetTextAlignment(m_textStyle.hAlign);
	m_textLayout->SetParagraphAlignment(m_textStyle.vAlign);
}

UIIconLabelLayout UIIconLabel::ComputeLayout() const
{
	UIIconLabelLayout layout = {};

	layout.iconRect = {
		LayoutData().left,
		LayoutData().top,
		LayoutData().left + m_iconAreaWidth,
		LayoutData().bottom
	};

	layout.textRect = {
		LayoutData().left + m_iconAreaWidth + m_textPadding.left,
		LayoutData().top + m_textPadding.top,
		LayoutData().right - m_textPadding.right,
		LayoutData().bottom - m_textPadding.bottom
	};

	return layout;
}
