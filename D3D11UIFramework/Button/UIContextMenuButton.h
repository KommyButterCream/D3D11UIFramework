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

    bool Update(float dt) override;
    void DiscardDeviceResources() override;
    void OnStateChanged(UIElementState oldState, UIElementState newState) override;

protected:
    bool AcquireDeviceResources(IRenderContext* context, bool reset) override;

public:
    void SetIconAreaWidth(float width);
    void SetExtraAreaWidth(float width);
    void SetExtraText(const wchar_t* text);
    void SetCheckable(bool enable);
    bool IsChecked() const;

    // 라디오처럼 쓰려면 호스트가 배타성을 관리해야 한다.
    // OnClick 의 토글과 별개로 상태를 직접 박는 경로다.
    void SetChecked(bool checked);

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

private:
    float m_iconAreaWidth = 28.0f;
    float m_extraAreaWidth = 40.0f;
    float m_extraAreaOffset = 5.0f;

    bool m_hasExtraText = false;
    wchar_t* m_extraText = nullptr;

    bool m_checkable = false;
    bool m_checked = false;

    bool m_hasSubMenu = false;

    // 하위 메뉴 꺾쇠. 내장 아이콘 경로를 그대로 탄다.
    //
    // 예전에는 이 클래스가 DrawLine 두 번으로 따로 그렸다. 같은 도형을
    // 두 군데서 그리게 되므로 UIIconRenderer 로 합쳤다.
    UIIcon m_subMenuArrow = {};

    IDWriteTextLayout* m_extraTextLayout = nullptr;
};

