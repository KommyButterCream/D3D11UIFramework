#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "UIPanel.h"

struct ID2D1Bitmap1;
struct ID2D1Effect;

class UIContextMenuButton;

// C4251: m_subMenus 가 std::vector 라서 나는 경고. UIPanel::m_children 과
// 같은 이유로 안전하다 — 이 벡터는 DLL 안에서만 만들어지고 순회되며,
// 밖으로 열린 접근자는 AttachSubMenu / IsSubMenuOpen 뿐이다.
#pragma warning(push)
#pragma warning(disable : 4251)

class D3D11_UI_FRAMEWORK_INTERFACE_API UIContextMenuPanel final : public UIPanel
{
public:
	UIContextMenuPanel();
	virtual ~UIContextMenuPanel();

	// IRenderLayer Override
	void Shutdown() override;
	bool Prepare() override;
	bool Render() override;
	void DiscardDeviceResources() override;

	// UIElementBase Override
	bool Update(float dt) override;
	bool OnMouseEvent(UIMouseEventType type, float x, float y) override;
	void OnLayoutChanged() override;

	// UIPanel Override

protected:
	// UIElementBase Override
	bool AcquireDeviceResources(IRenderContext* context, bool reset) override;

private:
	bool CreateShadowResources();
	void ReleaseShadowResources();
	float CalculateContentHeight();
	bool CreateShadowMask();
	void UpdateShadowEffect();
	void RenderShadow();

public:
	void SetMenuWidth(float width);
	void Show(float x, float y);

	// 체인 전체를 닫는다. 열려 있던 하위 메뉴도 재귀적으로 함께 닫힌다.
	void Hide();

	// ── 하위 메뉴 ────────────────────────────────────────────────────
	//
	// owner 항목에 하위 메뉴를 붙인다. owner 는 이 패널의 자식이어야 한다.
	//
	// subMenu 는 이 패널의 자식이 **아니다**. 자식으로 넣으면
	// UpdateVerticalLayout 이 패널 안쪽에 줄을 세우고,
	// CalculateContentHeight 가 부모 높이에 더해 버린다. 하위 메뉴는
	// 패널 바깥 오른쪽에 떠서 부모보다 **나중에** 그려져야 한다.
	void AttachSubMenu(UIContextMenuButton* owner,
		std::shared_ptr<UIContextMenuPanel> subMenu);

	// 항목 사각형 오른쪽에 붙여서 연다. 오른쪽 공간이 없으면 왼쪽으로 뒤집는다.
	void ShowAsSubMenu(const Core::ShapeType::Rect2f& ownerItemRect);

	// hover 로 열리기까지의 대기 시간(초). 기본 0.4 — 윈도우 탐색기와 같다.
	void SetSubMenuOpenDelay(float seconds);

	// 다른 항목으로 옮겼을 때 닫히기까지의 유예(초). 기본 0.25.
	//
	// 0 으로 두면 부모 항목에서 하위 메뉴로 **대각선**으로 이동할 때
	// 중간에 스치는 아래 항목 때문에 목적지 도착 전에 닫혀 버린다.
	void SetSubMenuCloseDelay(float seconds);

	bool IsSubMenuOpen() const;

private:
	struct SubMenuEntry
	{
		UIContextMenuButton* owner = nullptr;
		std::shared_ptr<UIContextMenuPanel> panel;
	};

	UIContextMenuPanel* FindSubMenu(const UIElementBase* owner) const;
	void OpenSubMenuFor(UIContextMenuButton* owner);
	void CloseSubMenu();
	void CancelSubMenuTimers();

	// 열린 하위 메뉴에 이벤트를 먼저 넘긴다. 소비했으면 true.
	bool RouteToOpenSubMenu(UIMouseEventType type, float x, float y);

	// 하위 메뉴 후보 위에서의 hover 를 추적한다(Move 전용).
	void TrackSubMenuHover(UIElementBase* hit);

	// Show / ShowAsSubMenu 공통부. 배치하고 그림자를 무효화한다.
	void ApplyMenuLayout(float posX, float posY, float menuHeight);

private:
	float m_menuWidth = 200.0f;

	// ── 하위 메뉴 상태 ───────────────────────────────────────────────
	std::vector<SubMenuEntry> m_subMenus;

	// 지금 열려 있는 하위 메뉴와 그 주인. 소유권은 m_subMenus 에 있다.
	UIContextMenuPanel* m_openSubMenu = nullptr;
	UIContextMenuButton* m_openSubMenuOwner = nullptr;

	// 열림 대기 — 하위 메뉴가 있는 항목 위에 머무는 중
	UIContextMenuButton* m_pendingOpenOwner = nullptr;
	float m_openElapsed = 0.0f;
	float m_openDelay = 0.4f;

	// 닫힘 유예 — 다른 항목으로 옮겼지만 아직 기다려 주는 중
	bool m_closePending = false;
	float m_closeElapsed = 0.0f;
	float m_closeDelay = 0.25f;

	// 이 패널이 다른 메뉴의 하위 메뉴인가. AttachSubMenu 가 켜 준다.
	//
	// 잎 항목을 골랐을 때 스스로 닫힐지를 이 값으로 가른다. 루트 메뉴는
	// 항목을 눌러도 열린 채로 두는 것이 기존 동작이라 구분이 필요하다.
	bool m_isSubMenu = false;

	// 부모 항목과 하위 메뉴를 이만큼 겹친다.
	// 겹치지 않으면 그 사이 빈 공간을 밟는 순간 hover 가 풀린다.
	float m_subMenuOverlap = 4.0f;

	// Shadow
	ID2D1Bitmap1* m_shadowMask = nullptr;
	ID2D1Effect* m_shadowEffect = nullptr;
	float m_shadowBlur = 12.0f;
	float m_shadowOpacity = 0.35f;
	float m_shadowOffsetX = 6.0f;
	float m_shadowOffsetY = 6.0f;
	bool m_shadowDirty = true;
};


#pragma warning(pop)
