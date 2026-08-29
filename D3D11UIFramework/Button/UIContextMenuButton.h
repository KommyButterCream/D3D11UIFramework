#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "UIButton.h"

struct ContextMenuLayout
{
    D2D1_RECT_F iconRect = {};
    D2D1_RECT_F textRect = {};
    D2D1_RECT_F extraRect = {};
};

class D3D11_UI_FRAMEWORK_INTERFACE_API UIContextMenuButton : public UIButton
{
public:
	UIContextMenuButton();
	virtual ~UIContextMenuButton();

public:
    // IRenderLayer Override
    void Shutdown() override;
    bool Render() override;

    // UIElementBase Override
    //
    // 하위 메뉴를 여는 항목은 커맨드를 내지 않는다. 여는 일 자체는 패널이
    // 한다 — 항목은 자기 하위 메뉴가 무엇인지 모른다.
    void OnActivated() override;

    // UIButton Override
    virtual void OnClick();

public:
    void SetIconAreaWidth(float width);
    void SetExtraAreaWidth(float width);
    void SetExtraText(const wchar_t* text);
    void SetCheckable(bool enable);
    bool IsChecked() const;

    // 이 항목이 하위 메뉴를 여는 항목인가.
    //
    // 플래그만 든다. 하위 메뉴 자체는 UIContextMenuPanel 이 소유하고
    // AttachSubMenu 가 이 값을 켜 준다. 항목이 패널 타입을 알면 순환
    // 참조가 되므로 일부러 모르게 둔다.
    void SetHasSubMenu(bool enable);
    bool HasSubMenu() const;

private:
    void UpdateTextLayout() override;
    ContextMenuLayout ComputeLayout() const;

    // extraRect 자리에 꺾쇠(›)를 직접 그린다.
    //
    // SVG 아이콘을 쓰지 않는 이유: 아이콘 자산이 저장소 밖에 있어서
    // 새로 추가하면 배포 의존이 하나 더 늘어난다. 선 두 개면 충분하고,
    // 글자와 같은 브러시를 쓰므로 hover 색 변화도 저절로 따라간다.
    void DrawSubMenuArrow(ID2D1DeviceContext* d2dContext,
                          const D2D1_RECT_F& extraRect) const;

private:
    float m_iconAreaWidth = 28.0f;
    float m_extraAreaWidth = 40.0f;
    float m_extraAreaOffset = 5.0f;

    bool m_hasExtraText = false;
    wchar_t* m_extraText = nullptr;

    bool m_checkable = false;
    bool m_checked = false;

    bool m_hasSubMenu = false;

    // 꺾쇠 크기(반높이)와 선 두께. extraRect 오른쪽에 세로 가운데 정렬한다.
    float m_arrowHalfHeight = 4.0f;
    float m_arrowThickness = 1.4f;

    IDWriteTextLayout* m_extraTextLayout = nullptr;
};

