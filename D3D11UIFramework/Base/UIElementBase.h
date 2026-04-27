#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "../../../Module/Core/ShapeType/Rect2f.h"
#include "../../../Module/D3D11EngineInterface/IUIRenderLayer.h"
#include "../Resource/ColorRGBA8.h"

namespace RYLYNN = Core::ShapeType;

struct UIColorSet
{
	D2D1_COLOR_F fill = { 0.0f, 0.0f, 0.0f, 0.0f };
	D2D1_COLOR_F border = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct UIStyle
{
	UIColorSet normal = {};
	UIColorSet hover = {};
	UIColorSet pressed = {};
	UIColorSet disabled = {};

	float borderThickness = 1.0f;
};

struct UITextStyle
{
	UIColorSet normal = {};
	UIColorSet hover = {};
	UIColorSet pressed = {};
	UIColorSet disabled = {};

	wchar_t fontName[50] = {};
	float fontSize = 14.0f;

	DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;
	DWRITE_TEXT_ALIGNMENT hAlign = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER;
	DWRITE_PARAGRAPH_ALIGNMENT vAlign = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
};

class IRenderContext;
class UIElementBaseImpl;

class D3D11_UI_FRAMEWORK_INTERFACE_API UIElementBase : public IUIRenderLayer
{
public:
	UIElementBase();
	virtual ~UIElementBase();

	UIElementBase(const UIElementBase&) = delete;
	UIElementBase& operator=(const UIElementBase&) = delete;
	UIElementBase(UIElementBase&&) = delete;
	UIElementBase& operator=(UIElementBase&&) = delete;

	// IRenderLayer Override
	bool Initialize(IRenderContext* context) override;
	void Shutdown() override;
	bool Render() override = 0;
	bool Prepare() override;

	// IUIRenderLayer Override
	bool HitTest(float x, float y) const override;
	void OnMouseEvent(UIMouseEventType type, float x, float y) override;

	virtual bool Update(float dt) = 0;
	virtual UIElementState GetState() const override;
	virtual void SetState(UIElementState state) override;

	virtual bool IsVisible() const override;
	virtual void SetVisible(bool visible) override;

	virtual void OnLayoutChanged();
	virtual void DiscardDeviceResources();
	virtual bool RestoreDeviceResources(IRenderContext* context);

public:
	void SetLayout(const RYLYNN::Rect2f& rect);
	const RYLYNN::Rect2f& GetLayout() const;

	void SetStyle(const UIStyle& style);
	UIStyle& GetStyle();
	const UIStyle& GetStyle() const;

protected:
	UIElementState m_state = UIElementState::Normal;
	bool m_visible = true;
	bool m_mouseOver = false;

	UIStyle m_style = {};

	IRenderContext* m_context = nullptr;

protected:
	bool BindRenderContext(IRenderContext* context, bool resetState);
	RYLYNN::Rect2f& LayoutData();
	const RYLYNN::Rect2f& LayoutData() const;
	virtual void OnStateChanged(UIElementState oldState, UIElementState newState);

private:
	UIElementBaseImpl* m_impl = nullptr;
};
