#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <d2d1_1.h>

#include "UIIconShape.h"

// 내장 아이콘을 D2D 로 그린다.
//
// ID2D1RenderTarget 기준으로 동작한다. ID2D1DeviceContext 가 여기서 파생되므로
// 뷰어의 렌더 경로에서도, WIC 비트맵 타깃에서도 같은 코드가 돈다.
// (SVG 경로는 ID2D1DeviceContext5 가 필요했다)
//
// 캐시하는 것:
//   - 스트로크 스타일 하나. 모든 아이콘이 둥근 끝/이음을 공유한다.
//   - 브러시 하나. 색만 갈아 끼운다.
//   - 곡선이 있어 프리미티브로 못 그리는 아이콘의 지오메트리.
//
// 원이나 직선은 지오메트리로 만들지 않는다. DrawEllipse 한 줄이 arc 두 개를
// 손으로 넣는 것보다 짧고, 무엇보다 코드를 읽고 어떤 아이콘인지 알 수 있다.
class D3D11_UI_FRAMEWORK_INTERFACE_API UIIconRenderer
{
public:
	UIIconRenderer() = default;
	~UIIconRenderer();

	UIIconRenderer(const UIIconRenderer&) = delete;
	UIIconRenderer& operator=(const UIIconRenderer&) = delete;

	// 디바이스가 바뀌면 다시 부른다.
	bool Initialize(ID2D1RenderTarget* target);
	void Shutdown();

	// drawRect 안에 아이콘을 그린다.
	//
	// 정사각형으로 맞춰 가운데 정렬한다. strokeScale 은 선 두께 배율로,
	// 1.0 이 기본이다.
	bool Draw(
		ID2D1RenderTarget* target,
		UIIconShape shape,
		const D2D1_RECT_F& drawRect,
		const D2D1_COLOR_F& color,
		float strokeScale = 1.0f);

private:
	// 0..1 좌표계에서 그린다. 호출자가 변환을 걸어 둔 상태다.
	void DrawShape(ID2D1RenderTarget* target, UIIconShape shape, float stroke);

	bool EnsureCursorGeometry(ID2D1RenderTarget* target);

private:
	ID2D1StrokeStyle* m_roundStroke = nullptr;
	ID2D1SolidColorBrush* m_brush = nullptr;

	// 곡선이 필요한 것만. 지금은 커서 하나뿐이다.
	ID2D1PathGeometry* m_cursorGeometry = nullptr;

	ID2D1Factory* m_factory = nullptr;
};
