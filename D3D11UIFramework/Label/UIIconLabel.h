#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "UILabel.h"
#include "../Resource/UIIcon.h"

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
	void Shutdown() override;
	bool Update(float dt) override;
	bool Render() override;
	void DiscardDeviceResources() override;

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

protected:
	// UIElementBase Override
	bool AcquireDeviceResources(IRenderContext* context, bool reset) override;

private:
	void UpdateTextLayout() override;
	UIIconLabelLayout ComputeLayout() const;

private:
	UIIconLabelLayout m_iconLabellayout = {};

	float m_iconAreaWidth = 28.0f;
	float m_textAreaWidth = 40.0f;

	// UIButton 과 같은 UIIcon 을 쓴다. 예전에는 두 클래스가 같은 멤버
	// 다섯 개를 각자 복제하고 있었다.
	UIIcon m_icon;
};

