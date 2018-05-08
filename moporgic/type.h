#pragma once
#define MOPORGIC_TYPE_H_

/*
 *  mopotype.h type.h
 *  Created on: 2012/10/30
 *      Author: moporgic
 */

#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include <type_traits>

typedef int64_t  ll;
typedef uint64_t ull;

typedef int64_t  i64;
typedef uint64_t u64;
typedef int32_t  i32;
typedef uint32_t u32;
typedef int16_t  i16;
typedef uint16_t u16;
typedef int8_t   i8;
typedef uint8_t  u8;
//typedef char     byte;
struct  byte;

typedef __int128          i128;
typedef unsigned __int128 u128;
typedef unsigned char     uchar;
typedef unsigned long int ulint;

typedef short life; // life is short
typedef long double llf;
typedef long double quadruple;
typedef long double quadra;
typedef long double f128;
typedef double f64;
typedef float  f32;

typedef intptr_t  iptr;
typedef uintptr_t uptr;

#if !defined(__cplusplus) || __cplusplus < 201103L
#define constexpr
#define noexcept
#endif

template<typename dst, typename src> static inline constexpr
dst  cast(src ptr) noexcept { return (dst) ptr; /*return reinterpret_cast<dst>(p);*/ }

template<typename dst, typename src> static inline constexpr
dst* pointer_cast(src* ptr) noexcept { return cast<dst*>(ptr); }

template<typename dst, typename src> static inline constexpr
dst& reference_cast(src& ref) noexcept { return *(pointer_cast<dst>(&ref)); }

template<typename dst, int off = 0, typename src> static inline constexpr
dst& raw_cast(src& ref, int sh = 0) noexcept { return *(pointer_cast<dst>(&ref) + off + sh); }

#include <iostream>
#include <iterator>
#include <algorithm>
#include <utility>

struct byte {
	unsigned char v;
	constexpr byte(unsigned char v = 0) noexcept : v(v) {}
	constexpr operator unsigned char&() noexcept { return v; }
	constexpr operator unsigned char*() noexcept { return &v; }
	constexpr operator char*() noexcept { return cast<char*>(&v); }
	constexpr operator const unsigned char&() const noexcept { return v; }
	constexpr operator const unsigned char*() const noexcept { return &v; }
	constexpr operator const char*() const noexcept { return cast<char*>(&v); }
	friend std::ostream& operator <<(std::ostream& os, const byte& b) {
		register auto hi = (b.v >> 4) & 0xf;
		register auto lo = (b.v >> 0) & 0xf;
		os << cast<char>(hi < 10 ? hi + '0' : hi + 'a' - 10);
		os << cast<char>(lo < 10 ? lo + '0' : lo + 'a' - 10);
		return os;
	}
	friend std::istream& operator >>(std::istream& is, const byte& b) {
		register unsigned char hi, lo;
		is >> hi >> lo;
		hi = (hi & 15) + (hi >> 6) * 9;
		lo = (lo & 15) + (lo >> 6) * 9;
		if (is) const_cast<byte&>(b).v = (hi << 4) | lo;
		return is;
	}
};

