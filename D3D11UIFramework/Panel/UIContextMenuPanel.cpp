#include "pch.h"
#include "UIContextMenuPanel.h"

#include "../Button/UIContextMenuButton.h"

#include "../../../Module/D3D11EngineInterface/IRenderContext.h"

UIContextMenuPanel::UIContextMenuPanel()
{

}

UIContextMenuPanel::~UIContextMenuPanel()
{
	Shutdown();
}

bool UIContextMenuPanel::AcquireDeviceResources(IRenderContext* context, bool reset)
{
	if (!UIPanel::AcquireDeviceResources(context, reset))
		return false;

	// 컨텍스트 메뉴는 닫힌 채로 시작한다. 디바이스 로스트 복구 때는
	// 열려 있었는지 여부를 그대로 유지해야 하므로 건드리지 않는다.
	if (reset)
	{
		SetVisible(false);
	}

	if (!CreateShadowResources())
		return false;

	// ★ 하위 메뉴는 m_children 이 아니므로 UIPanel 이 챙겨주지 않는다.
	//
	// 여기서 빠뜨리면 디바이스 로스트 뒤에 하위 메뉴만 브러시가 없어
	// 아무것도 안 그려진다. 최초 Initialize 때도 마찬가지다.
	for (auto& entry : m_subMenus)
	{
		if (!entry.panel)
			continue;

		const bool ok = reset
			? entry.panel->Initialize(context)
			: entry.panel->RestoreDeviceResources(context);

		if (!ok)
			return false;

		if (reset)
		{
			entry.panel->SetVisible(false);
		}
	}

	m_shadowDirty = true;
	return true;
}


void UIContextMenuPanel::DiscardDeviceResources()
{
	// 열려 있던 하위 메뉴는 디바이스가 날아가면 그릴 수 없다. 닫고 시작한다.
	CloseSubMenu();
	CancelSubMenuTimers();

	for (auto& entry : m_subMenus)
	{
		if (entry.panel)
		{
			entry.panel->DiscardDeviceResources();
		}
	}

	ReleaseShadowResources();
	UIPanel::DiscardDeviceResources();
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
	// 하위 메뉴를 먼저 내린다. 이 패널이 마지막 소유자이므로
	// m_subMenus 를 비우는 순간 하위 패널이 파괴된다.
	CloseSubMenu();
	CancelSubMenuTimers();

	for (auto& entry : m_subMenus)
	{
		if (entry.panel)
		{
			entry.panel->Shutdown();
		}
	}
	m_subMenus.clear();

	ReleaseShadowResources();

	UIPanel::Shutdown();
}

void UIContextMenuPanel::ReleaseShadowResources()
{
	SafeRelease(m_shadowEffect);
	SafeRelease(m_shadowMask);
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

	// 열린 하위 메뉴도 자기 그림자 마스크를 만들어야 한다.
	// 호스트는 루트 패널의 Prepare 만 부르므로 여기서 이어준다.
	if (m_openSubMenu)
	{
		m_openSubMenu->Prepare();
	}

	return true;
}

bool UIContextMenuPanel::Render()
{
	if (!IsVisible())
		return false;

	// 그림자 등 렌더
	RenderShadow();

	UIPanel::Render();

	// ★ 하위 메뉴는 자기 자신을 다 그린 **뒤에** 그린다.
	//
	// m_children 이 아니므로 UIPanel::Render 가 건드리지 않는다.
	// 여기서 마지막에 그려야 부모 위로 겹쳐 올라간다.
	// 더 깊은 단계는 하위 메뉴가 같은 방식으로 재귀한다.
	if (m_openSubMenu)
	{
		m_openSubMenu->Render();
	}

	return true;
}

