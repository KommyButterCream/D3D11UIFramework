#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "../Base/UIElementBase.h"
#include <memory>
#include <vector>
#include <stdint.h>

struct ID2D1SolidColorBrush;
class IRenderContext;

// C4251: m_children 이 std::vector 라서 나는 경고. 이 벡터는 DLL 안에서만
// 생성·소멸·순회되고(파생 패널도 전부 이 DLL 소속) 밖으로 노출되는 접근자는
// AddChild/RemoveChild/ClearChildren 뿐이다. 클라이언트가 벡터 내부를 직접
// 만지지 않으므로 CRT 힙/이터레이터 디버그 레벨 불일치 문제가 생기지 않는다.
#pragma warning(push)
#pragma warning(disable : 4251)

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
	void Shutdown() override;
	bool Update(float dt) override;
	bool Render() override;
	void DiscardDeviceResources() override;

	// UIElementBase Override
	// 패널의 유일한 마우스 진입점. 반환값은 소비 여부다.
	bool OnMouseEvent(UIMouseEventType type, float x, float y) override;
	void OnLayoutChanged() override;

public:
	void AddChild(std::shared_ptr<UIElementBase> child);
	void RemoveChild(std::shared_ptr<UIElementBase> child);
	void ClearChildren();


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
	// UIElementBase Override
	bool AcquireDeviceResources(IRenderContext* context, bool reset) override;

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
	bool InitializeChildren(IRenderContext* context);
	void ShutdownChildren();
	void DiscardChildDeviceResources();
	bool RestoreChildDeviceResources(IRenderContext* context);
	UIElementBase* HitTestRecursive(float x, float y);
	void UpdateVerticalLayout();
	void UpdateHorizontalLayout();

protected:
	// 예전에는 이 vector 하나만 pimpl(UIPanelImpl) 뒤에 있었다. 파생 클래스가
	// 자식 목록을 순회하려면 결국 impl 헤더를 include 해야 했으므로 숨겨서
	// 얻는 것이 없었고, 널 체크(EnsureImpl)만 곳곳에 붙었다.
	std::vector<std::shared_ptr<UIElementBase>> m_children;

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


#pragma warning(pop)
