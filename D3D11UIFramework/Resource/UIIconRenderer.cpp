// pch.h 를 쓰지 않는다. 미리보기 하네스가 이 파일만 직접 컴파일한다.
#include "UIIconRenderer.h"

#include <d2d1.h>

namespace
{
	inline D2D1_POINT_2F P(float x, float y)
	{
		return D2D1::Point2F(x, y);
	}

	template <typename T>
	void Release(T*& p)
	{
		if (p)
		{
			p->Release();
			p = nullptr;
		}
	}

	// 꺾은선 하나. 아이콘 대부분이 이것만으로 만들어진다.
	void Polyline(ID2D1RenderTarget* target, ID2D1Brush* brush,
		ID2D1StrokeStyle* style, float stroke,
		const D2D1_POINT_2F* pts, size_t count)
	{
		for (size_t i = 1; i < count; ++i)
		{
			target->DrawLine(pts[i - 1], pts[i], brush, stroke, style);
		}
	}
}

UIIconRenderer::~UIIconRenderer()
{
	Shutdown();
}

bool UIIconRenderer::Initialize(ID2D1RenderTarget* target)
{
	Shutdown();

	if (!target)
		return false;

	target->GetFactory(&m_factory);
	if (!m_factory)
		return false;

	// 모든 아이콘이 공유하는 단 하나의 스트로크 스타일.
	//
	// 끝과 이음을 둥글게 두면 얇은 선이 작은 크기에서도 뭉개지지 않고,
	// 꺾이는 지점이 뾰족하게 튀지 않는다.
	D2D1_STROKE_STYLE_PROPERTIES props = D2D1::StrokeStyleProperties();
	props.startCap = D2D1_CAP_STYLE_ROUND;
	props.endCap = D2D1_CAP_STYLE_ROUND;
	props.lineJoin = D2D1_LINE_JOIN_ROUND;

	if (FAILED(m_factory->CreateStrokeStyle(props, nullptr, 0, &m_roundStroke)))
	{
		Shutdown();
		return false;
	}

	if (FAILED(target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_brush)))
	{
		Shutdown();
		return false;
	}

	return true;
}

void UIIconRenderer::Shutdown()
{
	Release(m_cursorGeometry);
	Release(m_brush);
	Release(m_roundStroke);
	Release(m_factory);
}

bool UIIconRenderer::EnsureCursorGeometry(ID2D1RenderTarget* target)
{
	if (m_cursorGeometry)
		return true;

	if (!m_factory)
		return false;

	(void)target;

	if (FAILED(m_factory->CreatePathGeometry(&m_cursorGeometry)))
		return false;

	ID2D1GeometrySink* sink = nullptr;
	if (FAILED(m_cursorGeometry->Open(&sink)))
	{
		Release(m_cursorGeometry);
		return false;
	}

	// 내비게이션 화살표. 채우기라 직선만으로 되지만, 매번 5개 선을 잇는 대신
	// 한 번 만들어 두고 FillGeometry 로 그린다.
	sink->BeginFigure(P(0.92f, 0.10f), D2D1_FIGURE_BEGIN_FILLED);
	sink->AddLine(P(0.08f, 0.46f));
	sink->AddLine(P(0.44f, 0.56f));
	sink->AddLine(P(0.54f, 0.92f));
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);

	const HRESULT hr = sink->Close();
	sink->Release();

	if (FAILED(hr))
	{
		Release(m_cursorGeometry);
		return false;
	}

	return true;
}

bool UIIconRenderer::Draw(
	ID2D1RenderTarget* target,
	UIIconShape shape,
	const D2D1_RECT_F& drawRect,
	const D2D1_COLOR_F& color,
	float strokeScale)
{
	if (!target || !m_brush || shape == UIIconShape::None)
		return false;

	const float areaWidth = drawRect.right - drawRect.left;
	const float areaHeight = drawRect.bottom - drawRect.top;

	if (areaWidth <= 0.0f || areaHeight <= 0.0f)
		return false;

	// 정사각형으로 맞춰 가운데 정렬한다.
	const float size = (areaWidth < areaHeight) ? areaWidth : areaHeight;
	const float originX = drawRect.left + (areaWidth - size) * 0.5f;
	const float originY = drawRect.top + (areaHeight - size) * 0.5f;

	m_brush->SetColor(color);

	D2D1_MATRIX_3X2_F oldTransform = {};
	target->GetTransform(&oldTransform);

	// 0..1 좌표계를 대상 사각형으로 옮긴다. 선 두께도 같이 스케일되므로
	// 아이콘이 어느 크기에서도 같은 비율로 보인다.
	target->SetTransform(
		D2D1::Matrix3x2F::Scale(size, size) *
		D2D1::Matrix3x2F::Translation(originX, originY) *
		oldTransform);

	// 24px 기준 약 2px 선. 0..1 좌표라 크기와 무관하다.
	const float stroke = 0.085f * strokeScale;

	DrawShape(target, shape, stroke);

	target->SetTransform(oldTransform);

	return true;
}

