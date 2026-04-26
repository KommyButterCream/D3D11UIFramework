#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "UIPanel.h"

struct ID2D1Bitmap1;
struct ID2D1Effect;

class D3D11_UI_FRAMEWORK_INTERFACE_API UIContextMenuPanel final : public UIPanel
{
public:
	UIContextMenuPanel();
	virtual ~UIContextMenuPanel();

	// IRenderLayer Override
	bool Initialize(IRenderContext* context) override;
	void Shutdown() override;
	bool Update(float dt) override;
	bool Prepare() override;
	bool Render() override;
	void DiscardDeviceResources() override;
	bool RestoreDeviceResources(IRenderContext* context) override;

	// UIElementBase Override
	void OnMouseEvent(UIMouseEventType type, float x, float y) override;
	void OnLayoutChanged() override;

	// UIPanel Override
	bool HandleMouseEvent(UIMouseEventType type, float x, float y) override;

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
	void Hide();

private:
	float m_menuWidth = 200.0f;

	// Shadow
	ID2D1Bitmap1* m_shadowMask = nullptr;
	ID2D1Effect* m_shadowEffect = nullptr;
	float m_shadowBlur = 12.0f;
	float m_shadowOpacity = 0.35f;
	float m_shadowOffsetX = 6.0f;
	float m_shadowOffsetY = 6.0f;
	bool m_shadowDirty = true;
};

