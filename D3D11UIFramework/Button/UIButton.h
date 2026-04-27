#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "../Label/UILabel.h"
#include "../../../Module/D3D11Engine/util/SmoothValue.h"


struct ID2D1SolidColorBrush;
class UIEventDispatcher;
class IRenderContext;
class UISVGResource;
class FontManager;

enum class UICommand
{
	None,
	ZoomIn,
	ZoomOut,
	Zoom1to1,
	ZoomFit,
	ImageCenterCrossLine
};

class D3D11_UI_FRAMEWORK_INTERFACE_API UIButton : public UILabel
{
public:
	UIButton() = default;
	virtual ~UIButton();

public:
	// IRenderLayer Override
	bool Initialize(IRenderContext* context) override;
	void Shutdown() override;
	bool Update(float dt) override;
	bool Render() override;
	void DiscardDeviceResources() override;
	bool RestoreDeviceResources(IRenderContext* context) override;

	// IUIRenderLayer Override
	void OnMouseEvent(UIMouseEventType type, float x, float y) override;

	// UIElementBase Override
	void OnStateChanged(UIElementState oldState, UIElementState newState) override;
	void OnLayoutChanged() override;

	virtual void OnClick();

public:
	void SetCommand(UICommand command);
	void SetEventDispatcher(UIEventDispatcher* dispatcher);

	void SetIcon(const wchar_t* path);
	void SetIconScale(float scale);
	void SetIconStyle(const UIStyle& style);

	UIStyle& GetIconStyle();
	const UIStyle& GetIconStyle() const;

protected:
	void EnsureIconLoaded();

protected:
	// mouse event
	UICommand m_command = UICommand::None;
	UIEventDispatcher* m_dispatcher = nullptr;

	// animation rendering
	SmoothColor m_smoothIconColor = {};
	SmoothValue m_smoothIconScale = {};

	// icon
	UIStyle m_iconStyle = {};
	wchar_t* m_iconPath = nullptr;
	bool m_iconLoaded = false;
	UISVGResource* m_icon = nullptr;
	float m_iconScale = 0.6f;
};

