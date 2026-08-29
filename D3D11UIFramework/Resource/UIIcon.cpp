#include "pch.h"
#include "UIIcon.h"

#include "../../../Module/D3D11EngineInterface/IRenderContext.h"
#include "UISVGResource.h"

#include <string.h>

UIIcon::~UIIcon()
{
	Reset();
}

void UIIcon::SetPath(const wchar_t* path)
{
	if (m_path)
	{
		delete[] m_path;
		m_path = nullptr;
	}

	// 경로가 바뀌면 들고 있던 리소스는 다른 그림이다.
	ReleaseDeviceResources();

	if (!path)
	{
		return;
	}

	const size_t length = wcslen(path);
	if (length == 0)
	{
		return;
	}

	m_path = new wchar_t[length + 1];
	wcscpy_s(m_path, length + 1, path);
}

bool UIIcon::HasPath() const
{
	return m_path != nullptr;
}

void UIIcon::SetStyle(const UIStyle& style)
{
	m_style = style;
}

UIStyle& UIIcon::Style()
{
	return m_style;
}

const UIStyle& UIIcon::Style() const
{
	return m_style;
}

void UIIcon::SetScale(float scale)
{
	m_scale = scale;
}

float UIIcon::GetScale() const
{
	return m_scale;
}

bool UIIcon::EnsureLoaded(IRenderContext* context)
{
	if (m_loaded)
	{
		return true;
	}

	if (!context || !m_path)
	{
		return false;
	}

	ID2D1DeviceContext5* d2dContext = context->GetD2DDeviceContext5();
	if (!d2dContext)
	{
		return false;
	}

	ReleaseDeviceResources();

	m_resource = new UISVGResource();

	if (!m_resource->Load(d2dContext, m_path))
	{
		delete m_resource;
		m_resource = nullptr;
		return false;
	}

	m_loaded = true;
	return true;
}

bool UIIcon::IsLoaded() const
{
	return m_loaded && m_resource != nullptr;
}

void UIIcon::ReleaseDeviceResources()
{
	if (m_resource)
	{
		delete m_resource;
		m_resource = nullptr;
	}

	m_loaded = false;
}

void UIIcon::Reset()
{
	ReleaseDeviceResources();

	if (m_path)
	{
		delete[] m_path;
		m_path = nullptr;
	}
}

void UIIcon::SnapToState(UIElementState state, float scale)
{
	switch (state)
	{
	case UIElementState::Hovered:  m_smoothColor.Snap(m_style.hover.fill);    break;
	case UIElementState::Pressed:  m_smoothColor.Snap(m_style.pressed.fill);  break;
	case UIElementState::Disabled: m_smoothColor.Snap(m_style.disabled.fill); break;
	case UIElementState::Normal:
	default:                       m_smoothColor.Snap(m_style.normal.fill);   break;
	}

	m_smoothScale.Snap(scale);
}

void UIIcon::ApplyState(UIElementState state,
	float normalScale, float hoveredScale, float pressedScale)
{
	// 로드 전이면 목표를 걸어봐야 그릴 것이 없다.
	if (!IsLoaded())
	{
		return;
	}

	switch (state)
	{
	case UIElementState::Normal:
		m_smoothColor.SetTarget(m_style.normal.fill);
		m_smoothScale.SetTarget(normalScale);
		break;

	case UIElementState::Hovered:
		m_smoothColor.SetTarget(m_style.hover.fill);
		m_smoothScale.SetTarget(hoveredScale);
		break;

	case UIElementState::Pressed:
		m_smoothColor.SetTarget(m_style.pressed.fill);
		m_smoothScale.SetTarget(pressedScale);
		break;

	case UIElementState::Disabled:
		m_smoothColor.SetTarget(m_style.disabled.fill);
		m_smoothScale.SetTarget(normalScale);
		break;
	}
}

bool UIIcon::UpdateAnimation(float dt, float colorSpeed, float scaleSpeed)
{
	if (!IsLoaded())
	{
		return false;
	}

	m_smoothColor.Update(dt, colorSpeed);
	m_smoothScale.Update(dt, scaleSpeed);

	return !m_smoothColor.IsAtTarget() || !m_smoothScale.IsAtTarget();
}

bool UIIcon::Draw(ID2D1DeviceContext5* d2dContext, const D2D1_RECT_F& drawRect,
	float scaleOverride) const
{
	if (!d2dContext || !IsLoaded())
	{
		return false;
	}

	const float iconWidth = m_resource->GetViewBoxWidth();
	const float iconHeight = m_resource->GetViewBoxHeight();

	if (iconWidth <= 0.0f || iconHeight <= 0.0f)
	{
		return false;
	}

	const float areaWidth = drawRect.right - drawRect.left;
	const float areaHeight = drawRect.bottom - drawRect.top;

	if (areaWidth <= 0.0f || areaHeight <= 0.0f)
	{
		return false;
	}

	// 영역에 종횡비를 지켜 맞춘 뒤 사용자 배율과 애니메이션 배율을 곱한다.
	//
	// 예전 UIButton 은 min(폭, 높이) 로 정사각형을 만들어 거기에 맞췄는데,
	// 비정사각 영역에서 아이콘이 필요 이상으로 작아졌다. 툴바 버튼은 정사각형
	// 이라 결과가 같았고, UIIconLabel/UIContextMenuButton 은 이미 이 식을
	// 쓰고 있었다. 다수 쪽이자 옳은 쪽으로 통일한다.
	const float baseScale = (scaleOverride > 0.0f) ? scaleOverride : m_scale;

	const float scale =
		min(areaWidth / iconWidth, areaHeight / iconHeight) *
		baseScale *
		m_smoothScale.GetCurrent();

	if (scale <= 0.0f)
	{
		return false;
	}

	const float drawWidth = iconWidth * scale;
	const float drawHeight = iconHeight * scale;
	const float drawX = drawRect.left + (areaWidth - drawWidth) * 0.5f;
	const float drawY = drawRect.top + (areaHeight - drawHeight) * 0.5f;

	D2D1_MATRIX_3X2_F oldTransform = {};
	d2dContext->GetTransform(&oldTransform);

	m_resource->SetColor(m_smoothColor.GetColor());

	d2dContext->SetTransform(
		D2D1::Matrix3x2F::Scale(scale, scale) *
		D2D1::Matrix3x2F::Translation(drawX, drawY));

	m_resource->Render(d2dContext);
	d2dContext->SetTransform(oldTransform);

	return true;
}
