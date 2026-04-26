#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "../Base/UIElementBase.h"

enum class SplitBarType
{
	Vertical,
	Horizontal
};

class D3D11_UI_FRAMEWORK_INTERFACE_API UISplitBar : public UIElementBase
{
public:
	UISplitBar() = default;
	virtual ~UISplitBar();

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
	bool HitTest(float x, float y) const override;

	// UIElementBase Override
	void OnLayoutChanged() override;

public:
	void SetLineColor(const D2D1_COLOR_F& color);
	void SetSplitbarType(const SplitBarType& splitbarType);
	void SetLineThickness(float lineThickness);

private:
	bool CreateLineBrush(IRenderContext* context, bool resetState);

private:
	// split bar properties
	float m_thickness = 1.0f;
	float m_padding = 3.0f;

	// line draw type
	SplitBarType m_splitbarType = SplitBarType::Horizontal;

	// render color
	bool m_updateColor = true;
	D2D1_COLOR_F m_borderColor = {};

	ID2D1SolidColorBrush* m_borderBrush = nullptr;
};

