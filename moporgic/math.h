#pragma once
#define MOPORGIC_MATH_H_
/*
 * mopomath.h
 * moporgic/math.h
 *
 *  Created on: 2015/07/09
 *      Author: moporgic
 */

#include "half.h"
#include "type.h"
#include <math.h>
#include <cmath>
#include <numeric>
#include <iterator>
#include <algorithm>
#include <x86intrin.h>

namespace moporgic {

namespace math {
/*	reference:  The Aggregate Magic Algorithms
 *
 *	@techreport{magicalgorithms,
 *	author={Henry Gordon Dietz},
 *	title={{The Aggregate Magic Algorithms}},
 *	institution={University of Kentucky},
 *	howpublished={Aggregate.Org online technical report},
 *	URL={http://aggregate.org/MAGIC/}
 *	}
 *
 */

/**
 * Absolute Value of a Float
 * IEEE floating point uses an explicit sign bit,
 * so the absolute value can be taken by a bitwise AND with the complement of the sign bit.
 * For IA32 32-bit, the sign bit is an int value of 0x80000000,
 * for IA32 64-bit, the sign bit is the long long int value 0x8000000000000000. Of course,
 * if you prefer to use just int values, the IA32 64-bit sign bit is 0x80000000 at an int address offset of +1.
 */
static inline constexpr float abs(register float x) noexcept  { raw_cast<uint32_t>(x) &= 0x7fffffffu; return x; }
static inline constexpr float abs(register double x) noexcept { raw_cast<uint32_t, 1>(x) &= 0x7fffffffu; return x; }

/**
 * Absolute Value of an integer
 */
static inline constexpr uint32_t abs(register int32_t x) noexcept { return x & 0x80000000u ? ~x + 1 : x; }
static inline constexpr uint64_t abs(register int64_t x) noexcept { return x & 0x8000000000000000ull ? ~x + 1 : x; }

/**
 * Integer Selection
 * A branchless, lookup-free, alternative to code like if (a<b) x=c; else x=d; is ((((a-b) >> (WORDBITS-1)) & (c^d)) ^ d).
 * This code assumes that the shift is signed, which, of course, C does not promise.
 */
static inline constexpr int32_t select_lt(register int32_t a, register int32_t b, register int32_t c, register int32_t d) noexcept {
	return ((((a-b) >> (32-1)) & (c^d)) ^ d);
}
static inline constexpr int64_t select_lt(register int64_t a, register int64_t b, register int64_t c, register int64_t d) noexcept {
	return ((((a-b) >> (64-1)) & (c^d)) ^ d);
}

/**
 * Comparison of Float Values
 * IEEE floating point has a number of nice properties,
 * including the ability to use 2's complement integer comparisons to compare floating point values,
 * provided the native byte order is consistent between float and integer values.
 * The only complication is the use of sign+magnitude representation in floats.
 * The AMD Athlon Processor x86 Code Optimization Guide gives a nice summary on Page 43. Here's a set of C routines that embody the same logic:
 */
static inline constexpr bool isless0(register float f) noexcept          { return raw_cast<uint32_t>(f) > 0x80000000u; }
static inline constexpr bool islessequal0(register float f) noexcept     { return raw_cast<int32_t>(f) <= 0; }
static inline constexpr bool isgreater0(register float f) noexcept       { return raw_cast<int32_t>(f) > 0; }
static inline constexpr bool isgreaterequal0(register float f) noexcept  { return raw_cast<uint32_t>(f) <= 0x80000000u; }

static inline constexpr bool isless0(register double f) noexcept         { return raw_cast<uint32_t, 1>(f) > 0x80000000u; }
static inline constexpr bool islessequal0(register double f) noexcept    { return raw_cast<int32_t, 1>(f) <= 0; }
static inline constexpr bool isgreater0(register double f) noexcept      { return raw_cast<int32_t, 1>(f) > 0; }
static inline constexpr bool isgreaterequal0(register double f) noexcept { return raw_cast<uint32_t, 1>(f) <= 0x80000000u; }

/**
 * Integer Minimum or Maximum
 * Given 2's complement integer values x and y, the minimum can be computed without any branches as x+(((y-x)>>(WORDBITS-1))&(y-x)).
 * Logically, this works because the shift by (WORDBITS-1) replicates the sign bit to create a mask -- be aware,
 * however, that the C language does not require that shifts are signed even if their operands are signed, so there is a potential portability problem.
 * Additionally, one might think that a shift by any number greater than or equal to WORDBITS would have the same effect,
 * but many instruction sets have shifts that behave strangely when such shift distances are specified.
 *
 * Of course, maximum can be computed using the same trick: x-(((x-y)>>(WORDBITS-1))&(x-y)).
 *
 * Actually, the Integer Selection coding trick is just as efficient in encoding minimum and maximum....
 *
 */
static inline constexpr uint32_t max(register uint32_t x, register uint32_t y) noexcept {
	return x-(((x-y)>>(32-1))&(x-y));
//	return select_lt(x, y, y, x);
}
static inline constexpr uint64_t max(register uint64_t x, register uint64_t y) noexcept {
	return x-(((x-y)>>(64-1))&(x-y));
//	return select_lt(x, y, y, x);
}
static inline constexpr uint32_t min(register uint32_t x, register uint32_t y) noexcept {
	return x+(((y-x)>>(32-1))&(y-x));
//	return select_lt(x, y, x, y);
}
static inline constexpr uint64_t min(register uint64_t x, register uint64_t y) noexcept {
	return x+(((y-x)>>(64-1))&(y-x));
//	return select_lt(x, y, x, y);
}

/**
 * Average of Integers
 * This is actually an extension of the "well known" fact that for binary integer values x and y, (x+y) equals ((x&y)+(x|y)) equals ((x^y)+2*(x&y)).
 * Given two integer values x and y, the (floor of the) average normally would be computed by (x+y)/2; unfortunately,
 * this can yield incorrect results due to overflow. A very sneaky alternative is to use (x&y)+((x^y)/2).
 * If we are aware of the potential non-portability due to the fact that C does not specify if shifts are signed, this can be simplified to (x&y)+((x^y)>>1).
 * In either case, the benefit is that this code sequence cannot overflow.
 */
static inline constexpr uint32_t avg(register uint32_t x, register uint32_t y) noexcept { return (x&y)+((x^y)>>1); }
static inline constexpr uint64_t avg(register uint64_t x, register uint64_t y) noexcept { return (x&y)+((x^y)>>1); }
static inline constexpr int32_t avg(register int32_t x, register int32_t y) noexcept    { return (x&y)+((x^y)>>1); }
static inline constexpr int64_t avg(register int64_t x, register int64_t y) noexcept    { return (x&y)+((x^y)>>1); }

/**
 * Bit Reversal
 * Reversing the bits in an integer x is somewhat painful, but here's a SWAR algorithm for a 32-bit value
 */
static inline constexpr uint32_t reverse(register uint32_t x) noexcept {
	x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
	x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
	x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
	x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
	return ((x >> 16) | (x << 16));
}
static inline constexpr uint64_t reverse(register uint64_t x) noexcept {
	x = (((x & 0xaaaaaaaaaaaaaaaaULL) >> 1) | ((x & 0x5555555555555555ULL) << 1));
	x = (((x & 0xccccccccccccccccULL) >> 2) | ((x & 0x3333333333333333ULL) << 2));
	x = (((x & 0xf0f0f0f0f0f0f0f0ULL) >> 4) | ((x & 0x0f0f0f0f0f0f0f0fULL) << 4));
	x = (((x & 0xff00ff00ff00ff00ULL) >> 8) | ((x & 0x00ff00ff00ff00ffULL) << 8));
	x = (((x & 0xffff0000ffff0000ULL) >>16) | ((x & 0x0000ffff0000ffffULL) <<16));
	return ((x >> 32) | (x << 32));
}
/**
 * Bit Reversal (re-write Bit Reversal algorithm to use 4 instead of 8 constants)
 * Reversing the bits in an integer x is somewhat painful, but here's a SWAR algorithm for a 32-bit value
 */
//static inline constexpr uint32_t reverse_v2(register uint32_t x) noexcept {
//	register unsigned int y = 0x55555555;
//	x = (((x >> 1) & y) | ((x & y) << 1));
//	y = 0x33333333;
//	x = (((x >> 2) & y) | ((x & y) << 2));
//	y = 0x0f0f0f0f;
//	x = (((x >> 4) & y) | ((x & y) << 4));
//	y = 0x00ff00ff;
//	x = (((x >> 8) & y) | ((x & y) << 8));
//	return ((x >> 16) | (x << 16));
//}

/**
 * instruction based count: popcnt, lzcnt, tzcnt
 * to optimize these functions, enable compile flags -mabm -mbmi -mbmi2
 */
static inline constexpr uint64_t popcnt64(register uint64_t x) noexcept { return __builtin_popcountll(x); }
static inline constexpr uint32_t popcnt32(register uint32_t x) noexcept { return __builtin_popcount(x); }
static inline constexpr uint64_t popcnt(register uint64_t x) noexcept { return popcnt64(x); }
static inline constexpr uint32_t popcnt(register uint32_t x) noexcept { return popcnt32(x); }

static inline constexpr uint32_t lzcnt64(register uint64_t x) noexcept { return __builtin_clzll(x); }
static inline constexpr uint32_t lzcnt32(register uint32_t x) noexcept { return __builtin_clz(x); }
static inline constexpr uint32_t lzcnt(register uint64_t x) noexcept { return lzcnt64(x); }
static inline constexpr uint32_t lzcnt(register uint32_t x) noexcept { return lzcnt32(x); }

static inline constexpr uint32_t tzcnt64(register uint64_t x) noexcept { return __builtin_ctzll(x); }
static inline constexpr uint32_t tzcnt32(register uint32_t x) noexcept { return __builtin_ctz(x); }
static inline constexpr uint32_t tzcnt(register uint64_t x) noexcept { return tzcnt64(x); }
static inline constexpr uint32_t tzcnt(register uint32_t x) noexcept { return tzcnt32(x); }

/**
 * Population Count (Ones Count)
 * The population count of a binary integer value x is the number of one bits in the value.
 * Although many machines have single instructions for this,
 * the single instructions are usually microcoded loops that test a bit per cycle;
 * a log-time algorithm coded in C is often faster.
 * The following code uses a variable-precision SWAR algorithm to perform a tree
 * reduction adding the bits in a 32-bit value:
 */
static inline constexpr uint32_t ones32(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return popcnt32(x);
#else
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0f0f0f0f);
	x += (x >> 8);
	x += (x >> 16);
	return (x & 0x0000003f);
#endif
}

static inline constexpr uint32_t ones64(register uint64_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return popcnt64(x);
#else
	x = (x & 0x5555555555555555ull) + ((x >> 1) & 0x5555555555555555ull);
	x = (x & 0x3333333333333333ull) + ((x >> 2) & 0x3333333333333333ull);
	return (((x + (x >> 4)) & 0x0f0f0f0f0f0f0f0full) * 0x0101010101010101ull) >> 56;
#endif
}

static inline constexpr uint32_t ones16(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return popcnt32(x);
#else
	x -= ((x >> 1) & 0x5555);
	x = (((x >> 2) & 0x3333) + (x & 0x3333));
	x = (((x >> 4) + x) & 0x0f0f);
	x += (x >> 8);
	return (x & 0x001f);
#endif
}

static inline constexpr uint32_t ones8(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return popcnt32(x);
#else
	x -= ((x >> 1) & 0x5555);
	x = (((x >> 2) & 0x3333) + (x & 0x3333));
	x = (((x >> 4) + x) & 0x0f0f);
	return (x & 0x000f);
#endif
}

static inline constexpr uint32_t ones4(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return popcnt32(x);
#else
	x = ((x & 0b1010) >> 1) + (x & 0b0101);
	return (((x & 0b1100) >> 2) + (x & 0b0011)) & 0b0111;
#endif
}

static inline constexpr uint32_t ones(register uint32_t x) noexcept { return ones32(x); }
static inline constexpr uint32_t ones(register uint64_t x) noexcept { return ones64(x); }
static inline constexpr uint32_t ones(register uint16_t x) noexcept { return ones16(x); }

/**
 * Leading Zero Count
 * Some machines have had single instructions that count the number of leading zero bits in an integer;
 * such an operation can be an artifact of having floating point normalization hardware around. Clearly,
 * floor of base 2 log of x is (WORDBITS-lzc(x)). In any case, this operation has found its way
 * into quite a few algorithms, so it is useful to have an efficient implementation:
 */
static inline constexpr uint32_t lzc32(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return lzcnt32(x);
#else
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return (32 - ones32(x));
#endif
}
static inline constexpr uint32_t lzc64(register uint64_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return lzcnt64(x);
#else
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x |= (x >> 32);
	return (64 - ones64(x));
#endif
}
static inline constexpr uint32_t lzc16(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return lzcnt32(x);
#else
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	return (16 - ones16(x));
#endif
}
static inline constexpr uint32_t lzc(register uint32_t x) noexcept { return lzc32(x); }
static inline constexpr uint32_t lzc(register uint64_t x) noexcept { return lzc64(x); }
static inline constexpr uint32_t lzc(register uint16_t x) noexcept { return lzc16(x); }

/**
 * Trailing Zero Count
 * Given the Least Significant 1 Bit and Population Count (Ones Count) algorithms,
 * it is trivial to combine them to construct a trailing zero count (as pointed-out by Joe Bowbeer):
 */
static inline constexpr uint32_t tzc32(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return tzcnt32(x);
#else
	return (ones32((x & -x) - 1));
#endif
}
static inline constexpr uint32_t tzc64(register uint64_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return tzcnt64(x);
#else
	return (ones64((x & -x) - 1));
#endif
}
static inline constexpr uint32_t tzc16(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return tzcnt32(x);
#else
	return (ones16((x & -x) - 1));
#endif
}
static inline constexpr uint32_t tzc(register uint32_t x) noexcept { return tzc32(x); }
static inline constexpr uint32_t tzc(register uint64_t x) noexcept { return tzc64(x); }
static inline constexpr uint32_t tzc(register uint16_t x) noexcept { return tzc16(x); }

/**
 * Least Significant 1 Bit
 * This can be useful for extracting the lowest numbered element of a bit set.
 * Given a 2's complement binary integer value x, (x&-x) is the least significant 1 bit.
 * (This was pointed-out by Tom May.) The reason this works is that it is equivalent to (x & ((~x) + 1));
 *  any trailing zero bits in x become ones in ~x, adding 1 to that carries into the following bit,
 * and AND with x yields only the flipped bit... the original position of the least significant 1 bit.
 * Alternatively, since (x&(x-1)) is actually x stripped of its least significant 1 bit,
 * the least significant 1 bit is also (x^(x&(x-1))).
 *
 */
static inline constexpr uint32_t lsb32(register uint32_t x) noexcept {
	return (x & -x);
//	return (x ^ (x & (x - 1)));
}
static inline constexpr uint64_t lsb64(register uint64_t x) noexcept {
	return (x & -x);
//	return (x ^ (x & (x - 1)));
}
static inline constexpr uint32_t lsb(register uint32_t x) noexcept { return lsb32(x); }
static inline constexpr uint64_t lsb(register uint64_t x) noexcept { return lsb64(x); }

/**
 * Most Significant 1 Bit
 * Given a binary integer value x, the most significant 1 bit
 * (highest numbered element of a bit set) can be computed using a SWAR algorithm that recursively "folds"
 * the upper bits into the lower bits. This process yields a bit vector with
 * the same most significant 1 as x, but all 1's below it. Bitwise AND of the original value with
 * the complement of the "folded" value shifted down by one yields the most significant bit. For a 32-bit value:
 */
static inline constexpr uint32_t msb32(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	// note that lzcnt output 32 for 0
	// a 32-bit shifting causes 0x80000000u unchanged, thus we use 64-bit shifting to avoid this
	return 0x80000000ull >> lzcnt32(x);
#else
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return (x & ~(x >> 1));
#endif
}
static inline constexpr uint64_t msb64(register uint64_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	// the same reason as msb32, while we use xor to mask out bad value when x == 0
	// it should be compiled to cmove when optimization is on
	register uint64_t msb = 0x8000000000000000ull >> lzcnt64(x);
	return msb ^ (x ? 0ull : msb);
#else
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x |= (x >> 32);
	return (x & ~(x >> 1));
#endif
}
static inline constexpr uint32_t msb16(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return msb32(x);
#else
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	return (x & ~(x >> 1));
#endif
}
static inline constexpr uint32_t msb(register uint32_t x) noexcept { return msb32(x); }
static inline constexpr uint64_t msb(register uint64_t x) noexcept { return msb64(x); }
static inline constexpr uint16_t msb(register uint16_t x) noexcept { return msb16(x); }


/**
 * Log2 of an Integer
 * Given a binary integer value x, the floor of the base 2 log of that number efficiently
 * can be computed by the application of two variable-precision SWAR algorithms.
 * The first "folds" the upper bits into the lower bits to construct a bit vector with the
 * same most significant 1 as x, but all 1's below it. The second SWAR algorithm is population count,
 * defined elsewhere in this document. However, we must consider the issue of what the log2(0) should be;
 * the log of 0 is undefined, so how that value should be handled is unclear.
 * The following code for handling a 32-bit value gives two options:
 * This code returns -1 for log2(0); otherwise if LOG0DEFINED, it returns 0 for log2(0). For a 32-bit value:
 *
 */
static inline constexpr uint32_t lg32(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
#ifndef	LOG0DEFINED
	return 31 - lzcnt32(x);
#else
	return 31 - lzcnt32(x | 1);
#endif
#else // PREFER_GENERAL_INSTRUCTIONS
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
#ifndef	LOG0DEFINED
	return (ones32(x) - 1);
#else
	return (ones32(x >> 1));
#endif
#endif // PREFER_GENERAL_INSTRUCTIONS
}
static inline constexpr uint32_t lg64(register uint64_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
#ifndef	LOG0DEFINED
	return 63 - lzcnt64(x);
#else
	return 63 - lzcnt64(x | 1);
#endif
#else // PREFER_GENERAL_INSTRUCTIONS
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x |= (x >> 32);
#ifndef	LOG0DEFINED
	return (ones64(x) - 1);
#else
	return (ones64(x >> 1));
#endif
#endif // PREFER_GENERAL_INSTRUCTIONS
}
static inline constexpr uint32_t lg16(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return lg32(x);
#else // PREFER_GENERAL_INSTRUCTIONS
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
#ifndef	LOG0DEFINED
	return (ones16(x) - 1);
#else
	return (ones16(x >> 1));
#endif
#endif // PREFER_GENERAL_INSTRUCTIONS
}
static inline constexpr uint32_t lg8(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return lg32(x);
#else // PREFER_GENERAL_INSTRUCTIONS
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
#ifndef	LOG0DEFINED
	return (ones8(x) - 1);
#else
	return (ones8(x >> 1));
#endif
#endif // PREFER_GENERAL_INSTRUCTIONS
}
static inline constexpr uint32_t lg4(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return lg32(x);
#else // PREFER_GENERAL_INSTRUCTIONS
	x |= (x >> 1);
	x |= (x >> 2);
#ifndef	LOG0DEFINED
	return (ones4(x) - 1);
#else
	return (ones4(x >> 1));
#endif
#endif // PREFER_GENERAL_INSTRUCTIONS
}

static inline constexpr uint32_t log2(register uint32_t x) noexcept { return lg32(x); }
static inline constexpr uint32_t log2(register uint64_t x) noexcept { return lg64(x); }
static inline constexpr uint32_t log2(register uint16_t x) noexcept { return lg16(x); }
static inline constexpr uint32_t lg(register uint32_t x) noexcept { return lg32(x); }
static inline constexpr uint32_t lg(register uint64_t x) noexcept { return lg64(x); }
static inline constexpr uint32_t lg(register uint16_t x) noexcept { return lg16(x); }

/**
 * Log2 of an Integer
 * Suppose instead that you want the ceiling of the base 2 log.
 * The floor and ceiling are identical if x is a power of two; otherwise,
 * the result is 1 too small. This can be corrected using the power of 2 test
 * followed with the comparison-to-mask shift used in integer minimum/maximum.
 * The result is:
 */
//static inline constexpr uint32_t log2_unstable(register uint32_t x) noexcept {
//	register int y = (x & (x - 1));
//
//	y |= -y;
//	y >>= (sizeof(unsigned int) - 1);
//	x |= (x >> 1);
//	x |= (x >> 2);
//	x |= (x >> 4);
//	x |= (x >> 8);
//	x |= (x >> 16);
//#ifndef	LOG0DEFINED
//	return (ones(x) - 1 - y);
//#else
//	return (ones32(x >> 1) - y);
//#endif
//}

/**
 * Is Power of 2
 * A non-negative binary integer value x is a power of 2 iff (x&(x-1)) is 0 using 2's complement arithmetic.
 */
static inline constexpr bool ispo2(register uint32_t x) noexcept { return (x & (x - 1)) == 0; }
static inline constexpr bool ispo2(register uint64_t x) noexcept { return (x & (x - 1)) == 0; }

/**
 * Next Largest Power of 2
 *  Given a binary integer value x, the next largest power of 2 can be computed
 *  by a SWAR algorithm that recursively "folds" the upper bits into the lower bits.
 *  This process yields a bit vector with the same most significant 1 as x, but all 1's below it.
 *  Adding 1 to that value yields the next largest power of 2. For a 32-bit value:
 */
static inline constexpr uint32_t nlpo2(register uint32_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	return 1ull << (32 - lzcnt32(x));
#else
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return (x + 1);
#endif
}
static inline constexpr uint64_t nlpo2(register uint64_t x) noexcept {
#ifndef PREFER_GENERAL_INSTRUCTIONS
	register uint32_t n = lzcnt64(x);
	return (n ? 1ull : 0) << (64 - n);
#else
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x |= (x >> 32);
	return (x + 1);
#endif
}

/**
 * Swap without temporary
 */
//template<typename T> static inline constexpr
//void swap(T& x, T& y) noexcept {
//// implementation V1:
//	x ^= y; /* x' = (x^y) */
//	y ^= x; /* y' = (y^(x^y)) = x */
//	x ^= y; /* x' = (x^y)^x = y */
//
//// implementation V2:
////	x = x + y; /* x' = (x+y) */
////	y = x - y; /* y' = (x+y)-y = x */
////	x = x - y; /* x' = (x+y)-x = y */
//}

/**
 * bit extraction
 * to use these functions, compile flag -mbmi is necessary
 */
static inline constexpr uint64_t bextr64(register uint64_t x, register uint32_t off, register uint32_t len) noexcept { // return _bextr_u64(x, off, len); }
	return __builtin_ia32_bextr_u64(x, (off & 0xff) | ((len & 0xff) << 8)); }
static inline constexpr uint32_t bextr32(register uint32_t x, register uint32_t off, register uint32_t len) noexcept { // return _bextr_u32(x, off, len); }
	return __builtin_ia32_bextr_u32(x, (off & 0xff) | ((len & 0xff) << 8)); }
static inline constexpr uint64_t bextr(register uint64_t x, register uint32_t off, register uint32_t len) noexcept { return bextr64(x, off, len); }
static inline constexpr uint32_t bextr(register uint32_t x, register uint32_t off, register uint32_t len) noexcept { return bextr32(x, off, len); }

/**
 * Parallel bit deposit and extract: PEXT and PDEP
 * to use these functions, compile flag -mbmi2 is necessary
 */
static inline constexpr uint64_t pdep64(register uint64_t a, register uint64_t mask) noexcept { // return _pdep_u64(a, mask); }
	return __builtin_ia32_pdep_di(a, mask); }
static inline constexpr uint32_t pdep32(register uint32_t a, register uint32_t mask) noexcept { // return _pdep_u32(a, mask); }
	return __builtin_ia32_pdep_si(a, mask); }
static inline constexpr uint64_t pdep(register uint64_t a, register uint64_t mask) noexcept { return pdep64(a, mask); }
static inline constexpr uint32_t pdep(register uint32_t a, register uint32_t mask) noexcept { return pdep32(a, mask); }
static inline constexpr uint64_t pext64(register uint64_t a, register uint64_t mask) noexcept { // return _pext_u64(a, mask); }
	return __builtin_ia32_pext_di(a, mask); }
static inline constexpr uint32_t pext32(register uint32_t a, register uint32_t mask) noexcept { // return _pext_u32(a, mask); }
	return __builtin_ia32_pext_si(a, mask); }
static inline constexpr uint64_t pext(register uint64_t a, register uint64_t mask) noexcept { return pext64(a, mask); }
static inline constexpr uint32_t pext(register uint32_t a, register uint32_t mask) noexcept { return pext32(a, mask); }

/**
 * return the lowest nth set bit (n starting from 0)
 * to use these functions, compile flag -mbmi2 is necessary
 * https://stackoverflow.com/questions/7669057/find-nth-set-bit-in-an-int/27453505#27453505
 */
static inline constexpr uint64_t nthset64(register uint64_t x, register uint32_t n) noexcept { return pdep64(1ULL << n, x); }
static inline constexpr uint32_t nthset32(register uint32_t x, register uint32_t n) noexcept { return pdep32(1U << n, x); }
static inline constexpr uint64_t nthset(register uint64_t x, register uint32_t n) noexcept { return nthset64(x, n); }
static inline constexpr uint32_t nthset(register uint32_t x, register uint32_t n) noexcept { return nthset32(x, n); }

/**
 * Tail recursive pow
 */
template<typename N, typename P> static inline constexpr
N pow_tail(N v, N b, P p) noexcept { return p ? pow_tail(v * b, b, p - 1) : v; }

template<typename N, typename P> static inline constexpr
N pow(N b, P p) noexcept { return pow_tail(N(1), b, p); }

/**
 * mean
 */
template<typename numeric, typename iter> static inline constexpr
numeric mean(iter first, iter last, numeric init = 0) noexcept {
	return numeric(std::accumulate(first, last, init)) / std::distance(first, last);
}

/**
 * standard deviation
 */
template<typename numeric, typename iter> static inline constexpr
numeric deviation(iter first, iter last, numeric mean) noexcept {
	numeric sumofsqr = 0;
	for (iter it = first; it != last; std::advance(it, 1)) sumofsqr += std::pow(*it - mean, 2);
	return std::sqrt(sumofsqr / std::distance(first, last));
}

/**
 * standard deviation
 */
template<typename numeric, typename iter> static inline constexpr
numeric deviation(iter first, iter last) noexcept {
	return deviation<numeric>(first, last, mean<numeric>(first, last));
}

// reference: https://github.com/aappleby/smhasher/wiki/MurmurHash3
//            https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp

/**
 * Finalization mix - force all bits of a hash block to avalanche
 */
static inline constexpr uint32_t fmix32(register uint32_t h) {
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

/**
 * Finalization mix - force all bits of a hash block to avalanche
 */
static inline constexpr uint64_t fmix64(register uint64_t h) {
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccdull;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53ull;
	h ^= h >> 33;
	return h;
}

} /* math */

/**
 * the full declaration of half is inside type.h
 * here defines the 16-bit floating point operations
 */
constexpr half::half(f32 num) noexcept : hf(to_half(num)) {}
constexpr half::operator f32() const noexcept { return to_float(hf); }
constexpr half half::operator +(half f) const noexcept { return half::as(half_add(hf, f.hf)); }
constexpr half half::operator -(half f) const noexcept { return half::as(half_sub(hf, f.hf)); }
constexpr half half::operator *(half f) const noexcept { return half::as(half_mul(hf, f.hf)); }
constexpr half half::operator /(half f) const noexcept { return half::as(half_div(hf, f.hf)); }

} /* moporgic */
