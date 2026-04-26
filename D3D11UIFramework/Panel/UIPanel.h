#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "../Base/UIElementBase.h"
#include <memory>
#include <stdint.h>

struct ID2D1SolidColorBrush;
class IRenderContext;
class UIPanelImpl;

enum class UILayoutType
{
	None,
	Vertical,
	Horizontal
};

class D3D11_UI_FRAMEWORK_INTERFACE_API UIPanel : public UIElementBase
{
public:
	UIPanel();
	virtual ~UIPanel();

	// IRenderLayer Override
	bool Initialize(IRenderContext* context) override;
	void Shutdown() override;
	bool Update(float dt) override;
	bool Render() override;
	void DiscardDeviceResources() override;
	bool RestoreDeviceResources(IRenderContext* context) override;

	// UIElementBase Override
	void OnMouseEvent(UIMouseEventType type, float x, float y) override;
	void OnLayoutChanged() override;

public:
	void AddChild(std::shared_ptr<UIElementBase> child);
	void RemoveChild(std::shared_ptr<UIElementBase> child);
	void ClearChildren();

	virtual bool HandleMouseEvent(UIMouseEventType type, float x, float y);

	// UI Layout
	void Resize(float width, float height);

	void SetLayoutType(UILayoutType type);
	UILayoutType GetLayoutType() const;
	void SetPadding(float padding);
	float GetPadding() const;
	void SetSpacing(float spacing);
	float GetSpacing() const;

	void SetRounded(bool enable);
	void SetCornerRadius(float radius);

protected:
	bool CreateVisualResources(IRenderContext* context, bool resetState);
	void ReleaseVisualResources();
	ID2D1DeviceContext* GetDeviceContext() const;
	void DrawBackground(ID2D1DeviceContext* d2dContext);
	void UpdateChildLayout();
	int32_t GetChildItemCount() const;
	void UpdateDrawRectCache();
	void NotifyChildrenLeave(float x, float y);
	void NotifyChildLayoutChanged(UIElementBase* child);


private:
	bool EnsureImpl();
	bool InitializeChildren(IRenderContext* context);
	void ShutdownChildren();
	void DiscardChildDeviceResources();
	bool RestoreChildDeviceResources(IRenderContext* context);
	UIElementBase* HitTestRecursive(float x, float y);
	void UpdateVerticalLayout();
	void UpdateHorizontalLayout();

protected:
	UIPanelImpl* m_impl = nullptr;

	UIElementBase* m_hoveredChild = nullptr;

	ID2D1SolidColorBrush* m_fillBrush = nullptr;
	ID2D1SolidColorBrush* m_strokeBrush = nullptr;

	UILayoutType m_layoutType = UILayoutType::None;
	float m_padding = 4.0f; // 패널 내부 여백
	float m_spacing = 4.0f; // 버튼 간격

	// layout
	bool m_isRoundedRect = false;
	D2D1_RECT_F m_layoutRect = {};
	D2D1_ROUNDED_RECT m_layoutRoundedRect = {};
	float m_cornerRadius = 3.0f;
};

