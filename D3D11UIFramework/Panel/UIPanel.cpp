#include "pch.h"
#include "UIPanel.h"
#include "UIPanelImpl.h"

#include "../../../Module/D3D11EngineInterface/IRenderContext.h"

#include <vector>
#include <algorithm>
#include <cassert>

using namespace RYLYNN;

UIPanel::UIPanel()
	: m_impl(nullptr)
{
	EnsureImpl();
}

UIPanel::~UIPanel()
{
	Shutdown();

	if (m_impl)
	{
		m_impl->m_children.clear();
		delete m_impl;
		m_impl = nullptr;
	}
}

bool UIPanel::Initialize(IRenderContext* context)
{
	if (!EnsureImpl())
		return false;

	if (!CreateVisualResources(context, true))
		return false;

	if (!InitializeChildren(context))
		return false;

	UpdateChildLayout();

	return true;
}

void UIPanel::Shutdown()
{
	ShutdownChildren();
	m_hoveredChild = nullptr;
	ReleaseVisualResources();

	UIElementBase::Shutdown();
}

void UIPanel::DiscardDeviceResources()
{
	DiscardChildDeviceResources();
	m_hoveredChild = nullptr;
	ReleaseVisualResources();
}

bool UIPanel::RestoreDeviceResources(IRenderContext* context)
{
	if (!EnsureImpl())
		return false;

	if (!CreateVisualResources(context, false))
		return false;

	if (!RestoreChildDeviceResources(context))
		return false;

	UpdateDrawRectCache();
	return true;
}

bool UIPanel::Update(float dt)
{
	bool busy = false;

	if (!IsVisible())
		return busy;

	for (const auto& child : m_impl->m_children)
	{
		if (child->Update(dt))
		{
			busy = true;
		}
	}

	return busy;
}

bool UIPanel::Render()
{
	ID2D1DeviceContext* d2dContext = GetDeviceContext();
	if (!d2dContext)
		return false;

	DrawBackground(d2dContext);

	for (auto& child : m_impl->m_children)
	{
		if (child && child->IsVisible())
		{
			child->Render();
		}
	}

	return true;
}

void UIPanel::OnMouseEvent(UIMouseEventType type, float x, float y)
{
	if (!IsVisible())
		return;

	if (UIElementBase* hit = HitTestRecursive(x, y))
		hit->OnMouseEvent(type, x, y);
}

void UIPanel::OnLayoutChanged()
{
	UpdateDrawRectCache();
}

void UIPanel::AddChild(std::shared_ptr<UIElementBase> child)
{
	if (!child)
		return;

	if (!EnsureImpl())
		return;

	m_impl->m_children.push_back(child);

	if (m_context)
	{
		child->Initialize(m_context);
	}

	UpdateChildLayout();
}

void UIPanel::RemoveChild(std::shared_ptr<UIElementBase> child)
{
	auto& children = m_impl->m_children;

	if (m_hoveredChild == child.get())
		m_hoveredChild = nullptr;

	children.erase(std::remove(children.begin(), children.end(), child), children.end());

	UpdateChildLayout();
}

void UIPanel::ClearChildren()
{
	m_impl->m_children.clear();
	m_hoveredChild = nullptr;

	UpdateChildLayout();
}

bool UIPanel::HandleMouseEvent(UIMouseEventType type, float x, float y)
{
	if (!IsVisible())
		return false;

	if (!HitTest(x, y))
	{
		NotifyChildrenLeave(x, y);
		return false;
	}

	if (type == UIMouseEventType::LButtonDoubleDown)
	{
		// Toolbar 위에서 더블클릭 시 이미지 Zoom In/Out 관련
		// 어떠한 행위도 수행 하지 않도록 true 반환
		return true;
	}

	UIElementBase* hit = HitTestRecursive(x, y);

	if (!hit)
	{
		NotifyChildrenLeave(x, y);
		return true;
	}


	switch (type)
	{
	case UIMouseEventType::Move:
	{
		// 1️⃣ 이전 hover → Leave
		if (m_hoveredChild && m_hoveredChild != hit)
		{
			m_hoveredChild->OnMouseEvent(UIMouseEventType::Leave, x, y);
		}

		// 2️⃣ 현재 hit → Move
		if (hit)
		{
			hit->OnMouseEvent(UIMouseEventType::Move, x, y);
		}

		m_hoveredChild = hit;
		return hit != nullptr;
	}

	case UIMouseEventType::LButtonDown:
	case UIMouseEventType::LButtonUp:
	{
		if (hit)
		{
			hit->OnMouseEvent(type, x, y);
			return true;
		}
		return false;
	}

	case UIMouseEventType::Leave:
	{
		if (m_hoveredChild)
		{
			m_hoveredChild->OnMouseEvent(UIMouseEventType::Leave, x, y);
			m_hoveredChild = nullptr;
			return true;
		}
		return false;
	}

	default:
		break;
	}

	return false;
}

