#include "pch.h"
#include "UIIconHelper.h"

#include "../../Modules/D3D11EngineInterface/IRenderContext.h"
#include "UISVGResource.h"

#include <string.h>

void UIIconHelper::ReleaseIcon(UISVGResource*& icon, bool& loaded)
{
	if (icon)
	{
		delete icon;
		icon = nullptr;
	}

	loaded = false;
}

void UIIconHelper::ReleaseIconState(wchar_t*& iconPath, UISVGResource*& icon, bool& loaded)
{
	ReleaseIcon(icon, loaded);

	if (iconPath)
	{
		delete[] iconPath;
		iconPath = nullptr;
	}
}

void UIIconHelper::ResetIconPath(wchar_t*& iconPath, const wchar_t* path, UISVGResource*& icon, bool& loaded)
{
	if (iconPath)
	{
		delete[] iconPath;
		iconPath = nullptr;
	}

	ReleaseIcon(icon, loaded);

	if (!path)
		return;

	const size_t length = wcslen(path);
	if (length == 0)
		return;

	iconPath = new wchar_t[length + 1];
	if (!iconPath)
		return;

	wcscpy_s(iconPath, length + 1, path);
}

void UIIconHelper::EnsureIconLoaded(IRenderContext* context, wchar_t* iconPath, UISVGResource*& icon, bool& loaded)
{
	if (loaded || !context || !iconPath)
		return;

	ID2D1DeviceContext5* d2dContext = context->GetD2DDeviceContext5();
	if (!d2dContext)
		return;

	ReleaseIcon(icon, loaded);

	icon = new UISVGResource();
	if (!icon)
		return;

	if (icon->Load(d2dContext, iconPath))
	{
		loaded = true;
	}
	else
	{
		delete icon;
		icon = nullptr;
	}
}

void UIIconHelper::ApplyIconStateTargets(
	UIElementState state,
	const UIStyle& iconStyle,
	bool iconLoaded,
	SmoothColor& smoothIconColor,
	SmoothValue& smoothIconScale,
	float normalScale,
	float hoveredScale,
	float pressedScale)
{
	if (!iconLoaded)
		return;

	switch (state)
	{
	case UIElementState::Normal:
		smoothIconColor.SetTarget(iconStyle.normal.fill);
		smoothIconScale.SetTarget(normalScale);
		break;

	case UIElementState::Hovered:
		smoothIconColor.SetTarget(iconStyle.hover.fill);
		smoothIconScale.SetTarget(hoveredScale);
		break;

	case UIElementState::Pressed:
		smoothIconColor.SetTarget(iconStyle.pressed.fill);
		smoothIconScale.SetTarget(pressedScale);
		break;

	case UIElementState::Disabled:
		smoothIconColor.SetTarget(iconStyle.disabled.fill);
		smoothIconScale.SetTarget(normalScale);
		break;
	}
}

bool UIIconHelper::UpdateIconAnimation(
	bool iconLoaded,
	float dt,
	float colorAnimationSpeed,
	float scaleAnimationSpeed,
	SmoothColor& smoothIconColor,
	SmoothValue& smoothIconScale)
{
	if (!iconLoaded)
		return false;

	smoothIconColor.Update(dt, colorAnimationSpeed);
	smoothIconScale.Update(dt, scaleAnimationSpeed);

	return
		!smoothIconColor.IsAtTarget() ||
		!smoothIconScale.IsAtTarget();
}

bool UIIconHelper::DrawCenteredIcon(
	ID2D1DeviceContext5* d2dContext,
	UISVGResource* icon,
	const D2D1_RECT_F& drawRect,
	const D2D1_COLOR_F& color,
	float scale)
{
	if (!d2dContext || !icon || scale <= 0.0f)
		return false;

	const float iconWidth = icon->GetViewBoxWidth();
	const float iconHeight = icon->GetViewBoxHeight();

	if (iconWidth <= 0.0f || iconHeight <= 0.0f)
		return false;

	const float areaWidth = drawRect.right - drawRect.left;
	const float areaHeight = drawRect.bottom - drawRect.top;

	const float drawWidth = iconWidth * scale;
	const float drawHeight = iconHeight * scale;
	const float drawX = drawRect.left + (areaWidth - drawWidth) * 0.5f;
	const float drawY = drawRect.top + (areaHeight - drawHeight) * 0.5f;

	D2D1_MATRIX_3X2_F oldTransform = {};
	d2dContext->GetTransform(&oldTransform);

	icon->SetColor(color);

	d2dContext->SetTransform(
		D2D1::Matrix3x2F::Scale(scale, scale) *
		D2D1::Matrix3x2F::Translation(drawX, drawY));

	icon->Render(d2dContext);
	d2dContext->SetTransform(oldTransform);

	return true;
}
