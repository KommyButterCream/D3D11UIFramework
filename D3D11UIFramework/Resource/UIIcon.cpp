#include "pch.h"
#include "UIIcon.h"

#include "../../../Module/D3D11EngineInterface/IRenderContext.h"
#include "UISVGResource.h"

#include <string.h>

UIIcon::~UIIcon()
{
	Reset();
}

void UIIcon::SetShape(UIIconShape shape)
{
	m_shape = shape;
}

UIIconShape UIIcon::GetShape() const
{
	return m_shape;
}

bool UIIcon::HasIcon() const
{
	return m_shape != UIIconShape::None || m_path != nullptr;
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
	if (!context)
	{
		return false;
	}

	ID2D1DeviceContext5* d2dContext = context->GetD2DDeviceContext5();
	if (!d2dContext)
	{
		return false;
	}

	// 내장 도형은 파일이 없으므로 "로드" 가 아니라 캐시 준비다.
	// 스트로크 스타일과 브러시를 디바이스마다 한 번 만든다.
	if (m_shape != UIIconShape::None && !m_shapeReady)
	{
		m_shapeReady = m_shapeRenderer.Initialize(d2dContext);
	}

	// 경로가 없으면 SVG 로 할 일이 없다. 도형만 쓰는 정상 경로다.
	if (!m_path)
	{
		return m_shapeReady;
	}

	if (m_loaded)
	{
		return true;
	}

	if (m_resource)
	{
		delete m_resource;
		m_resource = nullptr;
	}

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
	// 도형은 파일 로드가 없으므로 캐시만 준비되면 그릴 수 있다.
	if (!m_path)
	{
		return m_shape != UIIconShape::None && m_shapeReady;
	}

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

	m_shapeRenderer.Shutdown();
	m_shapeReady = false;
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

	// ── 내장 도형 ────────────────────────────────────────────────────
	//
	// 경로가 설정돼 있지 않으면 이쪽이다. 도형은 0..1 정사각형이라
	// 종횡비 맞춤 계산이 필요 없고, 영역을 배율만큼 줄여서 넘기면 된다.
	if (!m_path && m_shape != UIIconShape::None)
	{
		const float areaW = drawRect.right - drawRect.left;
		const float areaH = drawRect.bottom - drawRect.top;

		if (areaW <= 0.0f || areaH <= 0.0f)
		{
			return false;
		}

		const float base = (scaleOverride > 0.0f) ? scaleOverride : m_scale;
		const float factor = base * m_smoothScale.GetCurrent();

		if (factor <= 0.0f)
		{
			return false;
		}

		const float side = min(areaW, areaH) * factor;
		const float cx = (drawRect.left + drawRect.right) * 0.5f;
		const float cy = (drawRect.top + drawRect.bottom) * 0.5f;

		const D2D1_RECT_F shapeRect = D2D1::RectF(
			cx - side * 0.5f, cy - side * 0.5f,
			cx + side * 0.5f, cy + side * 0.5f);

		return const_cast<UIIconRenderer&>(m_shapeRenderer).Draw(
			d2dContext, m_shape, shapeRect, m_smoothColor.GetColor());
	}

	// ── SVG (옵션) ───────────────────────────────────────────────────
	if (!m_resource)
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