void UIPanel::Resize(float width, float height)
{
	const Rect2f& layout = GetLayout();
	const UILayoutType layoutType = GetLayoutType();

	if (layoutType == UILayoutType::Vertical)
	{
		const float panelWidth = layout.Width();
		const Rect2f resizeLayout = {
			layout.left,
			layout.top,
			layout.left + panelWidth,
			static_cast<float>(height)
		};

		SetLayout(resizeLayout);
		UpdateChildLayout();
	}
	else if (layoutType == UILayoutType::Horizontal)
	{
		const float panelHeight = layout.Height();
		const Rect2f resizeLayout = {
			layout.left,
			static_cast<float>(height) - panelHeight,
			static_cast<float>(width),
			static_cast<float>(height)
		};

		SetLayout(resizeLayout);
		UpdateChildLayout();
	}
}

void UIPanel::SetLayoutType(UILayoutType type)
{
	if (m_layoutType == type)
		return;

	m_layoutType = type;
	UpdateChildLayout();
}

UILayoutType UIPanel::GetLayoutType() const
{
	return m_layoutType;
}

void UIPanel::SetPadding(float padding)
{
	if (m_padding == padding)
		return;

	m_padding = padding;
	UpdateChildLayout();
}

float UIPanel::GetPadding() const
{
	return m_padding;
}

void UIPanel::SetSpacing(float spacing)
{
	if (m_spacing == spacing)
		return;

	m_spacing = spacing;
	UpdateChildLayout();
}

float UIPanel::GetSpacing() const
{
	return m_spacing;
}

void UIPanel::SetRounded(bool enable)
{
	if (m_isRoundedRect == enable)
		return;

	m_isRoundedRect = enable;
	UpdateDrawRectCache();
}

void UIPanel::SetCornerRadius(float radius)
{
	if (m_cornerRadius == radius)
		return;

	m_cornerRadius = radius;
	UpdateDrawRectCache();
}

void UIPanel::UpdateChildLayout()
{
	if (LayoutData().Empty())
		return;

	switch (m_layoutType)
	{
	case UILayoutType::Vertical:
		UpdateVerticalLayout();
		break;

	case UILayoutType::Horizontal:
		UpdateHorizontalLayout();
		break;

	default:
		break;
	}
}

int32_t UIPanel::GetChildItemCount() const
{
	if (!m_impl)
		return 0;

	return static_cast<int32_t>(m_impl->m_children.size());
}

