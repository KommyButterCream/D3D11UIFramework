#include "pch.h"
#include "UISVGResource.h"
#include "UIDefaultColor.h"

#include <Shlwapi.h> // for SHCreateStreamOnFileW

#pragma comment(lib, "Shlwapi") // for SHCreateStreamOnFileW

UISVGResource::~UISVGResource()
{
	ReleaseResources();
}

void UISVGResource::ReleaseResources()
{
	m_viewBoxWidth = 0.0f;
	m_viewBoxHeight = 0.0f;

	SafeRelease(m_fillPaint);
	SafeRelease(m_svg);
}

bool UISVGResource::Load(ID2D1DeviceContext5* d2dContext, const wchar_t* path)
{
	if (!d2dContext || !path)
		return false;

	ReleaseResources();

	IStream* stream = nullptr;
	HRESULT hr = ::SHCreateStreamOnFileW(path, STGM_READ, &stream);
	if (FAILED(hr))
		return false;

	hr = d2dContext->CreateSvgDocument(
		stream,
		D2D1::SizeF(m_defaultViewBoxValue, m_defaultViewBoxValue), // icon 기준 viewBox 초기값
		&m_svg);

	SafeRelease(stream);

	if (FAILED(hr))
	{
		ReleaseResources();
		return false;
	}

	ID2D1SvgElement* root = nullptr;
	m_svg->GetRoot(&root);
	if (root)
	{
		// View Box
		if (!InitializeViewBox(root))
		{
			SafeRelease(root);
			ReleaseResources();
			return false;
		}

		// Fill Color
		if (!InitializeSvgFillColor(root, UIDefaultColor::iconNormalColor.ToD2DColor()))
		{
			SafeRelease(root);
			ReleaseResources();
			return false;
		}

		SafeRelease(root);
	}

	//m_svg->SetViewportSize(D2D1_SIZE_F(m_viewBoxWidth, m_viewBoxHeight));

	return true;
}

void UISVGResource::Render(ID2D1DeviceContext5* d2dContext)
{
	if (d2dContext && m_svg)
	{
		d2dContext->DrawSvgDocument(m_svg);
	}
}

void UISVGResource::SetColor(const D2D1_COLOR_F& color)
{
	if (!m_fillPaint || !m_svg)
		return;

	m_fillPaint->SetColor(color);
}

bool UISVGResource::InitializeViewBox(ID2D1SvgElement* root)
{
	if (!root || !m_svg)
		return false;

	D2D1_SVG_VIEWBOX viewBox = {};

	HRESULT hr = root->GetAttributeValue(
		L"viewBox",
		D2D1_SVG_ATTRIBUTE_POD_TYPE_VIEWBOX,
		&viewBox,
		sizeof(viewBox));

	if (SUCCEEDED(hr))
	{
		m_viewBoxWidth = viewBox.width;
		m_viewBoxHeight = viewBox.height;
	}
	else
	{
		m_viewBoxWidth = m_defaultViewBoxValue;
		m_viewBoxHeight = m_defaultViewBoxValue;
	}

	return true;
}

bool UISVGResource::InitializeSvgFillColor(ID2D1SvgElement* root, const D2D1_COLOR_F& color)
{
	if (!root || !m_svg)
		return false;

	if (m_fillPaint)
	{
		SafeRelease(m_fillPaint);
	}

	HRESULT hr = m_svg->CreatePaint(
		D2D1_SVG_PAINT_TYPE_COLOR,
		color,
		nullptr,
		&m_fillPaint);

	if (FAILED(hr))
		return false;

	// fill 강제 설정 (없어도 생성됨)
	root->SetAttributeValue(L"fill", m_fillPaint);

	m_fillPaint->SetColor(color);

	return true;
}
