// 내장 아이콘 미리보기.
//
// UIIconRenderer 를 WIC 비트맵 타깃에 그려 PNG 로 저장한다. D3D 디바이스가
// 필요 없다 — 아이콘이 순수 D2D 프리미티브라서 그렇고, 그 자체가 SVG 경로가
// ID2D1DeviceContext5 를 요구하던 것과 달라진 점이다.
//
// 빌드: Test/icon_preview.bat

#include <Windows.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <wincodec.h>
#include <stdio.h>

#include "../D3D11UIFramework/Resource/UIIconRenderer.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace
{
	// 뷰어의 실제 색.
	const D2D1_COLOR_F kPanelBg    = { 0.125f, 0.125f, 0.125f, 1.0f };  // 32,32,32
	const D2D1_COLOR_F kHoverBg    = { 0.161f, 0.161f, 0.161f, 1.0f };  // 41,41,41
	const D2D1_COLOR_F kMenuBg     = { 0.180f, 0.180f, 0.180f, 1.0f };  // 46,46,46
	const D2D1_COLOR_F kIconNormal = { 0.780f, 0.780f, 0.780f, 1.0f };
	const D2D1_COLOR_F kIconHover  = { 1.000f, 1.000f, 1.000f, 1.0f };

	struct Entry
	{
		UIIconShape shape;
		const char* name;
	};

	const Entry kIcons[] = {
		{ UIIconShape::ZoomIn,          "ZoomIn" },
		{ UIIconShape::ZoomOut,         "ZoomOut" },
		{ UIIconShape::ZoomOneToOne,    "ZoomOneToOne" },
		{ UIIconShape::ZoomFit,         "ZoomFit" },
		{ UIIconShape::MeasureDistance, "MeasureDistance" },
		{ UIIconShape::Lut,             "Lut" },
		{ UIIconShape::Check,           "Check" },
		{ UIIconShape::ChevronRight,    "ChevronRight" },
		{ UIIconShape::Cursor,          "Cursor" },
		{ UIIconShape::Palette,         "Palette" },
		{ UIIconShape::ImageSize,       "ImageSize" },
	};

	constexpr int kCount = _countof(kIcons);
}