void UIIconRenderer::DrawShape(ID2D1RenderTarget* target, UIIconShape shape, float stroke)
{
	ID2D1Brush* brush = m_brush;
	ID2D1StrokeStyle* style = m_roundStroke;

	switch (shape)
	{
	case UIIconShape::ZoomIn:
	case UIIconShape::ZoomOut:
	{
		// 렌즈 + 손잡이.
		const D2D1_ELLIPSE lens = D2D1::Ellipse(P(0.42f, 0.42f), 0.30f, 0.30f);
		target->DrawEllipse(lens, brush, stroke, style);
		target->DrawLine(P(0.64f, 0.64f), P(0.92f, 0.92f), brush, stroke * 1.15f, style);

		// 가로선은 둘 다, 세로선은 ZoomIn 만.
		target->DrawLine(P(0.26f, 0.42f), P(0.58f, 0.42f), brush, stroke, style);

		if (shape == UIIconShape::ZoomIn)
		{
			target->DrawLine(P(0.42f, 0.26f), P(0.42f, 0.58f), brush, stroke, style);
		}
		break;
	}

	case UIIconShape::ZoomOneToOne:
	{
		// 모서리 4개가 안쪽을 향한다 — "실제 크기로 줄인다".
		const D2D1_POINT_2F tl[] = { P(0.34f, 0.08f), P(0.34f, 0.34f), P(0.08f, 0.34f) };
		const D2D1_POINT_2F tr[] = { P(0.66f, 0.08f), P(0.66f, 0.34f), P(0.92f, 0.34f) };
		const D2D1_POINT_2F bl[] = { P(0.34f, 0.92f), P(0.34f, 0.66f), P(0.08f, 0.66f) };
		const D2D1_POINT_2F br[] = { P(0.66f, 0.92f), P(0.66f, 0.66f), P(0.92f, 0.66f) };

		Polyline(target, brush, style, stroke, tl, 3);
		Polyline(target, brush, style, stroke, tr, 3);
		Polyline(target, brush, style, stroke, bl, 3);
		Polyline(target, brush, style, stroke, br, 3);
		break;
	}

	case UIIconShape::ZoomFit:
	{
		// 모서리 4개가 바깥을 향한다 — "화면에 맞춰 넓힌다".
		const D2D1_POINT_2F tl[] = { P(0.08f, 0.34f), P(0.08f, 0.08f), P(0.34f, 0.08f) };
		const D2D1_POINT_2F tr[] = { P(0.66f, 0.08f), P(0.92f, 0.08f), P(0.92f, 0.34f) };
		const D2D1_POINT_2F bl[] = { P(0.08f, 0.66f), P(0.08f, 0.92f), P(0.34f, 0.92f) };
		const D2D1_POINT_2F br[] = { P(0.66f, 0.92f), P(0.92f, 0.92f), P(0.92f, 0.66f) };

		Polyline(target, brush, style, stroke, tl, 3);
		Polyline(target, brush, style, stroke, tr, 3);
		Polyline(target, brush, style, stroke, bl, 3);
		Polyline(target, brush, style, stroke, br, 3);
		break;
	}

	case UIIconShape::MeasureDistance:
	{
		// 양 끝점 + 잇는 선. 지금 툴바가 쓰는 '↔' 글리프를 대신한다.
		target->DrawLine(P(0.18f, 0.72f), P(0.82f, 0.28f), brush, stroke, style);

		const D2D1_ELLIPSE a = D2D1::Ellipse(P(0.18f, 0.72f), 0.12f, 0.12f);
		const D2D1_ELLIPSE b = D2D1::Ellipse(P(0.82f, 0.28f), 0.12f, 0.12f);
		target->FillEllipse(a, brush);
		target->FillEllipse(b, brush);
		break;
	}

	case UIIconShape::MeasureAngle:
	{
		// 꼭짓점에서 뻗은 두 변 + 사이의 호.
		target->DrawLine(P(0.14f, 0.84f), P(0.90f, 0.84f), brush, stroke, style);
		target->DrawLine(P(0.14f, 0.84f), P(0.76f, 0.22f), brush, stroke, style);

		// 각을 나타내는 호. 꼭짓점 중심의 원에서 1/8 조각만 그린다.
		const D2D1_ELLIPSE arc = D2D1::Ellipse(P(0.14f, 0.84f), 0.34f, 0.34f);
		target->PushAxisAlignedClip(
			D2D1::RectF(0.30f, 0.50f, 0.52f, 0.86f),
			D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		target->DrawEllipse(arc, brush, stroke * 0.8f, style);
		target->PopAxisAlignedClip();
		break;
	}

	case UIIconShape::PixelGrid:
	{
		// 3x3 격자. 가운데 칸만 채워서 "픽셀 하나를 본다" 를 나타낸다.
		const float a = 0.12f;
		const float b = 0.88f;
		const float s = (b - a) / 3.0f;

		target->FillRectangle(
			D2D1::RectF(a + s, a + s, a + s * 2.0f, a + s * 2.0f), brush);

		for (int i = 0; i <= 3; ++i)
		{
			const float t = a + s * static_cast<float>(i);
			target->DrawLine(P(t, a), P(t, b), brush, stroke * 0.75f, style);
			target->DrawLine(P(a, t), P(b, t), brush, stroke * 0.75f, style);
		}
		break;
	}

	case UIIconShape::Lut:
	{
		// 반쯤 채운 원 — 대비 조절의 관용적 표기.
		//
		// LUT 를 켜는 것이 자동 대비이므로 의미가 맞는다. 히스토그램 막대도
		// 후보였지만, 작은 크기에서 막대가 서로 뭉개져 읽히지 않는다.
		const D2D1_ELLIPSE outer = D2D1::Ellipse(P(0.5f, 0.5f), 0.40f, 0.40f);
		target->DrawEllipse(outer, brush, stroke, style);

		// 오른쪽 절반만 채운다. 사각형으로 잘라 채우기 위해 클립을 쓴다.
		target->PushAxisAlignedClip(
			D2D1::RectF(0.5f, 0.0f, 1.0f, 1.0f),
			D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		target->FillEllipse(outer, brush);
		target->PopAxisAlignedClip();
		break;
	}

	case UIIconShape::Check:
	{
		const D2D1_POINT_2F pts[] = { P(0.16f, 0.52f), P(0.40f, 0.78f), P(0.86f, 0.24f) };
		Polyline(target, brush, style, stroke * 1.25f, pts, 3);
		break;
	}

	case UIIconShape::ChevronRight:
	{
		const D2D1_POINT_2F pts[] = { P(0.36f, 0.20f), P(0.66f, 0.50f), P(0.36f, 0.80f) };
		Polyline(target, brush, style, stroke, pts, 3);
		break;
	}

	case UIIconShape::Cursor:
	{
		if (EnsureCursorGeometry(target))
		{
			target->FillGeometry(m_cursorGeometry, brush);
		}
		break;
	}

	case UIIconShape::Palette:
	{
		const D2D1_ELLIPSE outer = D2D1::Ellipse(P(0.5f, 0.5f), 0.40f, 0.40f);
		target->DrawEllipse(outer, brush, stroke, style);

		// 물감 세 방울.
		target->FillEllipse(D2D1::Ellipse(P(0.36f, 0.36f), 0.09f, 0.09f), brush);
		target->FillEllipse(D2D1::Ellipse(P(0.64f, 0.40f), 0.09f, 0.09f), brush);
		target->FillEllipse(D2D1::Ellipse(P(0.44f, 0.66f), 0.09f, 0.09f), brush);
		break;
	}

	case UIIconShape::ImageSize:
	{
		// 액자 + 해 + 능선. 이미지 크기 라벨 옆에 붙는다.
		const D2D1_ROUNDED_RECT frame =
			D2D1::RoundedRect(D2D1::RectF(0.10f, 0.18f, 0.90f, 0.82f), 0.08f, 0.08f);
		target->DrawRoundedRectangle(frame, brush, stroke, style);

		target->FillEllipse(D2D1::Ellipse(P(0.32f, 0.36f), 0.07f, 0.07f), brush);

		const D2D1_POINT_2F ridge[] = {
			P(0.16f, 0.74f), P(0.38f, 0.52f), P(0.54f, 0.66f), P(0.68f, 0.54f), P(0.84f, 0.70f)
		};
		Polyline(target, brush, style, stroke * 0.9f, ridge, 5);
		break;
	}

	default:
		break;
	}
}