// hover 로 하위 메뉴를 여닫는 타이머를 여기서 굴린다.
//
// ★ 대기 중에는 반드시 true 를 돌려줘야 한다.
//
// 렌더 스레드는 "애니메이션 중" 이 아니고 렌더 요청도 없으면
// SleepConditionVariableSRW(INFINITE) 로 잠든다. 마우스를 항목 위에 가만히
// 두면 hover 페이드가 ~100ms 만에 끝나므로, 여기서 true 를 돌려주지 않으면
// 그 시점에 스레드가 자 버리고 dt 가 끊겨 0.4초에 영영 도달하지 못한다.
bool UIContextMenuPanel::Update(float dt)
{
	bool busy = UIPanel::Update(dt);

	if (m_pendingOpenOwner)
	{
		m_openElapsed += dt;

		if (m_openElapsed >= m_openDelay)
		{
			OpenSubMenuFor(m_pendingOpenOwner);
			m_pendingOpenOwner = nullptr;
			m_openElapsed = 0.0f;
		}
		else
		{
			busy = true;
		}
	}

	if (m_closePending)
	{
		m_closeElapsed += dt;

		if (m_closeElapsed >= m_closeDelay)
		{
			CloseSubMenu();
		}
		else
		{
			busy = true;
		}
	}

	// 열린 하위 메뉴의 애니메이션과 그쪽 타이머도 이어서 굴린다.
	if (m_openSubMenu && m_openSubMenu->Update(dt))
	{
		busy = true;
	}

	return busy;
}

// 컨텍스트 메뉴만의 여닫기 규칙을 먼저 처리하고 나머지는 패널에 맡긴다.
//
// 예전에는 이 로직이 OnMouseEvent 와 HandleMouseEvent 로 쪼개져 있었다.
// 전자는 좌클릭/더블클릭 바깥 닫기를, 후자는 우클릭 여닫기를 담당해서
// "메뉴가 언제 닫히는가" 를 알려면 두 곳을 다 봐야 했다.
bool UIContextMenuPanel::OnMouseEvent(UIMouseEventType type, float x, float y)
{
	// ★ 열린 하위 메뉴가 언제나 먼저다.
	//
	// 하위 메뉴 안의 좌표는 이 패널의 사각형 **밖**이다. 순서를 바꾸면
	// 아래 HitTest 에 걸려 "메뉴 밖" 으로 판정되고 체인이 닫혀 버린다.
	// 여기서 소비되면 아래 NotifyChildrenLeave 를 타지 않으므로,
	// 부모 항목의 하이라이트가 유지되는 것도 이 순서 덕분이다.
	if (RouteToOpenSubMenu(type, x, y))
	{
		return true;
	}

	switch (type)
	{
	case UIMouseEventType::RButtonDown:
		Hide();
		return true;

	case UIMouseEventType::RButtonUp:
		// 새로 여는 것이므로 이전 체인은 리셋된 상태로 시작한다.
		Show(x, y);
		return true;

	case UIMouseEventType::LButtonDown:
	case UIMouseEventType::LButtonDoubleDown:
		// 메뉴 밖을 누르면 닫는다. 다만 그 클릭 자체는 소비하지 않는다 —
		// 호스트가 이미지 클릭으로 이어서 처리해야 한다.
		if (!HitTest(x, y))
		{
			// ★ 하위 메뉴는 스스로 닫지 않는다.
			//
			// "내 밖" 과 "체인 밖" 은 다르다. 부모 항목을 누른 것일 수도
			// 있는데, 그건 메뉴를 닫을 일이 아니다. 체인 전체를 볼 수
			// 있는 것은 루트뿐이므로 판단을 루트에 넘긴다.
			if (!m_isSubMenu && IsVisible())
			{
				Hide();
			}

			return false;
		}
		break;

	default:
		break;
	}

	// 이 패널 밖이면 hover 추적을 접는다. 실제 정리는 UIPanel 이 한다.
	if (type != UIMouseEventType::Leave && !HitTest(x, y))
	{
		m_pendingOpenOwner = nullptr;
		m_openElapsed = 0.0f;
	}

	UIElementBase* hit = nullptr;

	if (IsVisible() && HitTest(x, y))
	{
		hit = HitTestRecursive(x, y);
	}

	if (type == UIMouseEventType::Move)
	{
		TrackSubMenuHover(hit);
	}

	const bool consumed = UIPanel::OnMouseEvent(type, x, y);

	if (type == UIMouseEventType::LButtonUp && hit)
	{
		auto* item = dynamic_cast<UIContextMenuButton*>(hit);

		if (item && item->HasSubMenu())
		{
			// 클릭으로 즉시 열기. 타이머를 기다리지 않는다.
			//
			// 항목 자신은 자기 하위 메뉴를 모르므로(순환 참조 회피)
			// 커맨드를 내지 않고 빠지기만 한다. 실제로 여는 것은 여기다.
			m_pendingOpenOwner = nullptr;
			m_openElapsed = 0.0f;

			// 이미 열려 있으면 그대로 둔다(윈도우 탐색기와 같은 동작).
			if (m_openSubMenuOwner != item)
			{
				OpenSubMenuFor(item);
			}
		}
		else if (item && m_isSubMenu)
		{
			// 하위 메뉴에서 잎 항목을 골랐다. 커맨드는 방금
			// UIButton::OnActivated 가 냈고, 이제 메뉴가 닫혀야 한다.
			//
			// ★ 루트 메뉴는 이렇게 하지 않는다. 루트 항목(Zoom In 등)은
			//   연달아 누르는 동작이라 열린 채로 두는 것이 기존 동작이다.
			//   하위 메뉴는 "여럿 중 하나를 고르는" 자리라 성격이 다르다.
			//   위로의 전파는 부모가 RouteToOpenSubMenu 에서 처리한다.
			Hide();
		}
	}

	return consumed;
}

