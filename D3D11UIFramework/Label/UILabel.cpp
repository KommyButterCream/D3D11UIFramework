#include "pch.h"
#include "UILabel.h"

#include "../../Modules/D3D11Engine/Font/FontManager.h"
#include "../../Modules/D3D11EngineInterface/IRenderContext.h"

#include <string.h>

UILabel::~UILabel()
{
	Shutdown();
}

bool UILabel::Initialize(IRenderContext* context)
{
	return CreateVisualResources(context, true);
}

void UILabel::Shutdown()
{
	m_hasText = false;

	if (m_text)
	{
		delete[] m_text;
		m_text = nullptr;
	}

	m_textFormat = nullptr;
	SafeRelease(m_textLayout);

	m_isFormatDirty = true;
	m_isLayoutDirty = true;

	m_fontManager = nullptr;

	ReleaseVisualResources();

	UIElementBase::Shutdown();
}

bool UILabel::Update(float dt)
{
	if (!IsVisible())
		return false;

	return UpdateVisualAnimation(dt, true);
}

bool UILabel::Render()
{
	ID2D1DeviceContext5* d2dContext = nullptr;
	if (!ResolveRenderContext(&d2dContext))
		return false;

	DrawBackground(d2dContext);
	DrawD2DText(d2dContext, GetTextRect());

	return true;
}

void UILabel::DiscardDeviceResources()
{
	SafeRelease(m_textLayout);
	m_textFormat = nullptr;
	m_isFormatDirty = true;
	m_isLayoutDirty = true;

	ReleaseVisualResources();
}

bool UILabel::RestoreDeviceResources(IRenderContext* context)
{
	if (!CreateVisualResources(context, false))
		return false;

	m_textFormat = nullptr;
	m_isFormatDirty = true;
	m_isLayoutDirty = true;
	EnsureTextUpdate();

	return true;
}

void UILabel::OnMouseEvent(UIMouseEventType type, float x, float y)
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

	case UIMouseEventType::Leave:
		m_mouseOver = false;
		SetState(UIElementState::Normal);
		break;

	default:
		break;
	}
}

void UILabel::OnStateChanged(UIElementState oldState, UIElementState newState)
{
	UpdateVisualTargets();
}

void UILabel::OnLayoutChanged()
{
	UpdateDrawRectCache();

	m_isLayoutDirty = true;
	EnsureTextUpdate();
}

void UILabel::SetFontManager(FontManager* fontManager)
{
	m_fontManager = fontManager;
}

void UILabel::SetRounded(bool enable)
{
	m_isRoundedRect = enable;
}

void UILabel::SetCornerRadius(float radius)
{
	m_cornerRadius = radius;
}

void UILabel::SetText(const wchar_t* text)
{
	if (!text)
		return;

	const size_t length = wcslen(text);

	if (length == 0)
	{
		// 빈 문자열 처리
		if (m_text)
			m_text[0] = L'\0';

		m_hasText = false;
		m_isLayoutDirty = true;
		return;
	}

	if (m_text && wcscmp(m_text, text) == 0)
		return;

	// 1. 버퍼 용량 확인
	if (length + 1 > m_textCapacity)
	{
		delete[] m_text;

		m_textCapacity = length + 1;
		m_text = new wchar_t[m_textCapacity];
	}

	if (!m_text)
		return;

	// 2. 내용 복사
	wcscpy_s(m_text, m_textCapacity, text);

	m_hasText = true;
	m_isLayoutDirty = true;
}

void UILabel::SetTextStyle(const UITextStyle& style)
{
	const bool formatChanged =
		style.fontSize != m_textStyle.fontSize ||
		style.weight != m_textStyle.weight ||
		wcscmp(style.fontName, m_textStyle.fontName) != 0;

	m_textStyle = style;

	if (formatChanged)
		m_isFormatDirty = true;
	else
		m_isLayoutDirty = true;

	UpdateVisualTargets();
}

void UILabel::SetTextPadding(float left, float top, float right, float bottom)
{
	m_textPadding.left = left;
	m_textPadding.top = top;
	m_textPadding.right = right;
	m_textPadding.bottom = bottom;
}

void UILabel::UpdateDrawRectCache()
{
	const auto& layout = LayoutData();

	if (m_isRoundedRect)
	{
		m_layoutRoundedRect.radiusX = m_cornerRadius;
		m_layoutRoundedRect.radiusY = m_cornerRadius;
		m_layoutRoundedRect.rect = {
			layout.left,
			layout.top,
			layout.right,
			layout.bottom };

	}
	else
	{
		m_layoutRect = {
			layout.left,
			layout.top,
			layout.right,
			layout.bottom
		};
	}
}

void UILabel::EnsureTextUpdate()
{
	if (!m_fontManager || !m_hasText)
		return;

	if (m_isFormatDirty)
	{
		UpdateTextResources();
		m_isLayoutDirty = true;
	}

	if (m_isLayoutDirty)
	{
		UpdateTextLayout();
	}

	m_isFormatDirty = false;
	m_isLayoutDirty = false;
}

void UILabel::UpdateTextResources()
{
	if (!m_fontManager || !m_hasText)
		return;

	m_textFormat = m_fontManager->GetTextFormat(
		m_textStyle.fontName,
		m_textStyle.fontSize,
		m_textStyle.weight
	);
}

