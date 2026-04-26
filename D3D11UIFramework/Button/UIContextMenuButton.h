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
    bool Initialize(IRenderContext* context) override;
    void Shutdown() override;
    bool Update(float dt) override;
    bool Render() override;

    // UIButton Override
    virtual void OnClick();

public:
    void SetIconAreaWidth(float width);
    void SetExtraAreaWidth(float width);
    void SetExtraText(const wchar_t* text);
    void SetCheckable(bool enable);
    bool IsChecked() const;

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
    
    IDWriteTextLayout* m_extraTextLayout = nullptr;
};