// 열린 하위 메뉴에 이벤트를 먼저 넘긴다.
//
// 하위 메뉴가 자기 사각형 안이면 true 를 돌려주므로(UIPanel 규칙),
// 그 값이 곧 "체인이 이 이벤트를 먹었는가" 가 된다.
bool UIContextMenuPanel::RouteToOpenSubMenu(UIMouseEventType type, float x, float y)
{
	if (!m_openSubMenu)
		return false;

	switch (type)
	{
	case UIMouseEventType::RButtonDown:
	case UIMouseEventType::RButtonUp:
		// 우클릭은 "메뉴를 새로 연다" 는 뜻이다. 아래로 내려보내면 하위
		// 메뉴가 커서 자리에서 Show 를 해 버린다. 루트가 직접 처리한다.
		return false;

	case UIMouseEventType::Leave:
		// 좌표와 무관한 정리 통지. 흘려보내되 소비하지는 않는다.
		m_openSubMenu->OnMouseEvent(type, x, y);
		return false;

	default:
		break;
	}

	// 하위 메뉴 안으로 들어왔으면 예약된 닫기를 취소한다.
	// (부모 항목 → 하위 메뉴 대각선 이동이 여기서 구제된다)
	if (m_openSubMenu->HitTest(x, y))
	{
		m_closePending = false;
		m_closeElapsed = 0.0f;
	}

	const bool consumed = m_openSubMenu->OnMouseEvent(type, x, y);

	// 하위 메뉴가 잎 선택으로 스스로 닫혔다면 체인 전체를 닫는다.
	// 내가 또 누군가의 하위 메뉴라면 Hide() 가 위로 계속 전파된다.
	if (m_openSubMenu && !m_openSubMenu->IsVisible())
	{
		CloseSubMenu();
		Hide();
	}

	return consumed;
}

