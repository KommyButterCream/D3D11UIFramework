#pragma once

#include "../Base/UIElementBase.h"
#include "../../Modules/D3D11Engine/util/SmoothValue.h"

class IRenderContext;
class UISVGResource;
struct ID2D1DeviceContext5;

class UIIconHelper
{
public:
	static void ReleaseIcon(UISVGResource*& icon, bool& loaded);
	static void ReleaseIconState(wchar_t*& iconPath, UISVGResource*& icon, bool& loaded);
	static void ResetIconPath(wchar_t*& iconPath, const wchar_t* path, UISVGResource*& icon, bool& loaded);
	static void EnsureIconLoaded(IRenderContext* context, wchar_t* iconPath, UISVGResource*& icon, bool& loaded);
	static void ApplyIconStateTargets(
		UIElementState state,
		const UIStyle& iconStyle,
		bool iconLoaded,
		SmoothColor& smoothIconColor,
		SmoothValue& smoothIconScale,
		float normalScale,
		float hoveredScale,
		float pressedScale);
	static bool UpdateIconAnimation(
		bool iconLoaded,
		float dt,
		float colorAnimationSpeed,
		float scaleAnimationSpeed,
		SmoothColor& smoothIconColor,
		SmoothValue& smoothIconScale);
	static bool DrawCenteredIcon(
		ID2D1DeviceContext5* d2dContext,
		UISVGResource* icon,
		const D2D1_RECT_F& drawRect,
		const D2D1_COLOR_F& color,
		float scale);
};