struct u8c {
	u8 v;
	constexpr u8c(u32 b = 0) noexcept : v(b) {}
	constexpr u8c(const u8c& b) noexcept : v(b.v) {}
	constexpr u8c& operator =(u32 b) noexcept { v = b; return *this; }
	constexpr u8c& operator -=(u32 b) noexcept { v -= byte(b); return *this; }
	constexpr u8c& operator +=(u32 b) noexcept { v += byte(b); return *this; }
	constexpr u8c& operator <<=(u32 s) noexcept { v <<= s; return *this; }
	constexpr u8c& operator >>=(u32 s) noexcept { v >>= s; return *this; }
	constexpr u8c& operator &=(u32 b) noexcept { v &= byte(b); return *this; }
	constexpr u8c& operator |=(u32 b) noexcept { v |= byte(b); return *this; }
	constexpr bool operator ==(u32 b) const noexcept { return v == byte(b); }
	constexpr bool operator !=(u32 b) const noexcept { return v != byte(b); }
	constexpr bool operator <(u32 b) const noexcept { return v < byte(b); }
	constexpr bool operator >(u32 b) const noexcept { return v > byte(b); }
	constexpr bool operator <=(u32 b) const noexcept { return v <= byte(b); }
	constexpr bool operator >=(u32 b) const noexcept { return v >= byte(b); }
	constexpr bool operator !() const noexcept { return (v & 0xff) == 0; }
	constexpr u32 operator +(u32 b) const noexcept { return u32(v) + b; }
	constexpr u32 operator -(u32 b) const noexcept { return u32(v) - b; }
	constexpr u32 operator <<(u32 s) const noexcept { return u32(v) << s; }
	constexpr u32 operator >>(u32 s) const noexcept { return u32(v) >> s; }
	constexpr u32 operator &(u32 b) const noexcept { return u32(v) & b; }
	constexpr u32 operator |(u32 b) const noexcept { return u32(v) | b; }
	constexpr u32 operator ~() const noexcept { return ~u32(v); }
	constexpr u32 operator ++(int) noexcept { u32 b = v++; return b; }
	constexpr u32 operator --(int) noexcept { u32 b = v--; return b; }
	constexpr u32 operator ++() noexcept { return ++v; }
	constexpr u32 operator --() noexcept { return ++v; }
	constexpr operator u32() const noexcept { return v; }
	constexpr operator i32() const noexcept { return v; }
	constexpr operator bool() const noexcept { return (v & 0xff) != 0; }
	constexpr operator u8*() const noexcept { return cast<u8*>(&v); }
	constexpr operator i8*() const noexcept { return cast<i8*>(&v); }
	constexpr operator byte*() const noexcept { return cast<byte*>(&v); }
	constexpr operator const u8*() const noexcept { return cast<const u8*>(&v); }
	constexpr operator const i8*() const noexcept { return cast<const i8*>(&v); }
	constexpr operator const byte*() const noexcept { return cast<const byte*>(&v); }
};

namespace moporgic {
namespace endian {
static inline constexpr bool is_le() noexcept { register u32 v = 0x01; return raw_cast<byte, 0>(v); }
static inline constexpr bool is_be() noexcept { register u32 v = 0x01; return raw_cast<byte, 3>(v); }
template<typename T> static inline constexpr T repos(T v, int i, int p) noexcept { return ((v >> (i << 3)) & 0xff) << (p << 3); }
template<typename T> static inline constexpr T swpos(T v, int i, int p) noexcept { return repos(v, i, p) | repos(v, p, i); }
static inline constexpr u16 to_le(u16 v) noexcept { return is_le() ? v : swpos(v, 0, 1); }
static inline constexpr u32 to_le(u32 v) noexcept { return is_le() ? v : swpos(v, 0, 3) | swpos(v, 1, 2); }
static inline constexpr u64 to_le(u64 v) noexcept { return is_le() ? v : swpos(v, 0, 7) | swpos(v, 1, 6) | swpos(v, 2, 5) | swpos(v, 3, 4); }
static inline constexpr u16 to_be(u16 v) noexcept { return is_be() ? v : swpos(v, 0, 1); }
static inline constexpr u32 to_be(u32 v) noexcept { return is_be() ? v : swpos(v, 0, 3) | swpos(v, 1, 2); }
static inline constexpr u64 to_be(u64 v) noexcept { return is_be() ? v : swpos(v, 0, 7) | swpos(v, 1, 6) | swpos(v, 2, 5) | swpos(v, 3, 4); }
} // namespace endian
} // namespace moporgic

