#include "pch.h"
#include "UIPanel.h"


#include "../../../Module/D3D11EngineInterface/IRenderContext.h"

#include <vector>
#include <algorithm>
#include <cassert>

UIPanel::UIPanel()
{
}

UIPanel::~UIPanel()
{
	Shutdown();
	m_children.clear();
}

bool UIPanel::AcquireDeviceResources(IRenderContext* context, bool reset)
{
	if (!CreateVisualResources(context, reset))
		return false;

	// 자식도 같은 경로를 탄다. Initialize / RestoreDeviceResources 는 둘 다
	// UIElementBase 에서 AcquireDeviceResources 로 모이므로 아래 두 갈래는
	// 결국 자식의 같은 함수를 reset 값만 바꿔 부르는 것과 같다.
	if (!(reset ? InitializeChildren(context) : RestoreChildDeviceResources(context)))
		return false;

	// 최초에는 자식 위치가 아직 없으므로 배치부터 계산한다(그 안에서 그리기
	// 캐시도 갱신된다). 복구 때는 배치가 그대로이므로 캐시만 다시 만든다.
	if (reset)
	{
		UpdateChildLayout();
	}
	else
	{
		UpdateDrawRectCache();
	}

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

bool UIPanel::Update(float dt)
{
	bool busy = false;

	if (!IsVisible())
		return busy;

	for (const auto& child : m_children)
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

	for (auto& child : m_children)
	{
		if (child && child->IsVisible())
		{
			child->Render();
		}
	}

	return true;
}

// 패널의 단 하나뿐인 마우스 진입점.
//
// 예전에는 OnMouseEvent(인터페이스, void) 와 HandleMouseEvent(자체, bool) 가
// 따로 있었다. 실제로 쓰인 건 후자뿐이고 전자는 hover 추적을 하지 않아서,
// 잘못 부르면 하이라이트가 고착됐다. 인터페이스가 bool 을 돌려주게 되면서
// 둘을 합칠 수 있었다.
bool UIPanel::OnMouseEvent(UIMouseEventType type, float x, float y)
{
	if (!IsVisible())
	{
		return false;
	}

	// Leave 는 좌표와 무관하게 자식 정리 통지다. 히트 테스트보다 먼저 본다.
	if (type == UIMouseEventType::Leave)
	{
		NotifyChildrenLeave(x, y);
		return false;
	}

	// 패널 밖이면 안에 있던 hover 를 풀고 넘긴다.
	if (!HitTest(x, y))
	{
		NotifyChildrenLeave(x, y);
		return false;
	}

	// 툴바 위 더블클릭이 이미지 줌으로 새지 않게 흡수한다.
	if (type == UIMouseEventType::LButtonDoubleDown)
	{
		return true;
	}

	UIElementBase* hit = HitTestRecursive(x, y);

	if (!hit)
	{
		// 패널 안이지만 자식이 없는 자리. 뒤로 흘리지 않고 패널이 먹는다.
		NotifyChildrenLeave(x, y);
		return true;
	}

	if (type == UIMouseEventType::Move)
	{
		// 이전 hover 자식을 먼저 풀어야 하이라이트가 겹치지 않는다.
		if (m_hoveredChild && m_hoveredChild != hit)
		{
			m_hoveredChild->OnMouseEvent(UIMouseEventType::Leave, x, y);
		}

		m_hoveredChild = hit;
	}

	hit->OnMouseEvent(type, x, y);

	// 자식이 처리했든 아니든 패널 영역 안에서 일어난 일이다.
	// 여기서 자식의 반환값을 그대로 넘기면, 버튼 위 LButtonUp 인데
	// Pressed 가 아니었던 경우 등이 호스트로 새어 나간다.
	return true;
}

void UIPanel::OnLayoutChanged()
{
	UpdateDrawRectCache();
}

void UIPanel::AddChild(std::shared_ptr<UIElementBase> child)
{
	if (!child)
		return;

	m_children.push_back(child);

	if (m_context)
	{
		child->Initialize(m_context);
	}

	UpdateChildLayout();
}

void UIPanel::RemoveChild(std::shared_ptr<UIElementBase> child)
{
	auto& children = m_children;

	if (m_hoveredChild == child.get())
		m_hoveredChild = nullptr;

	children.erase(std::remove(children.begin(), children.end(), child), children.end());

	UpdateChildLayout();
}

void UIPanel::ClearChildren()
{
	m_children.clear();
	m_hoveredChild = nullptr;

	UpdateChildLayout();
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
	return static_cast<int32_t>(m_children.size());
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
	for (auto it = m_children.rbegin();
		it != m_children.rend();
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

	for (auto& child : m_children)
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

	for (auto& child : m_children)
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

bool UIPanel::InitializeChildren(IRenderContext* context)
{
	for (auto& child : m_children)
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
	for (auto& child : m_children)
	{
		if (child)
		{
			child->Shutdown();
		}
	}
}

void UIPanel::DiscardChildDeviceResources()
{
	for (auto& child : m_children)
	{
		if (child)
		{
			child->DiscardDeviceResources();
		}
	}
}

bool UIPanel::RestoreChildDeviceResources(IRenderContext* context)
{
	for (auto& child : m_children)
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
	// ★ GetState() != Normal 로 거르면 안 된다.
	//
	// 패널은 자기 상태를 갖지 않아 항상 Normal 이다. 그래서 중첩 패널을
	// 건너뛰게 되고, 그 안의 손자는 Leave 를 영영 못 받아 hover 가 고착됐다.
	// 지금은 무조건 내려보내고, 자식이 패널이면 스스로 재귀한다
	// (UIPanel::OnMouseEvent 의 Leave 분기). SetState 는 값이 같으면
	// 아무것도 하지 않으므로 이미 Normal 인 자식에게는 공짜다.
	for (auto& child : m_children)
	{
		if (child)
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
