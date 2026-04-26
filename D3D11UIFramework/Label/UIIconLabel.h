#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "UILabel.h"

class UISVGResource;

struct UIIconLabelLayout
{
	D2D1_RECT_F iconRect = {};
	D2D1_RECT_F textRect = {};
};

class D3D11_UI_FRAMEWORK_INTERFACE_API UIIconLabel : public UILabel
{
public:
	UIIconLabel();
	virtual ~UIIconLabel();

public:
	// IRenderLayer Override
	bool Initialize(IRenderContext* context) override;
	void Shutdown() override;
	bool Update(float dt) override;
	bool Render() override;
	void DiscardDeviceResources() override;
	bool RestoreDeviceResources(IRenderContext* context) override;

	// UIElementBase Override
	void OnStateChanged(UIElementState oldState, UIElementState newState) override;

public:
	void SetIconAreaWidth(float width);
	void SetTextAreaWidth(float width);

	void SetIcon(const wchar_t* path);
	void SetIconScale(float scale);
	void SetIconStyle(const UIStyle& style);

	UIStyle& GetIconStyle();
	const UIStyle& GetIconStyle() const;

private:
	void EnsureIconLoaded();

	void UpdateTextLayout() override;
	UIIconLabelLayout ComputeLayout() const;

private:
	UIIconLabelLayout m_iconLabellayout = {};

	// animation rendering
	SmoothColor m_smoothIconColor = {};
	SmoothValue m_smoothIconScale = {};

	float m_iconAreaWidth = 28.0f;
	float m_textAreaWidth = 40.0f;

	// icon
	UIStyle m_iconStyle = {};
	wchar_t* m_iconPath = nullptr;
	bool m_iconLoaded = false;
	UISVGResource* m_icon = nullptr;
	float m_iconScale = 0.6f;
};

