#pragma once

#include "../Resource/ColorRGBA8.h"

namespace UIDefaultColor
{
	// Common
	static constexpr ColorRGBA8 transparent = { 0, 0, 0, 0 };

	// Button
	static constexpr ColorRGBA8 buttonNormalColor = { 32, 32, 32, 255 };
	static constexpr ColorRGBA8 buttonHoverColor = { 51, 51, 51, 255 }; // 45
	static constexpr ColorRGBA8 buttonPressedColor = { 41, 41, 41, 255 };
	static constexpr ColorRGBA8 buttonDisabledColor = { 21, 21, 21, 255 };

	static constexpr ColorRGBA8 contextButtonNormalColor = { 46, 46, 46, 255 };
	static constexpr ColorRGBA8 contextButtonHoverColor = { 61, 61, 61, 255 }; // 45
	static constexpr ColorRGBA8 contextButtonPressedColor = { 53, 53, 53, 255 };
	static constexpr ColorRGBA8 contextButtonDisabledColor = { 21, 21, 21, 255 };

	// SplitBar
	static constexpr ColorRGBA8 contextSplitBarColor = { 71, 71, 71, 255 };

	// Image
	static constexpr ColorRGBA8 imageBackgroundColor = { 32, 32, 32, 255 };

	// Panel
	static constexpr ColorRGBA8 panelNormalColor = { 39, 39, 39, 255 };
	static constexpr ColorRGBA8 contextMenuPanelNormalColor = { 46, 46, 46, 255 };
	static constexpr ColorRGBA8 contextMenuPanelBorderColor = { 66, 66, 66, 255 };

	// Icon
	static constexpr ColorRGBA8 iconNormalColor = { 245, 245, 245, 255 };
	static constexpr ColorRGBA8 iconHoverColor = { 255, 255, 255, 255 };
	static constexpr ColorRGBA8 iconPressedColor = { 200, 200, 200, 255 };
	static constexpr ColorRGBA8 iconDisabledColor = { 130, 130, 130, 255 };

	// Text
	static constexpr ColorRGBA8 textNormalColor = { 245, 245, 245, 255 };
	static constexpr ColorRGBA8 textHoverColor = { 255, 255, 255, 255 };
	static constexpr ColorRGBA8 textPressedColor = { 200, 200, 200, 255 };
	static constexpr ColorRGBA8 textDisabledColor = { 21, 21, 21, 255 };

	// Statusbar Text
	static constexpr ColorRGBA8 statusbarTextNormalColor = { 220, 220, 220, 255 };
	static constexpr ColorRGBA8 statusbarTextHoverColor = { 255, 255, 255, 255 };
	static constexpr ColorRGBA8 statusbarTextPressedColor = { 200, 200, 200, 255 };
	static constexpr ColorRGBA8 statusbarTextDisabledColor = { 21, 21, 21, 255 };

	// Label
	static constexpr ColorRGBA8 labelNormalColor = { 46, 46, 46, 0 };
	static constexpr ColorRGBA8 labelHoverColor = { 61, 61, 61, 0 }; // 45
	static constexpr ColorRGBA8 labelPressedColor = { 53, 53, 53, 0 };
	static constexpr ColorRGBA8 labelDisabledColor = { 21, 21, 21, 0 };

	inline const D2D1_COLOR_F transparentF = transparent.ToD2DColor();
}
