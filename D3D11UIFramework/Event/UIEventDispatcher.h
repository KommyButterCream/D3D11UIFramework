#pragma once

#include "../Button/UIButton.h"

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

class D3D11_UI_FRAMEWORK_INTERFACE_API UIEventDispatcher
{
public:
	using UIEventCallback = void(*)(UICommand command, void* userData);

	UIEventDispatcher() = default;
	~UIEventDispatcher() = default;

	void RegisterCallback(UIEventCallback callback, void* userData);
	void Dispatch(UICommand command);

private:
	UIEventCallback m_callback = nullptr;
	void* m_userData = nullptr;
};