// hover 로 여는 조건을 추적한다. Move 에서만 부른다.
void UIContextMenuPanel::TrackSubMenuHover(UIElementBase* hit)
{
	auto* item = dynamic_cast<UIContextMenuButton*>(hit);
	const bool overSubMenuItem = (item && item->HasSubMenu());

	if (overSubMenuItem)
	{
		// 이미 열려 있는 그 항목 위라면 아무것도 하지 않는다.
		if (m_openSubMenuOwner == item)
		{
			m_closePending = false;
			m_closeElapsed = 0.0f;
			m_pendingOpenOwner = nullptr;
			m_openElapsed = 0.0f;
			return;
		}

		if (m_pendingOpenOwner != item)
		{
			m_pendingOpenOwner = item;
			m_openElapsed = 0.0f;
		}

		// 다른 하위 메뉴가 열려 있으면 곧 닫아야 한다.
		if (m_openSubMenu)
		{
			m_closePending = true;
		}

		return;
	}

	// 하위 메뉴가 없는 자리로 옮겼다. 열림 대기는 즉시 접는다.
	m_pendingOpenOwner = nullptr;
	m_openElapsed = 0.0f;

	// 열려 있던 하위 메뉴는 바로 닫지 않고 유예를 둔다.
	//
	// 부모 항목에서 하위 메뉴로 대각선으로 이동하면 그 아래 항목 위를
	// 스쳐 지나간다. 즉시 닫으면 목적지에 닿기 전에 사라진다.
	if (m_openSubMenu && !m_closePending)
	{
		m_closePending = true;
		m_closeElapsed = 0.0f;
	}
}

void UIContextMenuPanel::OnLayoutChanged()
{
	m_shadowDirty = true;

	UIPanel::OnLayoutChanged();
}


float UIContextMenuPanel::CalculateContentHeight()
{
	float total = GetPadding() * 2;
	for (auto& child : m_children)
	{
		if (child->IsVisible())
		{
			total += child->GetLayout().Height();
		}
	}

	const size_t childCount = m_children.size();
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
	// 새로 여는 것이므로 이전 체인은 남기지 않는다.
	CloseSubMenu();
	CancelSubMenuTimers();

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

	ApplyMenuLayout(posX, posY, menuHeight);
}

// 부모 항목의 오른쪽에 붙여서 연다.
//
//   ┌──────────────┐
//   │ Save image ›│╔═══════════════╗   ← m_subMenuOverlap 만큼 겹친다
//   │ Center line  │║ Save to PNG   ║
//   └──────────────┘║ Save to JPEG  ║
//                   ╚═══════════════╝
//
// 겹치는 이유: 부모 항목과 하위 메뉴 사이에 빈틈이 있으면 그 위를 지나는
// 순간 hover 가 풀려 메뉴가 닫힌다.
void UIContextMenuPanel::ShowAsSubMenu(const Rect2f& ownerItemRect)
{
	CloseSubMenu();
	CancelSubMenuTimers();

	SetVisible(true);

	const float menuHeight = CalculateContentHeight();

	const float viewWidth = m_context ? static_cast<float>(m_context->GetWidth()) : 0.0f;
	const float viewHeight = m_context ? static_cast<float>(m_context->GetHeight()) : 0.0f;

	// 기본은 오른쪽. 첫 항목이 부모 항목과 같은 높이에 오도록 padding 만큼 올린다.
	float posX = ownerItemRect.right - m_subMenuOverlap;
	float posY = ownerItemRect.top - GetPadding();

	// 오른쪽 공간이 없으면 왼쪽으로 뒤집는다.
	if (viewWidth > 0.0f && posX + m_menuWidth > viewWidth)
	{
		posX = ownerItemRect.left - m_menuWidth + m_subMenuOverlap;
	}

	// 아래가 모자라면 위로 밀되 화면 위로는 넘기지 않는다.
	if (viewHeight > 0.0f && posY + menuHeight > viewHeight)
	{
		posY = viewHeight - menuHeight;
	}

	if (posX < 0.0f) posX = 0.0f;
	if (posY < 0.0f) posY = 0.0f;

	ApplyMenuLayout(posX, posY, menuHeight);
}

void UIContextMenuPanel::ApplyMenuLayout(float posX, float posY, float menuHeight)
{
	SetLayout({ posX, posY, posX + m_menuWidth, posY + menuHeight });
	UpdateChildLayout();

	m_shadowDirty = true;
}

void UIContextMenuPanel::Hide()
{
	// 하위 메뉴부터 닫는다. 재귀적으로 체인 끝까지 내려간다.
	CloseSubMenu();
	CancelSubMenuTimers();

	SetVisible(false);
}

