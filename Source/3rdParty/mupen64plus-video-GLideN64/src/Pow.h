#pragma once
#include "Types.h"

constexpr u32 twopow(u32 dim) noexcept
{
	if (dim == 0) return 0;

	return (1 << dim);
}

constexpr u32 pow2(u32 dim) noexcept
{
	if (dim == 0) return 0;
	if (dim >= 0x80000000) return 0x80000000;

	u32 i = 1;

	while (i < dim) i <<= 1;

	return i;
}

constexpr u32 powof(u32 dim) noexcept
{
	if (dim == 0) return 0;
	if (dim >= 0x80000000) return 0x1F;

	u32 num = 2;
	u32 i = 1;

	while (num < dim) {
		num <<= 1;
		i++;
	}

	return i;
}
