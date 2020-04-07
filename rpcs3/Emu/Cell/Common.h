#pragma once

#include "Utilities/types.h"
#include "Utilities/BEType.h"

// Floating-point rounding mode (for both PPU and SPU)
enum FPSCR_RN
{
	FPSCR_RN_NEAR = 0,
	FPSCR_RN_ZERO = 1,
	FPSCR_RN_PINF = 2,
	FPSCR_RN_MINF = 3,
};

// Get the exponent of a float
inline int fexpf(float x)
{
	return (std::bit_cast<u32>(x) >> 23) & 0xff;
}

namespace utils
{
	bool has_avx();
}

// Verify AVX availability for TSX transactions
static inline const bool s_tsx_avx = utils::has_avx();

// 128-byte of cache line data copy from reservation load
using rdata_128_t = std::byte[128];

static FORCE_INLINE bool cmp_rdata_avx(const __m256i* lhs, const __m256i* rhs)
{
#if defined(_MSC_VER) || defined(__AVX__)
	const __m256 x0 = _mm256_xor_ps(_mm256_castsi256_ps(_mm256_load_si256(lhs + 0)), _mm256_castsi256_ps(_mm256_load_si256(rhs + 0)));
	const __m256 x1 = _mm256_xor_ps(_mm256_castsi256_ps(_mm256_load_si256(lhs + 1)), _mm256_castsi256_ps(_mm256_load_si256(rhs + 1)));
	const __m256 x2 = _mm256_xor_ps(_mm256_castsi256_ps(_mm256_load_si256(lhs + 2)), _mm256_castsi256_ps(_mm256_load_si256(rhs + 2)));
	const __m256 x3 = _mm256_xor_ps(_mm256_castsi256_ps(_mm256_load_si256(lhs + 3)), _mm256_castsi256_ps(_mm256_load_si256(rhs + 3)));
	const __m256 c0 = _mm256_or_ps(x0, x1);
	const __m256 c1 = _mm256_or_ps(x2, x3);
	const __m256 c2 = _mm256_or_ps(c0, c1);
	return _mm256_testz_si256(_mm256_castps_si256(c2), _mm256_castps_si256(c2)) != 0;
#else
	bool result = 0;
	__asm__(
		"vmovaps 0*32(%[lhs]), %%ymm0;" // load
		"vmovaps 1*32(%[lhs]), %%ymm1;"
		"vmovaps 2*32(%[lhs]), %%ymm2;"
		"vmovaps 3*32(%[lhs]), %%ymm3;"
		"vxorps 0*32(%[rhs]), %%ymm0, %%ymm0;" // compare
		"vxorps 1*32(%[rhs]), %%ymm1, %%ymm1;"
		"vxorps 2*32(%[rhs]), %%ymm2, %%ymm2;"
		"vxorps 3*32(%[rhs]), %%ymm3, %%ymm3;"
		"vorps %%ymm0, %%ymm1, %%ymm0;" // merge
		"vorps %%ymm2, %%ymm3, %%ymm2;"
		"vorps %%ymm0, %%ymm2, %%ymm0;"
		"vptest %%ymm0, %%ymm0;" // test
		"vzeroupper"
		: "=@ccz" (result)
		: [lhs] "r" (lhs)
		, [rhs] "r" (rhs)
		: "cc" // Clobber flags
		, "xmm0" // Clobber registers ymm0-ymm3 (see mov_rdata_avx)
		, "xmm1"
		, "xmm2"
		, "xmm3"
	);
	return result;
#endif
}

static FORCE_INLINE bool cmp_rdata(const rdata_128_t& _lhs, const rdata_128_t& _rhs)
{
#ifndef __AVX__
	if (s_tsx_avx) [[likely]]
#endif
	{
		return cmp_rdata_avx(reinterpret_cast<const __m256i*>(_lhs), reinterpret_cast<const __m256i*>(_rhs));
	}

	// TODO: use std::assume_aligned
	const auto lhs = reinterpret_cast<const v128*>(_lhs);
	const auto rhs = reinterpret_cast<const v128*>(_rhs);
	const v128 a = (lhs[0] ^ rhs[0]) | (lhs[1] ^ rhs[1]);
	const v128 b = (lhs[2] ^ rhs[2]) | (lhs[3] ^ rhs[3]);
	const v128 c = (lhs[4] ^ rhs[4]) | (lhs[5] ^ rhs[5]);
	const v128 d = (lhs[6] ^ rhs[6]) | (lhs[7] ^ rhs[7]);
	const v128 r = (a | b) | (c | d);
	return r == v128{};
}

static FORCE_INLINE void mov_rdata_avx(__m256i* dst, const __m256i* src)
{
#if defined(_MSC_VER) || defined(__AVX2__)
	// In AVX-only mode, for some older CPU models, GCC/Clang may emit 128-bit loads/stores instead.
	_mm256_store_si256(dst + 0, _mm256_loadu_si256(src + 0));
	_mm256_store_si256(dst + 1, _mm256_loadu_si256(src + 1));
	_mm256_store_si256(dst + 2, _mm256_loadu_si256(src + 2));
	_mm256_store_si256(dst + 3, _mm256_loadu_si256(src + 3));
#else
	__asm__(
		"vmovdqu 0*32(%[src]), %%ymm0;" // load
		"vmovdqu %%ymm0, 0*32(%[dst]);" // store
		"vmovdqu 1*32(%[src]), %%ymm0;"
		"vmovdqu %%ymm0, 1*32(%[dst]);"
		"vmovdqu 2*32(%[src]), %%ymm0;"
		"vmovdqu %%ymm0, 2*32(%[dst]);"
		"vmovdqu 3*32(%[src]), %%ymm0;"
		"vmovdqu %%ymm0, 3*32(%[dst]);"
#ifndef __AVX__
		"vzeroupper" // Don't need in AVX mode (should be emitted automatically)
#endif
		:
		: [src] "r" (src)
		, [dst] "r" (dst)
#ifdef __AVX__
		: "ymm0" // Clobber ymm0 register (acknowledge its modification)
#else
		: "xmm0" // ymm0 is "unknown" if not compiled in AVX mode, so clobber xmm0 only
#endif
	);
#endif
}

static FORCE_INLINE void mov_rdata(rdata_128_t& dst, const rdata_128_t& src)
{
#ifndef __AVX__
	if (s_tsx_avx) [[likely]]
#endif
	{
		mov_rdata_avx(reinterpret_cast<__m256i*>(dst), reinterpret_cast<const __m256i*>(src));
		return;
	}

	// TODO: use std::assume_aligned
	std::memcpy(reinterpret_cast<v128*>(dst), reinterpret_cast<const v128*>(src), sizeof(dst));
}