int main()
{
	::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	ID2D1Factory* d2dFactory = nullptr;
	if (FAILED(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory)))
	{
		printf("D2D1CreateFactory 실패\n");
		return 1;
	}

	IWICImagingFactory* wic = nullptr;
	if (FAILED(::CoCreateInstance(CLSID_WICImagingFactory, nullptr,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic))))
	{
		printf("WIC 팩토리 실패\n");
		return 1;
	}

	// 레이아웃
	//  행 1: 툴바 실제 크기 (40px 버튼 안 24px 아이콘)
	//  행 2: 컨텍스트 메뉴 실제 크기 (18px)
	//  행 3: 4배 확대 (96px) — 모양 확인용
	constexpr int kCell = 120;
	constexpr int kPad = 16;
	const int width = kPad * 2 + kCell * kCount;
	const int height = 420;

	IWICBitmap* bitmap = nullptr;
	if (FAILED(wic->CreateBitmap(width, height, GUID_WICPixelFormat32bppPBGRA,
		WICBitmapCacheOnLoad, &bitmap)))
	{
		printf("비트맵 생성 실패\n");
		return 1;
	}

	D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	ID2D1RenderTarget* rt = nullptr;
	if (FAILED(d2dFactory->CreateWicBitmapRenderTarget(bitmap, rtProps, &rt)))
	{
		printf("렌더 타깃 생성 실패\n");
		return 1;
	}

	UIIconRenderer icons;
	if (!icons.Initialize(rt))
	{
		printf("UIIconRenderer::Initialize 실패\n");
		return 1;
	}

	ID2D1SolidColorBrush* bg = nullptr;
	rt->CreateSolidColorBrush(kPanelBg, &bg);

	rt->BeginDraw();
	rt->Clear(D2D1::ColorF(0.08f, 0.08f, 0.08f, 1.0f));

	for (int i = 0; i < kCount; ++i)
	{
		const float cx = static_cast<float>(kPad + kCell * i);

		// ── 행 1: 툴바 버튼 40px, 아이콘 24px, hover 배경
		{
			const float bx = cx + (kCell - 40) * 0.5f;
			const float by = 30.0f;

			bg->SetColor(kHoverBg);
			rt->FillRoundedRectangle(
				D2D1::RoundedRect(D2D1::RectF(bx, by, bx + 40, by + 40), 4.0f, 4.0f), bg);

			icons.Draw(rt, kIcons[i].shape,
				D2D1::RectF(bx + 8, by + 8, bx + 32, by + 32), kIconHover);
		}

		// ── 행 2: 컨텍스트 메뉴 18px, 메뉴 배경
		{
			const float bx = cx + (kCell - 30) * 0.5f;
			const float by = 100.0f;

			bg->SetColor(kMenuBg);
			rt->FillRectangle(D2D1::RectF(bx - 8, by, bx + 38, by + 30), bg);

			icons.Draw(rt, kIcons[i].shape,
				D2D1::RectF(bx + 6, by + 6, bx + 24, by + 24), kIconNormal);
		}

		// ── 행 3: 4배 확대 96px
		{
			const float bx = cx + (kCell - 96) * 0.5f;
			const float by = 160.0f;

			bg->SetColor(kPanelBg);
			rt->FillRectangle(D2D1::RectF(bx - 4, by - 4, bx + 100, by + 100), bg);

			icons.Draw(rt, kIcons[i].shape,
				D2D1::RectF(bx, by, bx + 96, by + 96), kIconNormal);
		}

		// ── 행 4: 아주 작은 크기 14px — 뭉개지는지 확인
		{
			const float bx = cx + (kCell - 14) * 0.5f;
			const float by = 290.0f;

			icons.Draw(rt, kIcons[i].shape,
				D2D1::RectF(bx, by, bx + 14, by + 14), kIconNormal);
		}

		// ── 행 5: 흰 배경 (밝은 테마 확인)
		{
			const float bx = cx + (kCell - 40) * 0.5f;
			const float by = 330.0f;

			bg->SetColor(D2D1::ColorF(0.93f, 0.93f, 0.93f, 1.0f));
			rt->FillRoundedRectangle(
				D2D1::RoundedRect(D2D1::RectF(bx, by, bx + 40, by + 40), 4.0f, 4.0f), bg);

			icons.Draw(rt, kIcons[i].shape,
				D2D1::RectF(bx + 8, by + 8, bx + 32, by + 32),
				D2D1::ColorF(0.15f, 0.15f, 0.15f, 1.0f));
		}
	}

	const HRESULT hr = rt->EndDraw();
	if (FAILED(hr))
	{
		printf("EndDraw 실패 0x%08lX\n", hr);
		return 1;
	}

	// PNG 로 저장
	wchar_t outPath[MAX_PATH] = {};
	::GetCurrentDirectoryW(MAX_PATH, outPath);
	wcscat_s(outPath, L"\\icon_preview.png");

	IWICStream* stream = nullptr;
	IWICBitmapEncoder* encoder = nullptr;
	IWICBitmapFrameEncode* frame = nullptr;

	wic->CreateStream(&stream);
	stream->InitializeFromFilename(outPath, GENERIC_WRITE);
	wic->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
	encoder->Initialize(stream, WICBitmapEncoderNoCache);
	encoder->CreateNewFrame(&frame, nullptr);
	frame->Initialize(nullptr);
	frame->WriteSource(bitmap, nullptr);
	frame->Commit();
	encoder->Commit();

	wprintf(L"저장: %s\n", outPath);
	printf("아이콘 %d개\n", kCount);
	printf("행1=툴바 24px / 행2=메뉴 18px / 행3=4배 96px / 행4=14px / 행5=밝은 배경\n");
	printf("순서: ");
	for (int i = 0; i < kCount; ++i) printf("%s ", kIcons[i].name);
	printf("\n");

	frame->Release(); encoder->Release(); stream->Release();
	icons.Shutdown();
	bg->Release(); rt->Release(); bitmap->Release();
	wic->Release(); d2dFactory->Release();
	::CoUninitialize();

	return 0;
}
