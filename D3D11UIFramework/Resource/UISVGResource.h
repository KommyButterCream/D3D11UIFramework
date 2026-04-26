#pragma once

#ifdef BUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllexport)
#else
#define D3D11_UI_FRAMEWORK_INTERFACE_API __declspec(dllimport)
#endif

struct ID2D1DeviceContext5;
struct ID2D1SvgDocument;
struct ID2D1SvgPaint;

class D3D11_UI_FRAMEWORK_INTERFACE_API UISVGResource
{
public:
	UISVGResource() = default;
	~UISVGResource();

	bool Load(ID2D1DeviceContext5* d2dContext, const wchar_t* path);
	void Render(ID2D1DeviceContext5* d2dContext);

	ID2D1SvgDocument* Get() const { return m_svg; };

	float GetViewBoxWidth() const { return m_viewBoxWidth; };
	float GetViewBoxHeight() const { return m_viewBoxHeight; };

	void SetColor(const D2D1_COLOR_F& color);

private:
	void ReleaseResources();
	bool InitializeViewBox(ID2D1SvgElement* root);
	bool InitializeSvgFillColor(ID2D1SvgElement* root, const D2D1_COLOR_F& color);
private:
	ID2D1SvgDocument* m_svg = nullptr;
	ID2D1SvgPaint* m_fillPaint = nullptr;

	static constexpr float m_defaultViewBoxValue = 24.0f;

	float m_viewBoxWidth = 0.0f;
	float m_viewBoxHeight = 0.0f;
};
