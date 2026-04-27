#include "pch.h"
#include "UIIconLabel.h"

#include "../../../Module/D3D11Engine/Font/FontManager.h"
#include "../Resource/UISVGResource.h"
#include "../Resource/UIIconHelper.h"

UIIconLabel::UIIconLabel()
{
}

UIIconLabel::~UIIconLabel()
{
	Shutdown();
}

bool UIIconLabel::Initialize(IRenderContext* context)
{
	if (!UILabel::Initialize(context))
		return false;

	m_smoothIconColor.Snap(m_iconStyle.normal.fill);
	m_smoothIconScale.Snap(1.0f);

	EnsureIconLoaded();

	return true;
}

void UIIconLabel::Shutdown()
{
	UIIconHelper::ReleaseIconState(m_iconPath, m_icon, m_iconLoaded);

	UILabel::Shutdown();
}

void UIIconLabel::DiscardDeviceResources()
{
	UIIconHelper::ReleaseIcon(m_icon, m_iconLoaded);

	UILabel::DiscardDeviceResources();
}

bool UIIconLabel::RestoreDeviceResources(IRenderContext* context)
{
	if (!UILabel::RestoreDeviceResources(context))
		return false;

	EnsureIconLoaded();
	OnStateChanged(m_state, m_state);

	return true;
}

bool UIIconLabel::Update(float dt)
{
	if (!IsVisible())
		return false;

	constexpr float animationSpeed = 14.0f;
	const bool iconAnimating = UIIconHelper::UpdateIconAnimation(
		m_iconLoaded,
		dt,
		animationSpeed,
		animationSpeed,
		m_smoothIconColor,
		m_smoothIconScale);

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
	if (m_iconLoaded && m_icon)
	{
		const float iconWidth = m_icon->GetViewBoxWidth();
		const float iconHeight = m_icon->GetViewBoxHeight();
		if (iconWidth <= 0.0f || iconHeight <= 0.0f)
		{
			return false;
		}

		const float areaWidth = m_iconLabellayout.iconRect.right - m_iconLabellayout.iconRect.left;
		const float areaHeight = m_iconLabellayout.iconRect.bottom - m_iconLabellayout.iconRect.top;

		const float scale =
			min(areaWidth / iconWidth, areaHeight / iconHeight) *
			m_iconScale *
			m_smoothIconScale.GetCurrent();

		if (!UIIconHelper::DrawCenteredIcon(
			d2dContext,
			m_icon,
			m_iconLabellayout.iconRect,
			m_smoothIconColor.GetColor(),
			scale))
		{
			return false;
		}
	}

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
	UIIconHelper::ApplyIconStateTargets(
		newState,
		m_iconStyle,
		m_iconLoaded,
		m_smoothIconColor,
		m_smoothIconScale,
		1.0f,
		1.1f,
		1.2f);

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

void UIIconLabel::SetIcon(const wchar_t* path)
{
	UIIconHelper::ResetIconPath(m_iconPath, path, m_icon, m_iconLoaded);
}

void UIIconLabel::SetIconScale(float scale)
{
	m_iconScale = scale;
}

void UIIconLabel::SetIconStyle(const UIStyle& style)
{
	m_iconStyle = style;
}

UIStyle& UIIconLabel::GetIconStyle()
{
	return m_iconStyle;
}

const UIStyle& UIIconLabel::GetIconStyle() const
{
	return m_iconStyle;
}

void UIIconLabel::EnsureIconLoaded()
{
	UIIconHelper::EnsureIconLoaded(m_context, m_iconPath, m_icon, m_iconLoaded);
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