union r16 {
	u16 v_u16;
	u8c v_u8c[2];
	u8 v_u8[2];
	constexpr r16(const r16& v) noexcept : v_u16(v) {}
	constexpr r16(u16 v = 0) noexcept : v_u16(v) {}
	constexpr r16(i16 v) noexcept : v_u16(v) {}
	constexpr r16(u32 v) noexcept : v_u16(v) {}
	constexpr r16(i32 v) noexcept : v_u16(v) {}
	constexpr r16(u64 v) noexcept : v_u16(v) {}
	constexpr r16(i64 v) noexcept : v_u16(v) {}
	constexpr r16(u8 v) noexcept  : v_u16(v) {}
	constexpr r16(i8 v) noexcept  : v_u16(v) {}
	constexpr r16(u8* b) noexcept  : v_u16(*cast<u16*>(b)) {}
	constexpr r16(i8* b) noexcept  : v_u16(*cast<u16*>(b)) {}
	constexpr r16(byte* b) noexcept  : v_u16(*cast<u16*>(b)) {}
	constexpr r16(const u8* b) noexcept  : r16(const_cast<u8*>(b)) {}
	constexpr r16(const i8* b) noexcept  : r16(const_cast<i8*>(b)) {};
	constexpr r16(const byte* b) noexcept  : r16(cast<i8*>(b)) {};
	constexpr u8c& operator[](const int& i) noexcept { return v_u8c[i]; }
	constexpr operator u64() const noexcept { return v_u16; }
	constexpr operator i64() const noexcept { return v_u16; }
	constexpr operator f64() const noexcept { return v_u16; }
	constexpr operator u32() const noexcept { return v_u16; }
	constexpr operator i32() const noexcept { return v_u16; }
	constexpr operator f32() const noexcept { return v_u16; }
	constexpr operator u16() const noexcept { return v_u16; }
	constexpr operator i16() const noexcept { return v_u16; }
	constexpr operator u8() const noexcept { return v_u8[0]; }
	constexpr operator i8() const noexcept { return v_u8[0]; }
	constexpr operator bool() const noexcept { return static_cast<bool>(v_u16); }
	constexpr operator u8*() const noexcept { return cast<u8*>(v_u8); }
	constexpr operator i8*() const noexcept { return cast<i8*>(v_u8); }
	constexpr operator byte*() const noexcept { return cast<byte*>(v_u8); }
	constexpr operator const u8*() const noexcept { return cast<const u8*>(v_u8); }
	constexpr operator const i8*() const noexcept { return cast<const i8*>(v_u8); }
	constexpr operator const byte*() const noexcept { return cast<const byte*>(v_u8); }
	constexpr r16 le() const noexcept { return moporgic::endian::to_le(v_u16); }
	constexpr r16 be() const noexcept { return moporgic::endian::to_be(v_u16); }
};

union r32 {
	u32 v_u32;
	f32 v_f32;
	r16 v_r16[2];
	u8c v_u8c[4];
	u8 v_u8[4];
	constexpr r32(const r32& v) noexcept : v_u32(v) {}
	constexpr r32(u32 v = 0) noexcept : v_u32(v) {}
	constexpr r32(i32 v) noexcept : v_u32(v) {}
	constexpr r32(u16 v) noexcept : v_u32(v) {}
	constexpr r32(i16 v) noexcept : v_u32(v) {}
	constexpr r32(u64 v) noexcept : v_u32(v) {}
	constexpr r32(i64 v) noexcept : v_u32(v) {}
	constexpr r32(f32 v) noexcept : v_f32(v) {}
	constexpr r32(f64 v) noexcept : v_f32(v) {}
	constexpr r32(u8 v) noexcept  : v_u32(v) {}
	constexpr r32(i8 v) noexcept  : v_u32(v) {}
	constexpr r32(u8* b) noexcept  : v_u32(*cast<u32*>(b)) {}
	constexpr r32(i8* b) noexcept  : v_u32(*cast<u32*>(b)) {}
	constexpr r32(byte* b) noexcept  : v_u32(*cast<u32*>(b)) {}
	constexpr r32(const u8* b) noexcept  : r32(const_cast<u8*>(b)) {}
	constexpr r32(const i8* b) noexcept  : r32(const_cast<i8*>(b)) {}
	constexpr r32(const byte* b) noexcept  : r32(cast<i8*>(b)) {}
	constexpr u8c& operator[](const int& i) noexcept { return v_u8c[i]; }
	constexpr operator u64() const noexcept { return v_u32; }
	constexpr operator i64() const noexcept { return v_u32; }
	constexpr operator f64() const noexcept { return v_f32; }
	constexpr operator u32() const noexcept { return v_u32; }
	constexpr operator i32() const noexcept { return v_u32; }
	constexpr operator f32() const noexcept { return v_f32; }
	constexpr operator u16() const noexcept { return v_r16[0]; }
	constexpr operator i16() const noexcept { return v_r16[0]; }
	constexpr operator u8() const noexcept { return v_u8[0]; }
	constexpr operator i8() const noexcept { return v_u8[0]; }
	constexpr operator bool() const noexcept { return static_cast<bool>(v_u32); }
	constexpr operator u8*() const noexcept { return cast<u8*>(v_u8); }
	constexpr operator i8*() const noexcept { return cast<i8*>(v_u8); }
	constexpr operator byte*() const noexcept { return cast<byte*>(v_u8); }
	constexpr operator const u8*() const noexcept { return cast<const u8*>(v_u8); }
	constexpr operator const i8*() const noexcept { return cast<const i8*>(v_u8); }
	constexpr operator const byte*() const noexcept { return cast<const byte*>(v_u8); }
	constexpr r32 le() const noexcept { return moporgic::endian::to_le(v_u32); }
	constexpr r32 be() const noexcept { return moporgic::endian::to_be(v_u32); }
};

