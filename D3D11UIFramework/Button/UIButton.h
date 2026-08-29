#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "../Label/UILabel.h"
#include "../../../Module/D3D11Engine/util/SmoothValue.h"
#include "../Resource/UIIcon.h"


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
	ImageCenterCrossLine,

	// 두 점 사이 거리 측정. 토글이다.
	MeasureDistance,

	// 현재 붙어 있는 이미지를 파일로 저장한다. 형식마다 항목이 따로 있다.
	SaveImagePng,
	SaveImageJpeg,
	SaveImageBmp
};

class D3D11_UI_FRAMEWORK_INTERFACE_API UIButton : public UILabel
{
public:
	UIButton() = default;
	virtual ~UIButton();

public:
	// IRenderLayer Override
	void Shutdown() override;
	bool Update(float dt) override;
	bool Render() override;
	void DiscardDeviceResources() override;

	// UIElementBase Override
	//
	// 상태 머신은 재정의하지 않는다. 클릭이 성립했을 때 할 일만 여기 둔다.
	void OnActivated() override;
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
	// UIElementBase Override
	bool AcquireDeviceResources(IRenderContext* context, bool reset) override;

	// mouse event
	UICommand m_command = UICommand::None;
	UIEventDispatcher* m_dispatcher = nullptr;

	// 아이콘은 UIIcon 이 통째로 들고 있다. 예전에는 경로/리소스/로드 여부/
	// 배율/애니메이션 값이 멤버로 흩어져 있었고 UIIconLabel 이 같은 다섯 개를
	// 그대로 복제하고 있었다.
	UIIcon m_icon;
};