void UILabel::UpdateTextLayout()
{
	if (!m_fontManager || !m_text || !m_textFormat)
		return;

	SafeRelease(m_textLayout);

	const auto& layout = LayoutData();
	const float contentWidth = layout.Width();
	const float contentHeight = layout.Height();

	if (contentWidth <= 0.0f || contentHeight <= 0.0f)
		return;

	IDWriteFactory* factory = m_fontManager->GetDWriteFactory();

	factory->CreateTextLayout(
		m_text,
		static_cast<UINT32>(wcslen(m_text)),
		m_textFormat,
		contentWidth - (m_textPadding.left + m_textPadding.right),
		contentHeight - (m_textPadding.top + m_textPadding.bottom),
		&m_textLayout
	);

	m_textLayout->SetTextAlignment(m_textStyle.hAlign);
	m_textLayout->SetParagraphAlignment(m_textStyle.vAlign);
}

bool UILabel::CreateVisualResources(IRenderContext* context, bool resetState)
{
	if (!BindRenderContext(context, resetState))
		return false;

	ID2D1DeviceContext* d2dContext = m_context->GetD2DDeviceContext();
	if (!d2dContext)
		return false;

	if (FAILED(d2dContext->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1), &m_fillBrush)))
		return false;

	if (FAILED(d2dContext->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1), &m_strokeBrush)))
		return false;

	if (FAILED(d2dContext->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1), &m_textBrush)))
		return false;

	m_smoothFillColor.Snap(m_style.normal.fill);
	m_smoothborderColor.Snap(m_style.normal.border);
	m_smoothTextColor.Snap(m_textStyle.normal.fill);
	UpdateVisualTargets();

	return true;
}

void UILabel::ReleaseVisualResources()
{
	SafeRelease(m_strokeBrush);
	SafeRelease(m_fillBrush);
	SafeRelease(m_textBrush);
}

void UILabel::UpdateVisualTargets()
{
	switch (m_state)
	{
	case UIElementState::Normal:
		m_smoothFillColor.SetTarget(m_style.normal.fill);
		m_smoothborderColor.SetTarget(m_style.normal.border);
		if (m_hasText)
			m_smoothTextColor.SetTarget(m_textStyle.normal.fill);
		break;

	case UIElementState::Hovered:
		m_smoothFillColor.SetTarget(m_style.hover.fill);
		m_smoothborderColor.SetTarget(m_style.hover.border);
		if (m_hasText)
			m_smoothTextColor.SetTarget(m_textStyle.hover.fill);
		break;

	case UIElementState::Pressed:
		m_smoothFillColor.SetTarget(m_style.pressed.fill);
		m_smoothborderColor.SetTarget(m_style.pressed.border);
		if (m_hasText)
			m_smoothTextColor.SetTarget(m_textStyle.pressed.fill);
		break;

	case UIElementState::Disabled:
		m_smoothFillColor.SetTarget(m_style.disabled.fill);
		m_smoothborderColor.SetTarget(m_style.disabled.border);
		if (m_hasText)
			m_smoothTextColor.SetTarget(m_textStyle.disabled.fill);
		break;
	}
}

bool UILabel::UpdateVisualAnimation(float dt, bool includeText)
{
	constexpr float animationSpeed = 14.0f;

	m_smoothFillColor.Update(dt, animationSpeed);
	m_smoothborderColor.Update(dt, animationSpeed);

	if (includeText && m_hasText)
	{
		m_smoothTextColor.Update(dt, animationSpeed);
	}

	return
		!m_smoothFillColor.IsAtTarget() ||
		!m_smoothborderColor.IsAtTarget() ||
		(includeText && !m_smoothTextColor.IsAtTarget());
}

bool UILabel::ResolveRenderContext(ID2D1DeviceContext5** d2dContext)
{
	if (!IsVisible() || !m_context || !d2dContext)
		return false;

	EnsureTextUpdate();

	*d2dContext = m_context->GetD2DDeviceContext5();
	return *d2dContext != nullptr;
}

void UILabel::DrawBackground(ID2D1DeviceContext* d2dContext)
{
	if (!d2dContext || !m_fillBrush || !m_strokeBrush)
		return;

	m_fillBrush->SetColor(m_smoothFillColor.GetColor());
	m_strokeBrush->SetColor(m_smoothborderColor.GetColor());

	if (m_isRoundedRect)
	{
		d2dContext->FillRoundedRectangle(m_layoutRoundedRect, m_fillBrush);
		if (m_style.borderThickness != 0.0f && m_style.normal.border.a != 0.0f)
		{
			d2dContext->DrawRoundedRectangle(m_layoutRoundedRect, m_strokeBrush, m_style.borderThickness);
		}
	}
	else
	{
		d2dContext->FillRectangle(m_layoutRect, m_fillBrush);
		if (m_style.borderThickness != 0.0f && m_style.normal.border.a != 0.0f)
		{
			d2dContext->DrawRectangle(m_layoutRect, m_strokeBrush, m_style.borderThickness);
		}
	}
}

void UILabel::DrawD2DText(ID2D1DeviceContext* d2dContext, const D2D1_RECT_F& textRect)
{
	if (!d2dContext || !m_textBrush || !m_hasText || !m_textLayout)
		return;

	m_textBrush->SetColor(m_smoothTextColor.GetColor());

	d2dContext->DrawTextLayout(
		D2D1::Point2F(textRect.left, textRect.top),
		m_textLayout,
		m_textBrush,
		D2D1_DRAW_TEXT_OPTIONS_CLIP
	);
}

D2D1_RECT_F UILabel::GetTextRect() const
{
	const auto& layout = LayoutData();

	return {
		layout.left + m_textPadding.left,
		layout.top + m_textPadding.top,
		layout.right,
		layout.bottom
	};
}