void UIPanel::UpdateDrawRectCache()
{
	const Rect2f& layout = LayoutData();

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

UIElementBase* UIPanel::HitTestRecursive(float x, float y)
{
	for (auto it = m_impl->m_children.rbegin();
		it != m_impl->m_children.rend();
		++it)
	{
		auto& child = *it;
		if (!child || !child->IsVisible())
			continue;

		if (auto* panel = dynamic_cast<UIPanel*>(child.get()))
		{
			if (auto* hit = panel->HitTestRecursive(x, y))
				return hit;
		}

		if (child->HitTest(x, y))
			return child.get();
	}

	return nullptr;
}

void UIPanel::UpdateVerticalLayout()
{
	if (m_layoutType != UILayoutType::Vertical)
		return;

	const Rect2f& layout = LayoutData();

	if (layout.Empty())
		return;

	float x = layout.left + m_padding;
	float y = layout.top + m_padding;

	OnLayoutChanged();

	float contentWidth = layout.Width() - m_padding * 2;

	for (auto& child : m_impl->m_children)
	{
		if (!child || !child->IsVisible())
			continue;

		const auto& childRect = child->GetLayout();
		float contentHeight = childRect.Height();

		Rect2f contentRect;
		contentRect.left = x;
		contentRect.right = x + contentWidth;
		contentRect.top = y;
		contentRect.bottom = y + contentHeight;

		child->SetLayout(contentRect);
		NotifyChildLayoutChanged(child.get());

		y += contentHeight + m_spacing;
	}
}

void UIPanel::UpdateHorizontalLayout()
{
	if (m_layoutType != UILayoutType::Horizontal)
		return;

	const Rect2f& layout = LayoutData();

	if (layout.Empty())
		return;

	float x = layout.left + m_padding;
	float y = layout.top + m_padding;

	OnLayoutChanged();

	float contentHeight = layout.Height() - m_padding * 2;

	for (auto& child : m_impl->m_children)
	{
		if (!child || !child->IsVisible())
			continue;

		const auto& childRect = child->GetLayout();
		float contentWidth = childRect.Width();

		Rect2f contentRect;
		contentRect.left = x;
		contentRect.right = x + contentWidth;
		contentRect.top = y;
		contentRect.bottom = y + contentHeight;

		child->SetLayout(contentRect);
		NotifyChildLayoutChanged(child.get());

		x += contentWidth + m_spacing;
	}
}

bool UIPanel::CreateVisualResources(IRenderContext* context, bool resetState)
{
	if (!BindRenderContext(context, resetState))
		return false;

	ID2D1DeviceContext* d2dContext = GetDeviceContext();
	if (!d2dContext)
		return false;

	if (FAILED(d2dContext->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1), &m_fillBrush)))
		return false;

	if (FAILED(d2dContext->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1), &m_strokeBrush)))
		return false;

	return true;
}

bool UIPanel::EnsureImpl()
{
	if (m_impl)
		return true;

	m_impl = new UIPanelImpl();

	if (!m_impl)
	{
		assert("UIPanelImpl Create Failed!");
		return false;
	}

	return true;
}

bool UIPanel::InitializeChildren(IRenderContext* context)
{
	if (!m_impl)
		return false;

	for (auto& child : m_impl->m_children)
	{
		if (child && !child->Initialize(context))
		{
			return false;
		}
	}

	return true;
}

void UIPanel::ShutdownChildren()
{
	if (!m_impl)
		return;

	for (auto& child : m_impl->m_children)
	{
		if (child)
		{
			child->Shutdown();
		}
	}
}

void UIPanel::DiscardChildDeviceResources()
{
	if (!m_impl)
		return;

	for (auto& child : m_impl->m_children)
	{
		if (child)
		{
			child->DiscardDeviceResources();
		}
	}
}

bool UIPanel::RestoreChildDeviceResources(IRenderContext* context)
{
	if (!m_impl)
		return false;

	for (auto& child : m_impl->m_children)
	{
		if (child && !child->RestoreDeviceResources(context))
		{
			return false;
		}
	}

	return true;
}

void UIPanel::ReleaseVisualResources()
{
	SafeRelease(m_strokeBrush);
	SafeRelease(m_fillBrush);
}

ID2D1DeviceContext* UIPanel::GetDeviceContext() const
{
	if (!IsVisible() || !m_context)
		return nullptr;

	return m_context->GetD2DDeviceContext();
}

void UIPanel::DrawBackground(ID2D1DeviceContext* d2dContext)
{
	if (!d2dContext || !m_fillBrush || !m_strokeBrush)
		return;

	m_fillBrush->SetColor(m_style.normal.fill);
	m_strokeBrush->SetColor(m_style.normal.border);

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

void UIPanel::NotifyChildrenLeave(float x, float y)
{
	for (auto& child : m_impl->m_children)
	{
		if (child && child->GetState() != UIElementState::Normal)
		{
			child->OnMouseEvent(UIMouseEventType::Leave, x, y);
		}
	}

	m_hoveredChild = nullptr;
}

void UIPanel::NotifyChildLayoutChanged(UIElementBase* child)
{
	if (child)
	{
		child->OnLayoutChanged();
	}
}
