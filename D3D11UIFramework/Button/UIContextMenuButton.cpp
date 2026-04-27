#include "pch.h"
#include "UIContextMenuButton.h"
#include "../Resource/UIIconHelper.h"

#include "../../../Module/D3D11Engine/Font/FontManager.h"
#include "../Resource/UISVGResource.h"

UIContextMenuButton::UIContextMenuButton()
{
}

UIContextMenuButton::~UIContextMenuButton()
{
	Shutdown();
}

bool UIContextMenuButton::Initialize(IRenderContext* context)
{
	if (!UIButton::Initialize(context))
		return false;

	SetVisible(true);

	return true;
}

void UIContextMenuButton::Shutdown()
{
	m_hasExtraText = false;

	if (m_extraText)
	{
		delete[] m_extraText;
		m_extraText = nullptr;
	}

	SafeRelease(m_extraTextLayout);

	UIButton::Shutdown();
}

bool UIContextMenuButton::Update(float dt)
{
	if (!IsVisible())
		return false;

	return UIButton::Update(dt);
}

bool UIContextMenuButton::Render()
{
	ID2D1DeviceContext5* d2dContext = nullptr;
	if (!ResolveRenderContext(&d2dContext))
		return false;

	DrawBackground(d2dContext);

	// === layout ===
	const ContextMenuLayout layout = ComputeLayout();

	// ==================================
	// ICON
	// ==================================
	if (!m_checkable)
	{
		if (m_iconLoaded && m_icon)
		{
			const float iconWidth = m_icon->GetViewBoxWidth();
			const float iconHeight = m_icon->GetViewBoxHeight();

			const float areaWidth = layout.iconRect.right - layout.iconRect.left;
			const float areaHeight = layout.iconRect.bottom - layout.iconRect.top;

			const float scale =
				min(areaWidth / iconWidth, areaHeight / iconHeight) *
				m_iconScale *
				m_smoothIconScale.GetCurrent();

			if (!UIIconHelper::DrawCenteredIcon(
				d2dContext,
				m_icon,
				layout.iconRect,
				m_smoothIconColor.GetColor(),
				scale))
			{
				return false;
			}
		}
	}
	else
	{
		// checkable 일 때
		if (m_iconLoaded && m_icon && m_checked)
		{
			if (m_isRoundedRect)
			{
				const D2D1_ROUNDED_RECT roundRect = { layout.iconRect, m_cornerRadius, m_cornerRadius };
				d2dContext->FillRoundedRectangle(roundRect, m_fillBrush);

				if (m_style.borderThickness != 0.0f && m_style.normal.border.a != 0.0f)
				{
					d2dContext->DrawRoundedRectangle(roundRect, m_strokeBrush, m_style.borderThickness);
				}
			}
			else
			{
				d2dContext->FillRectangle(layout.iconRect, m_fillBrush);

				if (m_style.borderThickness != 0.0f && m_style.normal.border.a != 0.0f)
				{
					d2dContext->DrawRectangle(layout.iconRect, m_strokeBrush, m_style.borderThickness);
				}
			}

			const float iconWidth = m_icon->GetViewBoxWidth();
			const float iconHeight = m_icon->GetViewBoxHeight();

			const float areaWidth = layout.iconRect.right - layout.iconRect.left;
			const float areaHeight = layout.iconRect.bottom - layout.iconRect.top;

			const float scale =
				min(areaWidth / iconWidth, areaHeight / iconHeight) *
				0.75f *
				m_smoothIconScale.GetCurrent();

			if (!UIIconHelper::DrawCenteredIcon(
				d2dContext,
				m_icon,
				layout.iconRect,
				m_smoothIconColor.GetColor(),
				scale))
			{
				return false;
			}
		}
	}


	// ==================================
	// TEXT (LEFT ALIGNED)
	// ==================================
	if (m_hasText && m_textLayout)
	{
		DrawD2DText(d2dContext, layout.textRect);
	}

	// ==================================
	// EXTRA TEXT (RIGHT)
	// ==================================
	if (m_extraTextLayout)
	{
		m_textBrush->SetColor(m_smoothTextColor.GetColor());

		d2dContext->DrawTextLayout(
			D2D1::Point2F(layout.extraRect.left, layout.extraRect.top),
			m_extraTextLayout,
			m_textBrush,
			D2D1_DRAW_TEXT_OPTIONS_CLIP);
	}

	return true;
}

void UIContextMenuButton::OnClick()
{
	if (m_checkable)
	{
		m_checked = !m_checked;
	}
}

void UIContextMenuButton::SetIconAreaWidth(float width)
{
	m_iconAreaWidth = width;
}

void UIContextMenuButton::SetExtraAreaWidth(float width)
{
	m_extraAreaWidth = width;
}

void UIContextMenuButton::SetExtraText(const wchar_t* text)
{
	if (m_extraText)
	{
		delete[] m_extraText;
		m_extraText = nullptr;
	}

	if (!text)
		return;

	const size_t length = wcslen(text);

	if (length == 0)
		return;

	m_extraText = new wchar_t[length + 1];
	wcscpy_s(m_extraText, length + 1, text);

	m_isLayoutDirty = true;
}

void UIContextMenuButton::SetCheckable(bool enable)
{
	m_checkable = enable;
}

bool UIContextMenuButton::IsChecked() const
{
	return m_checked;
}

void UIContextMenuButton::UpdateTextLayout()
{
	if (!m_fontManager || !m_text || !m_textFormat)
		return;

	SafeRelease(m_textLayout);
	SafeRelease(m_extraTextLayout);

	const ContextMenuLayout layout = ComputeLayout();

	IDWriteFactory* factory = m_fontManager->GetDWriteFactory();

	// main text
	factory->CreateTextLayout(
		m_text,
		static_cast<UINT32>(wcslen(m_text)),
		m_textFormat,
		layout.textRect.right - layout.textRect.left,
		layout.textRect.bottom - layout.textRect.top,
		&m_textLayout);

	m_textLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	m_textLayout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	// extra text
	if (m_extraText)
	{
		factory->CreateTextLayout(
			m_extraText,
			static_cast<UINT32>(wcslen(m_extraText)),
			m_textFormat,
			layout.extraRect.right - layout.extraRect.left,
			layout.extraRect.bottom - layout.extraRect.top,
			&m_extraTextLayout);

		m_extraTextLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
		m_extraTextLayout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}
}

ContextMenuLayout UIContextMenuButton::ComputeLayout() const
{
	ContextMenuLayout layout = {};

	layout.iconRect = {
		LayoutData().left,
		LayoutData().top,
		LayoutData().left + m_iconAreaWidth,
		LayoutData().bottom
	};

	layout.extraRect = {
		LayoutData().right - m_extraAreaWidth - m_extraAreaOffset,
		LayoutData().top,
		LayoutData().right - m_extraAreaOffset,
		LayoutData().bottom
	};

	layout.textRect = {
		layout.iconRect.right + 2,
		LayoutData().top,
		layout.extraRect.left,
		LayoutData().bottom
	};

	return layout;
}