union r64 {
	u64 v_u64;
	f64 v_f64;
	r16 v_r16[4];
	r32 v_r32[2];
	u8c v_u8c[8];
	u8 v_u8[8];
	constexpr r64(const r64& v) noexcept : v_u64(v) {}
	constexpr r64(u64 v) noexcept : v_u64(v) {}
	constexpr r64(i64 v) noexcept : v_u64(v) {}
	constexpr r64(u32 v) noexcept : v_u64(v) {}
	constexpr r64(i32 v) noexcept : v_u64(v) {}
	constexpr r64(u16 v) noexcept : v_u64(v) {}
	constexpr r64(i16 v) noexcept : v_u64(v) {}
	constexpr r64(f32 v) noexcept : v_f64(v) {}
	constexpr r64(f64 v) noexcept : v_f64(v) {}
	constexpr r64(u8 v) noexcept  : v_u64(v) {}
	constexpr r64(i8 v) noexcept  : v_u64(v) {}
	constexpr r64(u8* b) noexcept  : v_u64(*cast<u64*>(b)) {}
	constexpr r64(i8* b) noexcept  : v_u64(*cast<u64*>(b)) {}
	constexpr r64(byte* b) noexcept  : v_u64(*cast<u64*>(b)) {}
	constexpr r64(const u8* b) noexcept  : r64(const_cast<u8*>(b)) {}
	constexpr r64(const i8* b) noexcept  : r64(const_cast<i8*>(b)) {}
	constexpr r64(const byte* b) noexcept  : r64(cast<i8*>(b)) {}
	constexpr u8c& operator[](const int& i) noexcept { return v_u8c[i]; }
	constexpr operator u64() const noexcept { return v_u64; }
	constexpr operator i64() const noexcept { return v_u64; }
	constexpr operator f64() const noexcept { return v_f64; }
	constexpr operator u32() const noexcept { return v_r32[0]; }
	constexpr operator i32() const noexcept { return v_r32[0]; }
	constexpr operator f32() const noexcept { return v_r32[0]; }
	constexpr operator u16() const noexcept { return v_r32[0]; }
	constexpr operator i16() const noexcept { return v_r32[0]; }
	constexpr operator u8() const noexcept { return v_u8[0]; }
	constexpr operator i8() const noexcept { return v_u8[0]; }
	constexpr operator bool() const noexcept { return static_cast<bool>(v_u64); }
	constexpr operator u8*() const noexcept { return cast<u8*>(v_u8); }
	constexpr operator i8*() const noexcept { return cast<i8*>(v_u8); }
	constexpr operator byte*() const noexcept { return cast<byte*>(v_u8); }
	constexpr operator const u8*() const noexcept { return cast<const u8*>(v_u8); }
	constexpr operator const i8*() const noexcept { return cast<const i8*>(v_u8); }
	constexpr operator const byte*() const noexcept { return cast<const byte*>(v_u8); }
	constexpr r64 le() const noexcept { return moporgic::endian::to_le(v_u64); }
	constexpr r64 be() const noexcept { return moporgic::endian::to_le(v_u64); }
};

namespace moporgic {

#define declare_enable_if_with(trait, name, detail...)\
template<typename test> using enable_if_##name = typename std::enable_if<std::is_##trait<detail>::value>::type;
#define declare_enable_if(trait)\
declare_enable_if_with(trait, trait, test);

declare_enable_if(arithmetic);
declare_enable_if(integral);
declare_enable_if(floating_point);
declare_enable_if(function);
declare_enable_if(fundamental);
declare_enable_if(reference);
declare_enable_if(object);
declare_enable_if(member_pointer);
declare_enable_if(pointer);
declare_enable_if(array);
declare_enable_if(scalar);
declare_enable_if(compound);
declare_enable_if(union);
declare_enable_if(class);
declare_enable_if(signed);
declare_enable_if(unsigned);

declare_enable_if_with(convertible, iterator_convertible, typename std::iterator_traits<test>::iterator_category, std::input_iterator_tag);

#define declare_trait(trait, detail...)\
template<> struct std::is_##trait<detail> : public std::true_type {};

} /* namespace moporgic */

