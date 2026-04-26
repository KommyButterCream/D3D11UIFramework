#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#include "UIPanel.h"

class D3D11_UI_FRAMEWORK_INTERFACE_API UIStatusPanel final : public UIPanel
{
public:
	UIStatusPanel() = default;
	~UIStatusPanel() override = default;
};