// ────────────────────────────────────────────────────────────────────
// 하위 메뉴
// ────────────────────────────────────────────────────────────────────

void UIContextMenuPanel::AttachSubMenu(UIContextMenuButton* owner,
	std::shared_ptr<UIContextMenuPanel> subMenu)
{
	if (!owner || !subMenu)
		return;

	// 같은 항목에 두 번 붙이면 뒤엣것으로 바꾼다.
	for (auto& entry : m_subMenus)
	{
		if (entry.owner == owner)
		{
			if (m_openSubMenu == entry.panel.get())
			{
				CloseSubMenu();
			}

			entry.panel = subMenu;
			subMenu->m_isSubMenu = true;
			subMenu->SetVisible(false);
			owner->SetHasSubMenu(true);
			return;
		}
	}

	subMenu->m_isSubMenu = true;
	subMenu->SetVisible(false);

	// 항목은 하위 메뉴가 무엇인지는 모르고 "있다" 는 것만 안다.
	// 꺾쇠를 그리고 클릭 시 커맨드를 내지 않는 데 그것으로 충분하다.
	owner->SetHasSubMenu(true);

	// ★ Initialize 이전이든 이후든 붙일 수 있어야 한다.
	//
	// 이미 초기화된 뒤라면 여기서 바로 초기화해 준다. 그러지 않으면
	// AcquireDeviceResources 를 다시 탈 일이 없어서(디바이스 로스트 전까지)
	// 하위 메뉴가 조용히 안 그려진다. 예전에 ROI 이벤트 핸들러를
	// Initialize 전에 등록하면 유실되던 것과 같은 함정이다.
	if (m_context)
	{
		subMenu->Initialize(m_context);
		subMenu->SetVisible(false);
	}

	m_subMenus.push_back({ owner, std::move(subMenu) });
}

UIContextMenuPanel* UIContextMenuPanel::FindSubMenu(const UIElementBase* owner) const
{
	if (!owner)
		return nullptr;

	for (const auto& entry : m_subMenus)
	{
		if (entry.owner == owner)
			return entry.panel.get();
	}

	return nullptr;
}

void UIContextMenuPanel::OpenSubMenuFor(UIContextMenuButton* owner)
{
	UIContextMenuPanel* subMenu = FindSubMenu(owner);

	if (!subMenu)
		return;

	if (m_openSubMenu == subMenu)
		return;

	// 형제 하위 메뉴는 한 번에 하나만 열린다.
	CloseSubMenu();

	m_openSubMenu = subMenu;
	m_openSubMenuOwner = owner;

	m_closePending = false;
	m_closeElapsed = 0.0f;

	subMenu->ShowAsSubMenu(owner->GetLayout());
}

void UIContextMenuPanel::CloseSubMenu()
{
	if (!m_openSubMenu)
	{
		m_openSubMenuOwner = nullptr;
		m_closePending = false;
		m_closeElapsed = 0.0f;
		return;
	}

	// 하위 메뉴의 Hide 가 그 아래 체인을 마저 닫는다.
	UIContextMenuPanel* subMenu = m_openSubMenu;

	m_openSubMenu = nullptr;
	m_openSubMenuOwner = nullptr;
	m_closePending = false;
	m_closeElapsed = 0.0f;

	subMenu->Hide();
}

void UIContextMenuPanel::CancelSubMenuTimers()
{
	m_pendingOpenOwner = nullptr;
	m_openElapsed = 0.0f;
	m_closePending = false;
	m_closeElapsed = 0.0f;
}

void UIContextMenuPanel::SetSubMenuOpenDelay(float seconds)
{
	m_openDelay = (seconds < 0.0f) ? 0.0f : seconds;
}

void UIContextMenuPanel::SetSubMenuCloseDelay(float seconds)
{
	m_closeDelay = (seconds < 0.0f) ? 0.0f : seconds;
}

bool UIContextMenuPanel::IsSubMenuOpen() const
{
	return m_openSubMenu != nullptr;
}
