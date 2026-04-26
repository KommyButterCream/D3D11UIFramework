#include "pch.h"
#include "UISplitBar.h"

#include "../Resource/UIDefaultColor.h"
#include "../../Modules/D3D11EngineInterface/IRenderContext.h"
#include "../../Modules/Core/ShapeType/Point2f.h"

UISplitBar::~UISplitBar()
{
	Shutdown();
}

bool UISplitBar::Initialize(IRenderContext* context)
{
	return CreateLineBrush(context, true);
}

void UISplitBar::Shutdown()
{
	SafeRelease(m_borderBrush);
	UIElementBase::Shutdown();
}

void UISplitBar::DiscardDeviceResources()
{
	SafeRelease(m_borderBrush);
}

bool UISplitBar::RestoreDeviceResources(IRenderContext* context)
{
	return CreateLineBrush(context, false);
}

bool UISplitBar::Update(float dt)
{
	return false;
}

bool UISplitBar::Render()
{
	if (!IsVisible() || !m_context)
		return false;

	if (!m_borderBrush)
		return false;

	ID2D1DeviceContext* d2dContext = m_context->GetD2DDeviceContext();
	if (!d2dContext)
		return false;

	if (m_updateColor)
	{
		m_borderBrush->SetColor(m_borderColor);
		m_updateColor = false;
	}

	if (m_splitbarType == SplitBarType::Horizontal)
	{
		const auto& layout = LayoutData();
		const D2D1_POINT_2F begin = { layout.left + m_padding, layout.Center().y };
		const D2D1_POINT_2F end = { layout.right - m_padding, layout.Center().y };

		d2dContext->DrawLine(begin, end, m_borderBrush, m_thickness);
	}
	else if (m_splitbarType == SplitBarType::Vertical)
	{
		const auto& layout = LayoutData();
		const D2D1_POINT_2F begin = { layout.Center().x, layout.top + m_padding };
		const D2D1_POINT_2F end = { layout.Center().x, layout.bottom - m_padding };

		d2dContext->DrawLine(begin, end, m_borderBrush, m_thickness);
	}

	return true;
}

void UISplitBar::OnMouseEvent(UIMouseEventType type, float x, float y)
{
	return;
}

bool UISplitBar::HitTest(float x, float y) const
{
	return false;
}

void UISplitBar::OnLayoutChanged()
{
	return;
}

void UISplitBar::SetLineColor(const D2D1_COLOR_F& color)
{
	m_borderColor = color;
	m_updateColor = true;

	if (m_borderBrush)
	{
		m_borderBrush->SetColor(m_borderColor);
		m_updateColor = false;
	}
}

void UISplitBar::SetSplitbarType(const SplitBarType& splitbarType)
{
	m_splitbarType = splitbarType;
}

void UISplitBar::SetLineThickness(float lineThickness)
{
	m_thickness = lineThickness;
}

bool UISplitBar::CreateLineBrush(IRenderContext* context, bool resetState)
{
	if (!BindRenderContext(context, resetState))
		return false;

	ID2D1DeviceContext* d2dContext = m_context->GetD2DDeviceContext();
	if (!d2dContext)
		return false;

	SafeRelease(m_borderBrush);

	return SUCCEEDED(d2dContext->CreateSolidColorBrush(m_borderColor, &m_borderBrush));
}
