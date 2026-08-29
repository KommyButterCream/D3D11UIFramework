#include "pch.h"
#include "UIContextMenuButton.h"
#include "../Resource/UIIcon.h"

#include "../../../Module/D3D11Engine/Font/FontManager.h"
#include "../Resource/UISVGResource.h"

UIContextMenuButton::UIContextMenuButton()
{
}

UIContextMenuButton::~UIContextMenuButton()
{
	Shutdown();
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
		m_icon.Draw(d2dContext, layout.iconRect);
	}
	else
	{
		// checkable 일 때
		if (m_icon.IsLoaded() && m_checked)
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

			// 체크 표시는 배경 판 위에 얹히므로 본래 아이콘보다 작게 그린다.
			m_icon.Draw(d2dContext, layout.iconRect, 0.75f);
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
	// EXTRA TEXT (RIGHT)  또는  하위 메뉴 꺾쇠
	// ==================================
	//
	// 둘은 같은 자리를 쓴다. 하위 메뉴 항목에 단축키를 같이 다는 UI 는
	// 없으므로 배타로 둔다.
	if (m_hasSubMenu)
	{
		DrawSubMenuArrow(d2dContext, layout.extraRect);
	}
	else if (m_extraTextLayout)
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

void UIContextMenuButton::DrawSubMenuArrow(ID2D1DeviceContext* d2dContext,
	const D2D1_RECT_F& extraRect) const
{
	if (!d2dContext || !m_textBrush)
		return;

	m_textBrush->SetColor(m_smoothTextColor.GetColor());

	// extraRect 오른쪽 끝에 붙이고 세로는 가운데.
	const float tipX = extraRect.right;
	const float baseX = tipX - m_arrowHalfHeight;
	const float centerY = (extraRect.top + extraRect.bottom) * 0.5f;

	// 선 두 개로 '>' 를 만든다. 끝을 둥글게 이으면 꺾이는 지점이 깔끔하다.
	d2dContext->DrawLine(
		D2D1::Point2F(baseX, centerY - m_arrowHalfHeight),
		D2D1::Point2F(tipX, centerY),
		m_textBrush,
		m_arrowThickness);

	d2dContext->DrawLine(
		D2D1::Point2F(tipX, centerY),
		D2D1::Point2F(baseX, centerY + m_arrowHalfHeight),
		m_textBrush,
		m_arrowThickness);
}

void UIContextMenuButton::OnActivated()
{
	// 하위 메뉴를 여는 항목은 커맨드를 내지 않는다.
	// 실제로 여는 것은 UIContextMenuPanel 이 클릭을 보고 처리한다.
	if (m_hasSubMenu)
		return;

	UIButton::OnActivated();
}

void UIContextMenuButton::SetHasSubMenu(bool enable)
{
	if (m_hasSubMenu == enable)
		return;

	m_hasSubMenu = enable;

	// 꺾쇠와 단축키 텍스트가 같은 자리를 쓰므로 레이아웃을 다시 잡는다.
	m_isLayoutDirty = true;
}

bool UIContextMenuButton::HasSubMenu() const
{
	return m_hasSubMenu;
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