namespace moporgic { class half; }
declare_trait(floating_point, moporgic::half);

namespace moporgic {
/**
 * half-precision half operation is defined in math.h
 */
class half {
public:
	constexpr half() noexcept : hf(0) {}
	constexpr half(f32 num) noexcept; /* : hf(to_half(num)) {} */
	constexpr half(const half& num) noexcept : hf(num.hf) {}
public:
	constexpr operator f32() const noexcept; /* { return to_float(hf); } */
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr operator numeric() const noexcept { return operator f32(); }
	constexpr operator bool() const noexcept { return hf; }
	constexpr half operator +(half f) const noexcept; /* { return half::as(half_add(hf, f.hf)); } */
	constexpr half operator -(half f) const noexcept; /* { return half::as(half_sub(hf, f.hf)); } */
	constexpr half operator *(half f) const noexcept; /* { return half::as(half_mul(hf, f.hf)); } */
	constexpr half operator /(half f) const noexcept; /* { return half::as(half_div(hf, f.hf)); } */
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr half operator +(numeric f) const noexcept { return operator f32() + f; }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr half operator -(numeric f) const noexcept { return operator f32() - f; }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr half operator *(numeric f) const noexcept { return operator f32() * f; }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr half operator /(numeric f) const noexcept { return operator f32() / f; }
public:
	constexpr half  operator ++(int) noexcept { half v(*this); operator +=(1); return v; }
	constexpr half  operator --(int) noexcept { half v(*this); operator -=(1); return v; }
	constexpr half& operator ++() noexcept { operator =(operator f32() + 1); return *this; }
	constexpr half& operator --() noexcept { operator =(operator f32() - 1); return *this; }
public:
	constexpr half& operator  =(const half& f) noexcept { hf = f.hf; return *this; }
	constexpr half& operator +=(half f) noexcept { return operator =(operator +(f)); }
	constexpr half& operator -=(half f) noexcept { return operator =(operator -(f)); }
	constexpr half& operator *=(half f) noexcept { return operator =(operator *(f)); }
	constexpr half& operator /=(half f) noexcept { return operator =(operator /(f)); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr half& operator  =(numeric f) noexcept { return operator =(half(f)); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr half& operator +=(numeric f) noexcept { return operator =(operator +(f)); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr half& operator -=(numeric f) noexcept { return operator =(operator -(f)); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr half& operator *=(numeric f) noexcept { return operator =(operator *(f)); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr half& operator /=(numeric f) noexcept { return operator =(operator /(f)); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr friend numeric& operator +=(numeric& n, half f) { return n += numeric(f); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr friend numeric& operator -=(numeric& n, half f) { return n -= numeric(f); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr friend numeric& operator *=(numeric& n, half f) { return n *= numeric(f); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr friend numeric& operator /=(numeric& n, half f) { return n /= numeric(f); }
public:
	constexpr bool operator ==(half f) const noexcept { return hf == f.hf; }
	constexpr bool operator !=(half f) const noexcept { return hf != f.hf; }
	constexpr bool operator <=(half f) const noexcept { return operator <=(f32(f)); }
	constexpr bool operator >=(half f) const noexcept { return operator >=(f32(f)); }
	constexpr bool operator < (half f) const noexcept { return operator < (f32(f)); }
	constexpr bool operator > (half f) const noexcept { return operator > (f32(f)); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr bool operator ==(numeric f) const noexcept { return operator numeric() == f; }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr bool operator !=(numeric f) const noexcept { return operator numeric() != f; }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr bool operator <=(numeric f) const noexcept { return operator numeric() <= f; }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr bool operator >=(numeric f) const noexcept { return operator numeric() >= f; }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr bool operator < (numeric f) const noexcept { return operator numeric() <  f; }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr bool operator > (numeric f) const noexcept { return operator numeric() >  f; }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr friend bool operator ==(numeric n, half f) { return n == numeric(f); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr friend bool operator !=(numeric n, half f) { return n != numeric(f); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr friend bool operator <=(numeric n, half f) { return n <= numeric(f); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr friend bool operator >=(numeric n, half f) { return n >= numeric(f); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr friend bool operator < (numeric n, half f) { return n <  numeric(f); }
	template<typename numeric, typename = enable_if_arithmetic<numeric>>
	constexpr friend bool operator > (numeric n, half f) { return n >  numeric(f); }

public:
	static constexpr half& as(const u16& raw) noexcept { return raw_cast<half>(raw); }
	friend std::ostream& operator <<(std::ostream& os, const half& hf) { return os << f32(hf); }
	friend std::istream& operator >>(std::istream& is, half& hf) { f32 k; is >> k; hf = k; return is; }
private:
	u16 hf;
};

} /* namespace moporgic */

typedef moporgic::half f16;

namespace moporgic {

template<typename type>
class clip {
public:
	constexpr clip(type *first, type *last) noexcept : first(first), last(last) {}
	constexpr clip(const clip& c) noexcept : first(c.first), last(c.last) {}
	constexpr clip() noexcept : first(nullptr), last(nullptr) {}
public:
	typedef type value_type;
	typedef type* iterator, pointer;
	typedef type& reference;
	typedef const type* const_iterator, const_pointer;
	typedef const type& const_reference;
	typedef size_t size_type, difference_type;
public:
	constexpr type* data() const noexcept { return first; }
	constexpr type* begin() const noexcept { return first; }
	constexpr type* end() const noexcept { return last; }
	constexpr type* begin(type* it) noexcept { return std::exchange(first, it); }
	constexpr type* end(type* it) noexcept { return std::exchange(last, it); }
	constexpr const type* cbegin() const noexcept { return begin(); }
	constexpr const type* cend() const noexcept { return end(); }
	constexpr type& front() const noexcept { return *(begin()); }
	constexpr type& back()  const noexcept { return *(end() - 1); }
	constexpr type& operator[](size_t i) const noexcept { return *(begin() + i); }
	constexpr type& at(size_t i) const noexcept { return operator[](i); }
	constexpr size_t size() const noexcept { return end() - begin(); }
	constexpr size_t capacity() const noexcept { return size(); }
	constexpr size_t max_size() const noexcept { return -1ull; }
	constexpr bool empty() const noexcept { return size() == 0; }
public:
	constexpr bool operator==(const clip<type>& c) const noexcept { return begin() == c.begin() && end() == c.end(); }
	constexpr bool operator!=(const clip<type>& c) const noexcept { return begin() != c.begin() || end() != c.end(); }
	constexpr bool operator< (const clip<type>& c) const noexcept { return begin() <  c.begin(); }
	constexpr bool operator<=(const clip<type>& c) const noexcept { return begin() <= c.begin(); }
	constexpr bool operator> (const clip<type>& c) const noexcept { return begin() >  c.begin(); }
	constexpr bool operator>=(const clip<type>& c) const noexcept { return begin() >= c.begin(); }
public:
	constexpr void swap(clip<type>& c) noexcept { std::swap(first, c.first); std::swap(last, c.last); }
	constexpr clip<type>& operator=(const clip<type>& c) noexcept = default;
protected:
	type *first, *last;
};

template<typename type, typename alloc = std::allocator<type>>
class list : public clip<type> {
public:
	constexpr list() noexcept : clip<type>() {}
	list(const clip<type>& c) : list(c.begin(), c.end()) {}
	list(const list<type>& l) : list(l.begin(), l.end()) {}
	list(list<type>&& l) noexcept : list() { clip<type>::swap(l); }
	template<typename iter, typename = enable_if_iterator_convertible<iter>>
	list(iter first, iter last) : list() { insert(clip<type>::cend(), first, last); }
	list(size_t n, const type& v = {}) : list() { insert(clip<type>::cend(), n, v); }
	list(std::initializer_list<type> init) : list(init.begin(), init.end()) {}
	~list() { clear(); }
public:
	void clear() { set(nullptr, nullptr); }
	void shrink_to_fit() noexcept /* unnecessary */ {}
	void reserve(size_t n) noexcept /* unavailable */ {}
	void resize(size_t n, const type& v = {}) {
		if (n > clip<type>::size()) insert(clip<type>::cend(), n - clip<type>::size(), v);
		else if (n < clip<type>::size()) erase(clip<type>::cbegin() + n, clip<type>::cend());
	}
	type* insert(const type* p, size_t n, const type& v) {
		type* buf = alloc().allocate(clip<type>::size() + n);
		new (buf) type[clip<type>::size() + n]();
		type* pos = std::copy(clip<type>::cbegin(), p, buf);
		std::fill(pos, pos + n, v);
		std::copy(p, clip<type>::cend(), pos + n);
		set(buf, buf + clip<type>::size() + n);
		return pos;
	}
	template<typename iter, typename = enable_if_iterator_convertible<iter>>
	type* insert(const type* p, iter i, iter j) {
		type* pos = insert(p, std::distance(i, j), type());
		std::copy(i, j, pos);
		return pos;
	}
	type* insert(const type* p, const type& v) { return insert(p, &v, &v + 1); }
	type* erase(const type* i, const type* j) {
		list<type> tmp(clip<type>::size() - std::distance(i, j));
		std::copy(j, clip<type>::cend(), std::copy(clip<type>::cbegin(), i, tmp.begin()));
		clip<type>::swap(tmp);
		return clip<type>::begin() + (i - tmp.begin());
	}
	type* erase(const type* p) { return erase(p, p + 1); }
	void push_front(const type& v) { insert(clip<type>::cbegin(), v); }
	void push_back(const type& v) { insert(clip<type>::cend(), v); }
	void pop_front() { erase(clip<type>::cbegin()); }
	void pop_back() { erase(clip<type>::cend() - 1); }
	template<typename... args> /* fake emplace */
	type* emplace(const type* p, args&&... a) { return insert(p, type(std::forward<args>(a)...)); }
	template<typename... args> /* fake emplace */
	type& emplace_back(args&&... a) { return *emplace(clip<type>::cend(), std::forward<args>(a)...); }
	void assign(size_t n, const type& v) { operator=(list<type>(n, v)); }
	template<typename iter, typename = enable_if_iterator_convertible<iter>>
	void assign(iter i, iter j) { operator=(list<type>(i, j)); }
	list<type>& operator=(const clip<type>& c) { assign(c.cbegin(), c.cend()); return *this; }
	list<type>& operator=(const list<type>& l) { assign(l.cbegin(), l.cend()); return *this; }
	list<type>& operator=(list<type>&& l) { clip<type>::swap(l); return *this; }
public:
	static constexpr list<type>& as(clip<type>& c) noexcept { return raw_cast<list<type>>(c); }
	static constexpr const list<type>& as(const clip<type>& c) noexcept { return raw_cast<list<type>>(c); }
protected:
	void set(type* first, type* last) {
		for (type* it = clip<type>::begin(); it != clip<type>::end(); it++) it->~type();
		alloc().deallocate(clip<type>::begin(), clip<type>::size());
		clip<type>::operator=(clip<type>(first, last));
	}
};

class hexadeca {
public:
	constexpr hexadeca(u64 hex = 0) noexcept : hex(hex) {}
	constexpr operator u64&() noexcept { return hex; }
	constexpr operator u64() const noexcept { return hex; }
	constexpr static hexadeca& as(u64& hex) noexcept { return raw_cast<hexadeca>(hex); }
public:
	class dual;
	class cell {
		friend class hexadeca;
		friend class hexadeca::dual;
	public:
		constexpr cell() noexcept = delete;
		constexpr cell(const cell& c) noexcept = default;
		constexpr operator u32() const noexcept { return (ref >> (idx << 2)) & 0x0f; }
		constexpr cell& operator =(u32 val) noexcept { ref = (ref & ~(0x0full << (idx << 2))) | ((val & 0x0full) << (idx << 2)); return *this; }
		constexpr cell& operator =(const cell& c) noexcept { return operator =(u32(c)); }
		constexpr cell& operator +=(u32 val) noexcept { return operator =(operator u32() + val); }
		constexpr cell& operator -=(u32 val) noexcept { return operator =(operator u32() - val); }
		constexpr cell& operator ++() noexcept { return operator +=(1); }
		constexpr cell& operator --() noexcept { return operator -=(1); }
		constexpr u32 operator ++(int) noexcept { u32 v = operator u32(); operator ++(); return v; }
		constexpr u32 operator --(int) noexcept { u32 v = operator u32(); operator --(); return v; }
	protected:
		constexpr cell(u64& ref, u32 idx) noexcept : ref(ref), idx(idx) {}
		u64& ref;
		u32 idx;
	};
public:
	constexpr cell operator [](u32 idx) const noexcept { return cell(const_cast<u64&>(hex), idx); }
	constexpr cell at(u32 idx) const noexcept { return operator [](idx); }
	constexpr size_t size() const noexcept { return at(15); }
	constexpr void resize(u32 len) noexcept { hex &= (len ? -1ull >> ((16 - len) << 2) : 0); at(15) = len; }
	constexpr size_t capacity() const noexcept { return 15; }
	constexpr size_t max_size() const noexcept { return 16; }
	constexpr bool empty() const noexcept { return size() == 0; }
	constexpr void clear() { hex = 0; }
	constexpr cell front() const noexcept { return at(0); }
	constexpr cell back() const noexcept { return at(size() - 1); }
	constexpr void push_front(u32 v) noexcept { u32 n = size(); hex <<= 4; at(0) = v; resize(n + 1); }
	constexpr void push_back(u32 v) noexcept { at(size()) = v; resize(size() + 1); }
	constexpr void pop_front() noexcept { u32 n = size(); hex >>= 4; resize(n - 1); }
	constexpr void pop_back() noexcept { resize(size() - 1); }
protected:
	u64 hex;
};

class hexadeca::dual : public hexadeca {
friend class hexadeca;
public:
	constexpr dual(u64 hex = 0, u64 ext = 0) noexcept : hexadeca(hex), ext(ext) {}
public:
	constexpr cell operator [](u32 idx) const noexcept { return cell(cast<u64*>(this)[idx >> 4], idx); }
	constexpr cell at(u32 idx) const noexcept { return operator [](idx); }
	constexpr size_t size() const noexcept { return raw_cast<u8, 7>(ext); }
	constexpr void resize(u32 len) noexcept {
		if (len > 16) ext &= (-1ull >> ((16 - (len - 16)) << 2));
		else ext = 0, hex &= (len ? -1ull >> ((16 - len) << 2) : 0);
		raw_cast<u8, 7>(ext) = len;
	}
	constexpr size_t capacity() const noexcept { return 30; }
	constexpr size_t max_size() const noexcept { return 32; }
	constexpr bool empty() const noexcept { return size() == 0; }
	constexpr void clear() { hex = ext = 0; }
	constexpr cell front() const noexcept { return at(0); }
	constexpr cell back() const noexcept { return at(size() - 1); }
	constexpr void push_front(u32 v) noexcept { u32 n = size(); ext <<= 4; at(16) = at(15); hex <<= 4; at(0) = v; resize(n + 1); }
	constexpr void push_back(u32 v) noexcept { at(size()) = v; resize(size() + 1); }
	constexpr void pop_front() noexcept { u32 n = size(); hex >>= 4; at(15) = at(16); ext >>= 4; resize(n - 1); }
	constexpr void pop_back() noexcept { resize(size() - 1); }
protected:
	u64 ext;
};

typedef hexadeca hex;
typedef hexadeca::dual hexa;

template<typename type,
	template<typename...> class list = moporgic::list,
	template<typename...> class alloc = std::allocator,
	bool bchk = false>
class segment {
public:
	constexpr segment() noexcept {}
	segment(const segment& seg) = delete;
	segment(segment&& seg) noexcept : free(std::move(seg.free)) {}

	type* allocate(size_t num) {
		for (auto it = free.begin(); it != free.end(); it++) {
			if (it->size() >= num) {
				type* buf = it->begin(it->begin() + num);
				if (it->empty()) free.erase(it);
				return buf;
			}
		}
		return nullptr;
	}

	void deallocate(type* buf, size_t num) {
		clip<type> tok(buf, buf + num);
		auto it = std::lower_bound(free.begin(), free.end(), tok);
		auto pt = std::prev(it);

		if (bchk && it != free.begin()) {
			tok.begin(std::max(tok.begin(), pt->end()));
			tok.end(std::max(tok.end(), pt->end()));
		}
		if (bchk && it != free.end()) {
			tok.end(std::min(tok.end(), it->begin()));
		}

		if (it != free.begin() && pt->end() == tok.begin()) {
			if (it != free.end() && tok.end() == it->begin()) {
				it->begin(pt->begin());
				it = free.erase(pt);
			} else {
				pt->end(tok.end());
			}
		} else {
			if (it != free.end() && tok.end() == it->begin()) {
				it->begin(tok.begin());
			} else {
				it = free.insert(it, tok);
			}
		}
	}

private:
	list<clip<type>, alloc<clip<type>>> free;
};

} /* namespace moporgic */

