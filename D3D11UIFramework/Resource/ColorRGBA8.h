#pragma once
#include <stdint.h>
#include <d2d1helper.h> 

struct ColorRGBA8
{
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 255;

	constexpr bool IsTransparent() const noexcept
	{
		return a == 0;
	}

	constexpr bool IsOpaque() const noexcept
	{
		return a == 255;
	}

	constexpr D2D1_COLOR_F ToD2DColor() const noexcept
	{
		constexpr float inv = 1.0f / 255.0f;

		return {
			r * inv,
			g * inv,
			b * inv,
			a * inv
		};
	}

	constexpr void ToFloat4(float out[4]) const noexcept
	{
		constexpr float inv = 1.0f / 255.0f;
		out[0] = r * inv;
		out[1] = g * inv;
		out[2] = b * inv;
		out[3] = a * inv;
	}
};