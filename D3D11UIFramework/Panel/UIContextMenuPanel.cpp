#include "pch.h"
#include "UIContextMenuPanel.h"
#include "UIPanelImpl.h"

#include "../../../Module/D3D11EngineInterface/IRenderContext.h"

UIContextMenuPanel::UIContextMenuPanel()
{

}

UIContextMenuPanel::~UIContextMenuPanel()
{
	Shutdown();
}

bool UIContextMenuPanel::Initialize(IRenderContext* context)
{
	if (!UIPanel::Initialize(context))
		return false;

	SetVisible(false);

	return CreateShadowResources();
}

void UIContextMenuPanel::DiscardDeviceResources()
{
	ReleaseShadowResources();
	UIPanel::DiscardDeviceResources();
}

bool UIContextMenuPanel::RestoreDeviceResources(IRenderContext* context)
{
	if (!UIPanel::RestoreDeviceResources(context))
		return false;

	if (!CreateShadowResources())
		return false;

	m_shadowDirty = true;
	return true;
}

bool UIContextMenuPanel::CreateShadowResources()
{
	ID2D1DeviceContext* d2dContext = m_context->GetD2DDeviceContext();
	if (!d2dContext)
		return false;

	HRESULT hr = S_OK;

	hr = d2dContext->CreateEffect(CLSID_D2D1Shadow, &m_shadowEffect);
	if (FAILED(hr))
		return false;

	m_shadowEffect->SetValue(
		D2D1_SHADOW_PROP_COLOR,
		D2D1::Vector4F(0, 0, 0, 1)
	);

	m_shadowEffect->SetValue(
		D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION,
		m_shadowBlur
	);

	return true;
}

void UIContextMenuPanel::Shutdown()
{
	ReleaseShadowResources();

	UIPanel::Shutdown();
}

void UIContextMenuPanel::ReleaseShadowResources()
{
	SafeRelease(m_shadowEffect);
	SafeRelease(m_shadowMask);
}

bool UIContextMenuPanel::Update(float dt)
{
	if (!IsVisible())
		return false;

	return UIPanel::Update(dt);
}

bool UIContextMenuPanel::Prepare()
{
	if (m_shadowDirty)
	{
		if (CreateShadowMask())
		{
			UpdateShadowEffect();
			m_shadowDirty = false;
		}
	}

	return true;
}

bool UIContextMenuPanel::Render()
{
	if (!IsVisible())
		return false;

	// 그림자 등 렌더
	RenderShadow();

	return UIPanel::Render();
}

void UIContextMenuPanel::OnMouseEvent(UIMouseEventType type, float x, float y)
{
	if (type == UIMouseEventType::LButtonDown ||
		type == UIMouseEventType::LButtonDoubleDown)
	{
		if (!HitTest(x, y))
			Hide();
	}

	UIPanel::OnMouseEvent(type, x, y);
}

void UIContextMenuPanel::OnLayoutChanged()
{
	m_shadowDirty = true;

	UIPanel::OnLayoutChanged();
}

bool UIContextMenuPanel::HandleMouseEvent(UIMouseEventType type, float x, float y)
{
	if (type == UIMouseEventType::RButtonDown)
	{
		Hide();

		return true;
	}
	else if (type == UIMouseEventType::RButtonUp)
	{
		Show(x, y);

		return true;
	}
	else if (type == UIMouseEventType::LButtonDown)
	{
		if (!HitTest(x, y) && IsVisible())
			Hide();

		if (!HitTest(x, y))
			return false;
	}

	return UIPanel::HandleMouseEvent(type, x, y);
}

float UIContextMenuPanel::CalculateContentHeight()
{
	float total = GetPadding() * 2;
	for (auto& child : m_impl->m_children)
	{
		if (child->IsVisible())
		{
			total += child->GetLayout().Height();
		}
	}

	const size_t childCount = m_impl->m_children.size();
	if (childCount > 1)
	{
		total += static_cast<float>(childCount - 1) * GetSpacing();
	}

	return total;
}

