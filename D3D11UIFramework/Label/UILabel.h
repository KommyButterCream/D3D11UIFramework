#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "../Base/UIElementBase.h"
#include "../../Modules/D3D11Engine/util/SmoothValue.h"

struct ID2D1SolidColorBrush;
class UIEventDispatcher;
class IRenderContext;
class UISVGResource;
class FontManager;

class D3D11_UI_FRAMEWORK_INTERFACE_API UILabel : public UIElementBase
{
public:
	UILabel() = default;
	virtual ~UILabel();

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

public:
	void SetFontManager(FontManager* fontManager);

	void SetRounded(bool enable);
	void SetCornerRadius(float radius);

	void SetText(const wchar_t* text);
	void SetTextStyle(const UITextStyle& style);

	void SetTextPadding(float left, float top, float right, float bottom);

protected:
	virtual void UpdateTextLayout();

	bool CreateVisualResources(IRenderContext* context, bool resetState);
	void ReleaseVisualResources();
	void UpdateVisualTargets();
	bool UpdateVisualAnimation(float dt, bool includeText);
	bool ResolveRenderContext(ID2D1DeviceContext5** d2dContext);
	void DrawBackground(ID2D1DeviceContext* d2dContext);
	void DrawD2DText(ID2D1DeviceContext* d2dContext, const D2D1_RECT_F& textRect);
	D2D1_RECT_F GetTextRect() const;

	void UpdateDrawRectCache();
	void EnsureTextUpdate();
	void UpdateTextResources();

protected:
	// render color
	ID2D1SolidColorBrush* m_fillBrush = nullptr;
	ID2D1SolidColorBrush* m_strokeBrush = nullptr;
	ID2D1SolidColorBrush* m_textBrush = nullptr;

	// animation rendering
	SmoothColor m_smoothFillColor = {};
	SmoothColor m_smoothborderColor = {};
	SmoothColor m_smoothTextColor = {};

	// layout
	bool m_isRoundedRect = false;
	D2D1_RECT_F m_layoutRect = {};
	D2D1_ROUNDED_RECT m_layoutRoundedRect = {};
	float m_cornerRadius = 3.0f;

	// font and text
	D2D1_RECT_F m_textPadding = {};
	FontManager* m_fontManager = nullptr;
	UITextStyle m_textStyle = {};
	bool m_hasText = false;
	wchar_t* m_text = nullptr;
	size_t m_textCapacity = 0;
	IDWriteTextFormat* m_textFormat = nullptr;
	IDWriteTextLayout* m_textLayout = nullptr;
	bool m_isFormatDirty = true;
	bool m_isLayoutDirty = true;
};