bool UIContextMenuPanel::CreateShadowMask()
{
	const auto& layout = LayoutData();
	const float contentWidth = layout.Width();
	const float contentHeight = layout.Height();

	if (contentWidth <= 0 || contentHeight <= 0)
		return false;

	if (m_shadowMask)
	{
		D2D1_SIZE_F maskSize = m_shadowMask->GetSize();
		if (maskSize.width == contentWidth && maskSize.height == contentHeight)
		{
			return true;
		}
	}

	SafeRelease(m_shadowMask);

	ID2D1DeviceContext* d2dContext = m_context->GetD2DDeviceContext();
	if (!d2dContext)
		return false;

	D2D1_BITMAP_PROPERTIES1 bitmapProperties =
		D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0.f, 0.f);

	HRESULT hr = d2dContext->CreateBitmap(
		D2D1::SizeU(static_cast<UINT32>(contentWidth), static_cast<UINT32>(contentHeight)),
		nullptr,
		0,
		&bitmapProperties,
		&m_shadowMask
	);

	if (FAILED(hr))
		return false;

	// offscreen draw
	ID2D1Image* oldTarget = nullptr;
	d2dContext->GetTarget(&oldTarget);
	//if (oldTarget)
	//	oldTarget->AddRef();

	d2dContext->SetTarget(m_shadowMask);
	d2dContext->BeginDraw();
	d2dContext->Clear(D2D1::ColorF(0, 0, 0, 0));

	ID2D1SolidColorBrush* white = nullptr;
	hr = d2dContext->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &white);

	if (FAILED(hr))
	{
		d2dContext->SetTarget(oldTarget);
		SafeRelease(oldTarget);
		return false;
	}

	if (m_isRoundedRect)
	{
		D2D1_ROUNDED_RECT roundRect = {};
		roundRect.rect = D2D1::RectF(0, 0, contentWidth, contentHeight);
		roundRect.radiusX = m_cornerRadius;
		roundRect.radiusY = m_cornerRadius;
		d2dContext->FillRoundedRectangle(roundRect, white);
	}
	else
	{
		d2dContext->FillRectangle(D2D1::RectF(0, 0, contentWidth, contentHeight), white);
	}

	hr = d2dContext->EndDraw();
	d2dContext->SetTarget(oldTarget);

	SafeRelease(white);
	SafeRelease(oldTarget);

	return SUCCEEDED(hr);
}

void UIContextMenuPanel::UpdateShadowEffect()
{
	if (!m_shadowEffect || !m_shadowMask)
		return;

	m_shadowEffect->SetInput(0, m_shadowMask);
}

void UIContextMenuPanel::RenderShadow()
{
	if (!IsVisible() || !m_shadowEffect)
		return;

	ID2D1DeviceContext* d2dContext = m_context->GetD2DDeviceContext();
	if (!d2dContext)
		return;

	D2D1_MATRIX_3X2_F old = {};
	d2dContext->GetTransform(&old);

	d2dContext->SetTransform(
		D2D1::Matrix3x2F::Translation(
			LayoutData().left + m_shadowOffsetX,
			LayoutData().top + m_shadowOffsetY
		)
	);

	d2dContext->DrawImage(
		m_shadowEffect,
		D2D1_INTERPOLATION_MODE_LINEAR,
		D2D1_COMPOSITE_MODE_SOURCE_OVER
	);

	d2dContext->SetTransform(old);
}

void UIContextMenuPanel::SetMenuWidth(float width)
{
	m_menuWidth = width;

	m_shadowDirty = true;
}

void UIContextMenuPanel::Show(float x, float y)
{
	SetVisible(true);

	const float menuHeight = CalculateContentHeight();

	float posX = x;
	float posY = y;
	const float viewWidth = m_context ? static_cast<float>(m_context->GetWidth()) : 0.0f;
	const float viewHeight = m_context ? static_cast<float>(m_context->GetHeight()) : 0.0f;

	if (viewWidth > 0.0f && x + m_menuWidth > viewWidth)
		posX -= m_menuWidth;
	if (viewHeight > 0.0f && y + menuHeight > viewHeight)
		posY -= menuHeight;

	SetLayout({ posX, posY, posX + m_menuWidth, posY + menuHeight });
	UpdateChildLayout();

	m_shadowDirty = true;
}

void UIContextMenuPanel::Hide()
{
	SetVisible(false);
}
