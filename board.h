//============================================================================
// Name        : board.h
// Author      : Hung Guei
// Version     : 4.5
// Description : bitboard of 2048
//============================================================================

#pragma once
#include "moporgic/type.h"
#include "moporgic/util.h"
#include "moporgic/math.h"
#include <algorithm>
#include <iostream>
#include <cstdio>

namespace moporgic {

class board {
private:
	u64 raw;
	u32 ext;
	u32 inf;

public:
	inline constexpr board(u64 raw = 0, u32 ext = 0, u32 inf = 0) : raw(raw), ext(ext), inf(inf) {}
	inline constexpr board(u64 raw, u16 ext) : board(raw, u32(ext) << 16) {}
	inline constexpr board(const board& b) = default;
	inline constexpr board& operator =(u64 raw) { this->raw = raw; this->ext = 0; return *this; }
	inline constexpr board& operator =(const board& b) = default;

	inline constexpr operator u64&() { return raw; }
	inline constexpr operator u64() const { return raw; }
	declare_comparators_with(const board&, raw_cast<u128>(*this), raw_cast<u128>(v), inline constexpr)

public:
	class cache {
	public:
		static inline const cache& load(u32 i) {
			static byte block[sizeof(cache) * (1 << 20)] = {};
			return pointer_cast<cache>(block)[i];
		}
		static __attribute__((constructor)) void make() {
			if (load(0).moved) return;
			for (u32 i = 0; i < (1 << 20); i++) new (const_cast<board::cache*>(&load(i))) board::cache(i);
		}

	public:
		cache(const cache& c) = default;
		cache() : raw(0), ext(0), species(0), merge(0), moved(-1), legal(0) {}
		cache(u32 r) : raw(r & 0x0ffff), ext(r & 0xf0000), species(0), merge(0), left(r, false), right(r, true), moved(-1), legal(0) {
			for (int i = 0; i < 4; i++) {
				u32 t = ((r >> (i << 2)) & 0x0f) | ((r >> (12 + i)) & 0x10);
				species |= (1 << t);
				numof[t] += 1;
				mask[t] |= (1 << i);
			}
			for (int i = 0; i < 16; i++) {
				if ((r >> i) & 1) layout.push_back(i);
			}
			merge = left.merge | right.merge;
			moved = left.moved & right.moved;
			if (left.moved == 0)  legal |= (0x08 | 0x01);
			if (right.moved == 0) legal |= (0x02 | 0x04);
		}

	public:
		class move {
		public:
			template<int i> inline void moveh64(u64& raw, u32& sc) const {
				raw |= u64(rawh) << (i << 4);
				sc += score;
			}
			template<int i> inline void moveh80(u64& raw, u32& ext, u32& sc) const {
				moveh64<i>(raw, sc);
				ext |= exth << (i << 2);
			}
			template<int i> inline void movev64(u64& raw, u32& sc) const {
				raw |= rawv << (i << 2);
				sc += score;
			}
			template<int i> inline void movev80(u64& raw, u32& ext, u32& sc) const {
				movev64<i>(raw, sc);
				ext |= extv << i;
			}

		public:
			move(const move& op) = default;
			move() : rawh(0), exth(0), rawv(0), extv(0), score(0), moved(-1), merge(0), mono(0) {}
			move(u32 r, bool reverse) : rawh(0), exth(0), rawv(0), extv(0), score(0), moved(-1), merge(0), mono(0) {
				u32 row[] = {((r >> 0) & 0x0f) | ((r >> 12) & 0x10), ((r >> 4) & 0x0f) | ((r >> 13) & 0x10),
							((r >> 8) & 0x0f) | ((r >> 14) & 0x10), ((r >> 12) & 0x0f) | ((r >> 15) & 0x10)};
				if (reverse) std::reverse(row, row + 4);
				u32 top = 0, hold = 0;
				for (u32 i = 0; i < 4; i++) {
					u32 tile = row[i];
					if (tile == 0) continue;
					row[i] = 0;
					if (hold) {
						if (tile == hold) {
							row[top++] = ++tile;
							score += (1 << tile);
							merge++;
							hold = 0;
						} else {
							row[top++] = hold;
							hold = tile;
						}
					} else {
						hold = tile;
					}
				}
				if (hold) row[top] = hold;
				if (reverse) std::reverse(row, row + 4);

				u32 lo[] = { (row[0] & 0x0f), (row[1] & 0x0f), (row[2] & 0x0f), (row[3] & 0x0f) };
				u32 hi[] = { (row[0] & 0x10) >> 4, (row[1] & 0x10) >> 4, (row[2] & 0x10) >> 4, (row[3] & 0x10) >> 4 };
				rawh = ((lo[0] << 0) | (lo[1] << 4) | (lo[2] << 8) | (lo[3] << 12));
				exth = ((hi[0] << 0) | (hi[1] << 1) | (hi[2] << 2) | (hi[3] << 3)) << 16;
				rawv = (u64(lo[0]) << 0) | (u64(lo[1]) << 16) | (u64(lo[2]) << 32) | (u64(lo[3]) << 48);
				extv = ((hi[0] << 0) | (hi[1] << 4) | (hi[2] << 8) | (hi[3] << 12)) << 16;
				moved = ((rawh | exth) == r) ? -1 : 0;

				const u32 monores[6][2] = { { 0, 1 }, { 0, 2 }, { 0, 3 }, { 1, 2 }, { 1, 3 }, { 2, 3 }, };
				for (u32 i = 0; i < 6; i++) {
					u32 a = row[monores[i][0]], b = row[monores[i][1]];
					mono |= ((a == b ? (a ? 0b11 : 0b00) : (a > b ? 0b01 : 0b10)) << (i << 1));
				}
			}

		public:
			u32 rawh; // horizontal move (16-bit raw)
			u32 exth; // horizontal move (4-bit extra)
			u64 rawv; // vertical move (64-bit raw)
			u32 extv; // vertical move (16-bit extra)
			u32 score; // merge score (reward)
			i32 moved; // moved or not (moved: 0, otherwise -1)
			u16 merge; // number of merged tiles
			u16 mono; // monotonic decreasing value (12-bit)
		};

		template<int i> inline void moveh64(u64& lraw, u32& lsc, u64& rraw, u32& rsc) const {
			left.moveh64<i>(lraw, lsc);
			right.moveh64<i>(rraw, rsc);
		}
		template<int i> inline void moveh80(u64& lraw, u32& lext, u32& lsc, u64& rraw, u32& rext, u32& rsc) const {
			left.moveh80<i>(lraw, lext, lsc);
			right.moveh80<i>(rraw, lext, rsc);
		}
		template<int i> inline void movev64(u64& uraw, u32& usc, u64& draw, u32& dsc) const {
			left.movev64<i>(uraw, usc);
			right.movev64<i>(draw, dsc);
		}
		template<int i> inline void movev80(u64& uraw, u32& uext, u32& usc, u64& draw, u32& dext, u32& dsc) const {
			left.movev80<i>(uraw, uext, usc);
			right.movev80<i>(draw, dext, dsc);
		}

	public:
		u32 raw; // base row (16-bit raw)
		u32 ext; // base row (4-bit extra)
		u32 species; // species of this row
		u32 merge; // number of merged tiles
		move left; // left operation
		move right; // right operation
		hexa numof; // number of each tile-type
		hexa mask; // mask of each tile-type
		hexa layout; // layout of board-type
		i32 moved; // moved or not
		u32 legal; // legal actions
	};

	inline const cache& query(u32 r) const { return query16(r); }
	inline const cache& query16(u32 r) const { return cache::load(fetch16(r)); }
	inline const cache& query20(u32 r) const { return cache::load(fetch20(r)); }

	inline constexpr u32 fetch(u32 i) const { return fetch16(i); }
	inline constexpr u32 fetch16(u32 i) const {
		return ((raw >> (i << 4)) & 0xffff);
	}
	inline constexpr u32 fetch20(u32 i) const {
		return fetch16(i) | ((ext >> (i << 2)) & 0xf0000);
	}

	inline constexpr void place(u32 i, u32 r) { place16(i, r); }
	inline constexpr void place16(u32 i, u32 r) {
		raw = (raw & ~(0xffffull << (i << 4))) | (u64(r & 0xffff) << (i << 4));
	}
	inline constexpr void place20(u32 i, u32 r) {
		place16(i, r & 0xffff);
		ext = (ext & ~(0xf0000 << (i << 2))) | ((r & 0xf0000) << (i << 2));
	}

	inline constexpr u32 at(u32 i) const { return at4(i); }
	inline constexpr u32 at4(u32 i) const {
		return (raw >> (i << 2)) & 0x0f;
	}
	inline constexpr u32 at5(u32 i) const {
		return at4(i) | ((ext >> (i + 12)) & 0x10);
	}

	inline constexpr void set(u32 i, u32 t) { set4(i, t); }
	inline constexpr void set4(u32 i, u32 t) {
		raw = (raw & ~(0x0full << (i << 2))) | (u64(t & 0x0f) << (i << 2));
	}
	inline constexpr void set5(u32 i, u32 t) {
		set4(i, t);
		ext = (ext & ~(1U << (i + 16))) | ((t & 0x10) << (i + 12));
	}
	inline constexpr void set(u64 raw, u32 ext = 0) { this->raw = raw; this->ext = ext; }
	inline constexpr void set(u64 raw, u16 ext) { set(raw, u32(ext) << 16); }
	inline constexpr void set(const board& b) { set(b.raw, b.ext); }

	inline constexpr void mirror() { mirror64(); }
	inline constexpr void mirror64() {
		raw = ((raw & 0x000f000f000f000full) << 12) | ((raw & 0x00f000f000f000f0ull) << 4)
			| ((raw & 0x0f000f000f000f00ull) >> 4) | ((raw & 0xf000f000f000f000ull) >> 12);
	}
	inline constexpr void mirror80() {
		mirror64();
		ext = ((ext & 0x11110000) << 3) | ((ext & 0x22220000) << 1)
			| ((ext & 0x44440000) >> 1) | ((ext & 0x88880000) >> 3);
	}

	inline constexpr void flip() { flip64(); }
	inline constexpr void flip64() {
		raw = ((raw & 0x000000000000ffffull) << 48) | ((raw & 0x00000000ffff0000ull) << 16)
			| ((raw & 0x0000ffff00000000ull) >> 16) | ((raw & 0xffff000000000000ull) >> 48);
	}
	inline constexpr void flip80() {
		flip64();
		ext = ((ext & 0x000f0000) << 12) | ((ext & 0x00f00000) << 4)
			| ((ext & 0x0f000000) >> 4) | ((ext & 0xf0000000) >> 12);
	}

	inline constexpr void reverse() { reverse64(); }
	inline constexpr void reverse64() { mirror64(); flip64(); }
	inline constexpr void reverse80() { mirror80(); flip80(); }

	inline constexpr void transpose() { transpose64(); }
	inline constexpr void transpose64() {
		register u64 buf = 0;
		buf = (raw ^ (raw >> 12)) & 0x0000f0f00000f0f0ull;
		raw ^= buf ^ (buf << 12);
		buf = (raw ^ (raw >> 24)) & 0x00000000ff00ff00ull;
		raw ^= buf ^ (buf << 24);
	}
	inline constexpr void transpose80() {
		transpose64();
		register u32 buf = 0;
		buf = (ext ^ (ext >> 3)) & 0x0a0a0000;
		ext ^= buf ^ (buf << 3);
		buf = (ext ^ (ext >> 6)) & 0x00cc0000;
		ext ^= buf ^ (buf << 6);
	}

	inline constexpr u32 empty() const { return empty64(); }
	inline constexpr u32 empty64() const {
		register u64 x = raw;
		x |= (x >> 2);
		x |= (x >> 1);
		x = ~x & 0x1111111111111111ull;
		register u32 e = x + (x >> 32);
		e += e >> 16;
		e += e >> 8;
		return (e & 0x0f) + ((e >> 4) & 0x0f);
	}
	inline constexpr u32 empty80() const {
		register u64 x = raw;
		x |= (x >> 2);
		x |= (x >> 1);
		x = ~x & 0x1111111111111111ull;
		u16 z[]{ 0x1111, 0x1110, 0x1101, 0x1100, 0x1011, 0x1010, 0x1001, 0x1000, 0x0111, 0x0110, 0x0101, 0x0100, 0x0011, 0x0010, 0x0001, 0x0000 };
		x &= (u64(z[(ext >> 28) & 0x0f]) << 48) | (u64(z[(ext >> 24) & 0x0f]) << 32) | (u64(z[(ext >> 20) & 0x0f]) << 16) | u64(z[(ext >> 16) & 0x0f]);
		register u32 e = x + (x >> 32);
		e += e >> 16;
		e += e >> 8;
		return (e & 0x0f) + ((e >> 4) & 0x0f);
	}

	inline hexa spaces() const { return spaces64(); }
	inline hexa spaces64() const { return find64(0); }
	inline hexa spaces80() const { return find80(0); }

	inline void init() {
		u32 u = moporgic::rand();
		u32 k = (u & 0xffff);
		u32 i = (k) % 16;
		u32 j = (i + 1 + (k >> 4) % 15) % 16;
		u32 r = (u >> 16) % 100;
		raw =  (r >=  1 ? 1ull : 2ull) << (i << 2);
		raw |= (r >= 19 ? 1ull : 2ull) << (j << 2);
		ext = 0;
	}

	inline void next() { return next64(); }
	inline void next64() {
		u64 x = raw;
		x |= (x >> 2);
		x |= (x >> 1);
		x = ~x & 0x1111111111111111ull;
		u32 e = x + (x >> 32);
		e += e >> 16;
		e += e >> 8;
		e = (e & 0x0f) + ((e >> 4) & 0x0f);
		u32 u = moporgic::rand();
		u32 k = (u >> 16) % e;
		while (k--) x &= ~(x & -x);
		u32 p = math::lg64(x & -x);;
		raw |= (u % 10 ? 1ull : 2ull) << p;
	}
	inline void next80() {
		u64 x = raw;
		x |= (x >> 2);
		x |= (x >> 1);
		x = ~x & 0x1111111111111111ull;
		u16 z[]{ 0x1111, 0x1110, 0x1101, 0x1100, 0x1011, 0x1010, 0x1001, 0x1000, 0x0111, 0x0110, 0x0101, 0x0100, 0x0011, 0x0010, 0x0001, 0x0000 };
		x &= (u64(z[(ext >> 28) & 0x0f]) << 48) | (u64(z[(ext >> 24) & 0x0f]) << 32) | (u64(z[(ext >> 20) & 0x0f]) << 16) | u64(z[(ext >> 16) & 0x0f]);
		u32 e = x + (x >> 32);
		e += e >> 16;
		e += e >> 8;
		e = (e & 0x0f) + ((e >> 4) & 0x0f);
		u32 u = moporgic::rand();
		u32 k = (u >> 16) % e;
		while (k--) x &= ~(x & -x);
		u32 p = math::lg64(x & -x);;
		raw |= (u % 10 ? 1ull : 2ull) << p;
	}

	inline bool popup() { return popup64(); }
	inline bool popup64() {
		hexa empty = spaces64();
		if (empty.size() == 0) return false;
		u32 u = moporgic::rand();
		u32 p = hex::as(empty)[(u >> 16) % empty.size()];
		raw |= (u % 10 ? 1ull : 2ull) << (p << 2);
		return true;
	}
	inline bool popup80() {
		hexa empty = spaces80();
		if (empty.size() == 0) return false;
		u32 u = moporgic::rand();
		u32 p = hex::as(empty)[(u >> 16) % empty.size()];
		raw |= (u % 10 ? 1ull : 2ull) << (p << 2);
		return true;
	}

	inline constexpr void clear() {
		raw = 0;
		ext = 0;
	}

	inline void rotright()   { rotright64(); }
	inline void rotright64() { transpose64(); mirror64(); }
	inline void rotright80() { transpose80(); mirror80(); }

	inline void rotleft()   { rotleft64(); }
	inline void rotleft64() { transpose64(); flip64(); }
	inline void rotleft80() { transpose80(); flip80(); }

	inline void rotate(int r = 1) {
		rotate64(r);
	}
	inline void rotate64(int r = 1) {
		switch (((r % 4) + 4) % 4) {
		default:
		case 0: break;
		case 1: rotright64(); break;
		case 2: reverse64(); break;
		case 3: rotleft64(); break;
		}
	}
	inline void rotate80(int r = 1) {
		switch (((r % 4) + 4) % 4) {
		default:
		case 0: break;
		case 1: rotright80(); break;
		case 2: reverse80(); break;
		case 3: rotleft80(); break;
		}
	}

	inline void isomorphic(int i = 0) {
		return isomorphic64(i);
	}
	inline void isomorphic64(int i = 0) {
		if ((i % 8) / 4) mirror64();
		rotate64(i);
	}
	inline void isomorphic80(int i = 0) {
		if ((i % 8) / 4) mirror80();
		rotate80(i);
	}

	inline i32 left()  { return left64(); }
	inline i32 right() { return right64(); }
	inline i32 up()    { return up64(); }
	inline i32 down()  { return down64(); }

	inline i32 left64() {
		register u64 rawp = raw, rawn = 0;
		register u32 score = 0;
		query16(0).left.moveh64<0>(rawn, score);
		query16(1).left.moveh64<1>(rawn, score);
		query16(2).left.moveh64<2>(rawn, score);
		query16(3).left.moveh64<3>(rawn, score);
		register i32 moved = (rawp ^ rawn) ? 0 : -1;
		raw = rawn;
		return score | moved;
	}
	inline i32 right64() {
		register u64 rawp = raw, rawn = 0;
		register u32 score = 0;
		query16(0).right.moveh64<0>(rawn, score);
		query16(1).right.moveh64<1>(rawn, score);
		query16(2).right.moveh64<2>(rawn, score);
		query16(3).right.moveh64<3>(rawn, score);
		register i32 moved = (rawp ^ rawn) ? 0 : -1;
		raw = rawn;
		return score | moved;
	}
	inline i32 up64() {
		register u64 rawp = raw, rawn = 0;
		register u32 score = 0;
		transpose64();
		query16(0).left.movev64<0>(rawn, score);
		query16(1).left.movev64<1>(rawn, score);
		query16(2).left.movev64<2>(rawn, score);
		query16(3).left.movev64<3>(rawn, score);
		register i32 moved = (rawp ^ rawn) ? 0 : -1;
		raw = rawn;
		return score | moved;
	}
	inline i32 down64() {
		register u64 rawp = raw, rawn = 0;
		register u32 score = 0;
		transpose64();
		query16(0).right.movev64<0>(rawn, score);
		query16(1).right.movev64<1>(rawn, score);
		query16(2).right.movev64<2>(rawn, score);
		query16(3).right.movev64<3>(rawn, score);
		register i32 moved = (rawp ^ rawn) ? 0 : -1;
		raw = rawn;
		return score | moved;
	}

	inline i32 left80() {
		register u64 rawp = raw, rawn = 0;
		register u32 extp = ext, extn = 0;
		register u32 score = 0;
		query20(0).left.moveh80<0>(rawn, extn, score);
		query20(1).left.moveh80<1>(rawn, extn, score);
		query20(2).left.moveh80<2>(rawn, extn, score);
		query20(3).left.moveh80<3>(rawn, extn, score);
		register i32 moved = (rawp ^ rawn) | (extp ^ extn) ? 0 : -1;
		raw = rawn;
		ext = extn;
		return score | moved;
	}
	inline i32 right80() {
		register u64 rawp = raw, rawn = 0;
		register u32 extp = ext, extn = 0;
		register u32 score = 0;
		query20(0).right.moveh80<0>(rawn, extn, score);
		query20(1).right.moveh80<1>(rawn, extn, score);
		query20(2).right.moveh80<2>(rawn, extn, score);
		query20(3).right.moveh80<3>(rawn, extn, score);
		register i32 moved = (rawp ^ rawn) | (extp ^ extn) ? 0 : -1;
		raw = rawn;
		ext = extn;
		return score | moved;
	}
	inline i32 up80() {
		register u64 rawp = raw, rawn = 0;
		register u32 extp = ext, extn = 0;
		register u32 score = 0;
		transpose80();
		query20(0).left.movev80<0>(rawn, extn, score);
		query20(1).left.movev80<1>(rawn, extn, score);
		query20(2).left.movev80<2>(rawn, extn, score);
		query20(3).left.movev80<3>(rawn, extn, score);
		register i32 moved = (rawp ^ rawn) | (extp ^ extn) ? 0 : -1;
		raw = rawn;
		ext = extn;
		return score | moved;
	}
	inline i32 down80() {
		register u64 rawp = raw, rawn = 0;
		register u32 extp = ext, extn = 0;
		register u32 score = 0;
		transpose80();
		query20(0).right.movev80<0>(rawn, extn, score);
		query20(1).right.movev80<1>(rawn, extn, score);
		query20(2).right.movev80<2>(rawn, extn, score);
		query20(3).right.movev80<3>(rawn, extn, score);
		register i32 moved = (rawp ^ rawn) | (extp ^ extn) ? 0 : -1;
		raw = rawn;
		ext = extn;
		return score | moved;
	}

	inline void after(board& U, board& R, board& D, board& L) const {
		return after64(U, R, D, L);
	}
	inline void after64(board& U, board& R, board& D, board& L) const {
		U = R = D = L = board();

		board b(*this);
		b.query16(0).moveh64<0>(L.raw, L.inf, R.raw, R.inf);
		b.query16(1).moveh64<1>(L.raw, L.inf, R.raw, R.inf);
		b.query16(2).moveh64<2>(L.raw, L.inf, R.raw, R.inf);
		b.query16(3).moveh64<3>(L.raw, L.inf, R.raw, R.inf);
		L.inf |= (L.raw ^ raw) ? 0 : -1;
		R.inf |= (R.raw ^ raw) ? 0 : -1;

		b.transpose64();
		b.query16(0).movev64<0>(U.raw, U.inf, D.raw, D.inf);
		b.query16(1).movev64<1>(U.raw, U.inf, D.raw, D.inf);
		b.query16(2).movev64<2>(U.raw, U.inf, D.raw, D.inf);
		b.query16(3).movev64<3>(U.raw, U.inf, D.raw, D.inf);
		U.inf |= (U.raw ^ raw) ? 0 : -1;
		D.inf |= (D.raw ^ raw) ? 0 : -1;
	}
	inline void after80(board& U, board& R, board& D, board& L) const {
		U = R = D = L = board();

		board b(*this);
		b.query20(0).moveh80<0>(L.raw, L.ext, L.inf, R.raw, R.ext, R.inf);
		b.query20(1).moveh80<1>(L.raw, L.ext, L.inf, R.raw, R.ext, R.inf);
		b.query20(2).moveh80<2>(L.raw, L.ext, L.inf, R.raw, R.ext, R.inf);
		b.query20(3).moveh80<3>(L.raw, L.ext, L.inf, R.raw, R.ext, R.inf);
		L.inf |= (L.raw ^ raw) | (L.ext ^ ext) ? 0 : -1;
		R.inf |= (R.raw ^ raw) | (R.ext ^ ext) ? 0 : -1;

		b.transpose80();
		b.query20(0).movev80<0>(U.raw, U.ext, U.inf, D.raw, D.ext, D.inf);
		b.query20(1).movev80<1>(U.raw, U.ext, U.inf, D.raw, D.ext, D.inf);
		b.query20(2).movev80<2>(U.raw, U.ext, U.inf, D.raw, D.ext, D.inf);
		b.query20(3).movev80<3>(U.raw, U.ext, U.inf, D.raw, D.ext, D.inf);
		U.inf |= (U.raw ^ raw) | (U.ext ^ ext) ? 0 : -1;
		D.inf |= (D.raw ^ raw) | (D.ext ^ ext) ? 0 : -1;
	}

	class action {
	public:
		action() = delete;
		enum opcode : u32 {
			up    = 0x00u,
			right = 0x01u,
			down  = 0x02u,
			left  = 0x03u,
			next  = 0x0eu,
			init  = 0x0fu,

			x64   = 0x40u,
			x80   = 0x80u,
			ext   = x80,

			up64    = up | x64,
			right64 = right | x64,
			down64  = down | x64,
			left64  = left | x64,
			next64  = next | x64,
			init64  = init | x64,

			up80    = up | x80,
			right80 = right | x80,
			down80  = down | x80,
			left80  = left | x80,
			next80  = next | x80,
			init80  = init | x80,
		};
	};

	inline i32 operate(u32 op) {
		if (op & action::x64) return operate64(op);
		if (op & action::x80) return operate80(op);
        return operate64(op);
	}
	inline i32 operate64(u32 op) {
		switch (op & 0x0fu) {
		case action::up:    return up64();
		case action::right: return right64();
		case action::down:  return down64();
		case action::left:  return left64();
		case action::next:  return popup64() ? 0 : -1;
		case action::init:  return init(), 0;
		default:            return -1;
		}
	}
	inline i32 operate80(u32 op) {
		switch (op & 0x0fu) {
		case action::up:    return up80();
		case action::right: return right80();
		case action::down:  return down80();
		case action::left:  return left80();
		case action::next:  return popup80() ? 0 : -1;
		case action::init:  return init(), 0;
		default:            return -1;
		}
	}

	inline i32 move(u32 op)   { return operate(op); }
	inline i32 move64(u32 op) { return operate64(op); }
	inline i32 move80(u32 op) { return operate80(op); }

	inline constexpr u32 shift(u32 k = 0, u32 u = 0) { return shift64(k, u); }
	inline constexpr u32 shift64(u32 k = 0, u32 u = 0) {
		u32 hash = hash64();
		u32 tile = math::msb16(hash);
		u32 mask = (((k ? (1 << k) : tile) << 1) - 1) & ~((2 << u) - 1);
		u32 hole = ~hash & mask;
		if (hole == 0 || hole > tile) return 0;
		u32 h = math::lg16(hole);
		for (u32 i = 0; i < 16; i++) {
			u32 t = at4(i);
			set4(i, t > h ? t - 1 : t);
		}
		return h;
	}
	inline constexpr u32 shift80(u32 k = 0, u32 u = 0) {
		u32 hash = hash80();
		u32 tile = math::msb16(hash);
		u32 mask = (((k ? (1 << k) : tile) << 1) - 1) & ~((2 << u) - 1);
		u32 hole = ~hash & mask;
		if (hole == 0 || hole > tile) return 0;
		u32 h = math::lg16(hole);
		for (u32 i = 0; i < 16; i++) {
			u32 t = at5(i);
			set5(i, t > h ? t - 1 : t);
		}
		return h;
	}

	inline u32 species() const { return species64(); }
	inline u32 species64() const {
		return query16(0).species | query16(1).species | query16(2).species | query16(3).species;
	}
	inline u32 species80() const {
		return query20(0).species | query20(1).species | query20(2).species | query20(3).species;
	}

	inline u32 scale() const   { return scale64(); }
	inline u32 scale64() const { return species64(); }
	inline u32 scale80() const { return species80(); }

	inline constexpr u32 hash() const { return hash64(); }
	inline constexpr u32 hash64() const {
		register u32 h = 0;
		for (u32 i = 0; i < 16; i++) h |= (1 << at4(i));
		return h;
	}
	inline constexpr u32 hash80() const {
		register u32 h = 0;
		for (u32 i = 0; i < 16; i++) h |= (1 << at5(i));
		return h;
	}

	inline u32 max()   const { return max64(); }
	inline u32 max64() const { return math::log2(scale64()); }
	inline u32 max80() const { return math::log2(scale80()); }

	inline hex numof() const {
		return query(0).numof + query(1).numof + query(2).numof + query(3).numof;
	}

	inline u32 numof(u32 t) const { return numof64(t); }
	inline u32 numof64(u32 t) const {
		return query16(0).numof[t] + query16(1).numof[t] + query16(2).numof[t] + query16(3).numof[t];
	}
	inline u32 numof80(u32 t) const {
		return query20(0).numof[t] + query20(1).numof[t] + query20(2).numof[t] + query20(3).numof[t];
	}

	inline void numof(u32 num[], u32 min, u32 max) const { return numof64(num, min, max); }
	inline void numof64(u32 num[], u32 min, u32 max) const {
		hexa numof0 = query16(0).numof;
		hexa numof1 = query16(1).numof;
		hexa numof2 = query16(2).numof;
		hexa numof3 = query16(3).numof;
		for (u32 i = min; i < max; i++) {
			num[i] = numof0[i] + numof1[i] + numof2[i] + numof3[i];
		}
	}
	inline void numof80(u32 num[], u32 min, u32 max) const {
		hexa numof0 = query20(0).numof;
		hexa numof1 = query20(1).numof;
		hexa numof2 = query20(2).numof;
		hexa numof3 = query20(3).numof;
		for (u32 i = min; i < max; i++) {
			num[i] = numof0[i] + numof1[i] + numof2[i] + numof3[i];
		}
	}

	inline constexpr u32 count(u32 t) const { return count64(t); }
	inline constexpr u32 count64(u32 t) const {
		register u32 num = 0;
		for (u32 i = 0; i < 16; i++)
			if (at4(i) == t) num++;
		return num;
	}
	inline constexpr u32 count80(u32 t) const {
		register u32 num = 0;
		for (u32 i = 0; i < 16; i++)
			if (at5(i) == t) num++;
		return num;
	}

	inline constexpr void count(u32 num[], u32 min, u32 max) const { return count64(num, min, max); }
	inline constexpr void count64(u32 num[], u32 min, u32 max) const {
		for (u32 i = 0; i < 16; i++) num[at4(i)]++;
	}
	inline constexpr void count80(u32 num[], u32 min, u32 max) const {
		for (u32 i = 0; i < 16; i++) num[at5(i)]++;
	}

	inline u32 mask(u32 t) const { return mask64(t); }
	inline u32 mask64(u32 t) const {
		return (query16(0).mask[t] << 0) | (query16(1).mask[t] << 4) | (query16(2).mask[t] << 8) | (query16(3).mask[t] << 12);
	}
	inline u32 mask80(u32 t) const {
		return (query20(0).mask[t] << 0) | (query20(1).mask[t] << 4) | (query20(2).mask[t] << 8) | (query20(3).mask[t] << 12);
	}

	inline void mask(u32 msk[], u32 min, u32 max) const { return mask64(msk, min, max); }
	inline void mask64(u32 msk[], u32 min, u32 max) const {
		hexa mask0 = query16(0).mask;
		hexa mask1 = query16(1).mask;
		hexa mask2 = query16(2).mask;
		hexa mask3 = query16(3).mask;
		for (u32 i = min; i < max; i++) {
			msk[i] = (mask0[i] << 0) | (mask1[i] << 4) | (mask2[i] << 8) | (mask3[i] << 12);
		}
	}
	inline void mask80(u32 msk[], u32 min, u32 max) const {
		hexa mask0 = query20(0).mask;
		hexa mask1 = query20(1).mask;
		hexa mask2 = query20(2).mask;
		hexa mask3 = query20(3).mask;
		for (u32 i = min; i < max; i++) {
			msk[i] = (mask0[i] << 0) | (mask1[i] << 4) | (mask2[i] << 8) | (mask3[i] << 12);
		}
	}

	inline hexa find(u32 t) const { return find64(t); }
	inline hexa find64(u32 t) const { return cache::load(mask64(t)).layout; }
	inline hexa find80(u32 t) const { return cache::load(mask80(t)).layout; }

	inline u64 monoleft() const { return monoleft64(); }
	inline u64 monoleft64() const {
		register u64 mono = 0;
		mono |= u64(query16(0).left.mono) <<  0;
		mono |= u64(query16(1).left.mono) << 12;
		mono |= u64(query16(2).left.mono) << 24;
		mono |= u64(query16(3).left.mono) << 36;
		return mono;
	}
	inline u64 monoleft80() const {
		register u64 mono = 0;
		mono |= u64(query20(0).left.mono) <<  0;
		mono |= u64(query20(1).left.mono) << 12;
		mono |= u64(query20(2).left.mono) << 24;
		mono |= u64(query20(3).left.mono) << 36;
		return mono;
	}

	inline u64 monoright() const { return monoright64(); }
	inline u64 monoright64() const {
		register u64 mono = 0;
		mono |= u64(query16(0).right.mono) <<  0;
		mono |= u64(query16(1).right.mono) << 12;
		mono |= u64(query16(2).right.mono) << 24;
		mono |= u64(query16(3).right.mono) << 36;
		return mono;
	}
	inline u64 monoright80() const {
		register u64 mono = 0;
		mono |= u64(query20(0).right.mono) <<  0;
		mono |= u64(query20(1).right.mono) << 12;
		mono |= u64(query20(2).right.mono) << 24;
		mono |= u64(query20(3).right.mono) << 36;
		return mono;
	}

	inline u64 monotonic(bool left = true) const   { return left ? monoleft() : monoright(); }
	inline u64 monotonic64(bool left = true) const { return left ? monoleft64() : monoright64(); }
	inline u64 monotonic80(bool left = true) const { return left ? monoleft80() : monoright80(); }

	inline u32 operations() const { return operations64(); }
	inline u32 operations64() const {
		board trans(*this); trans.transpose64();
		u32 hori = this->query16(0).legal | this->query16(1).legal | this->query16(2).legal | this->query16(3).legal;
		u32 vert = trans.query16(0).legal | trans.query16(1).legal | trans.query16(2).legal | trans.query16(3).legal;
		return (hori & 0x0a) | (vert & 0x05);
	}
	inline u32 operations80() const {
		board trans(*this); trans.transpose80();
		u32 hori = this->query20(0).legal | this->query20(1).legal | this->query20(2).legal | this->query20(3).legal;
		u32 vert = trans.query20(0).legal | trans.query20(1).legal | trans.query20(2).legal | trans.query20(3).legal;
		return (hori & 0x0a) | (vert & 0x05);
	}

	inline hex actions() const { return actions64(); }
	inline hex actions64() const {
		u32 o = operations64();
		u32 k = 0, x = 0;
		u32 u = o & 1;
		k |= (u ? 0 : 0) << x;
		x += (u << 2);
		u32 r = o & 2;
		k |= (r ? 1 : 0) << x;
		x += (r << 1);
		u32 d = o & 4;
		k |= (d ? 2 : 0) << x;
		x += (d >> 0);
		u32 l = o & 8;
		k |= (l ? 3 : 0) << x;
		x += (l >> 1);
		hex a(k); a[15] = (x >> 2);
		return a;
	}
	inline hex actions80() const {
		u32 o = operations80();
		u32 k = 0, x = 0;
		u32 u = o & 1;
		k |= (u ? 0 : 0) << x;
		x += (u << 2);
		u32 r = o & 2;
		k |= (r ? 1 : 0) << x;
		x += (r << 1);
		u32 d = o & 4;
		k |= (d ? 2 : 0) << x;
		x += (d >> 0);
		u32 l = o & 8;
		k |= (l ? 3 : 0) << x;
		x += (l >> 1);
		hex a(k); a[15] = (x >> 2);
		return a;
	}

	inline bool operable() const { return operable64(); }
	inline bool operable64() const {
		if (this->query16(0).moved == 0) return true;
		if (this->query16(1).moved == 0) return true;
		if (this->query16(2).moved == 0) return true;
		if (this->query16(3).moved == 0) return true;
		board trans(*this); trans.transpose64();
		if (trans.query16(0).moved == 0) return true;
		if (trans.query16(1).moved == 0) return true;
		if (trans.query16(2).moved == 0) return true;
		if (trans.query16(3).moved == 0) return true;
		return false;
	}
	inline bool operable80() const {
		if (this->query20(0).moved == 0) return true;
		if (this->query20(1).moved == 0) return true;
		if (this->query20(2).moved == 0) return true;
		if (this->query20(3).moved == 0) return true;
		board trans(*this); trans.transpose80();
		if (trans.query20(0).moved == 0) return true;
		if (trans.query20(1).moved == 0) return true;
		if (trans.query20(2).moved == 0) return true;
		if (trans.query20(3).moved == 0) return true;
		return false;
	}

	inline bool movable() const   { return operable(); }
	inline bool movable64() const { return operable64(); }
	inline bool movable80() const { return operable80(); }

	class tile {
	friend class board;
	public:
		inline constexpr tile(const tile& t) = default;
		inline constexpr tile() = delete;
		inline constexpr board& whole() const { return b; }
		inline constexpr u32 where() const { return i; }
		inline constexpr operator u32() const { return at(is(style::extend), is(style::exact)); }
		inline constexpr tile& operator =(u32 k) { set(k, is(style::extend), is(style::exact)); return *this; }
	public:
		inline static constexpr u32 itov(u32 i) { return (1u << i) & 0xfffffffeu; }
		inline static constexpr u32 vtoi(u32 v) { return math::log2(v); }
	public:
		friend std::ostream& operator <<(std::ostream& out, const tile& t) {
			u32 v = t.at(t.is(style::extend), !t.is(style::binary) && t.is(style::exact));
			return t.is(style::binary) ? moporgic::write_cast<byte>(out, v) : (out << v);
		}
		friend std::istream& operator >>(std::istream& in, const tile& t) {
			u32 v;
			if (t.is(style::binary) ? moporgic::read_cast<byte>(in, v) : (in >> v))
				t.set(v, t.is(style::extend), !t.is(style::binary) && t.is(style::exact));
			return in;
		}
	protected:
		inline constexpr tile(const board& b, u32 i) : b(const_cast<board&>(b)), i(i) {}
		board& b;
		u32 i;
		inline constexpr bool is(u32 item) const { return b.inf & item; }
		inline constexpr u32 at(bool extend, bool exact) const {
			u32 v = extend ? b.at5(i) : b.at4(i);
			return exact ? tile::itov(v) : v;
		}
		inline constexpr void set(u32 k, bool extend, bool exact) const {
			u32 v = exact ? tile::itov(k) : k;
			if (extend) b.set5(i, v); else b.set4(i, v);
		}
	};
	class iter : tile {
	friend class board;
	public:
		typedef u32 value_type;
		typedef i32 difference_type;
		typedef tile& reference;
		typedef iter pointer;
		typedef std::forward_iterator_tag iterator_category;
		inline constexpr iter(const iter& t) = default;
		inline constexpr iter() = delete;
		inline constexpr tile& operator *() { return *this; }
		inline constexpr const tile& operator *() const { return *this; }
		inline constexpr tile  operator ->() const { return *this; }
		inline constexpr bool  operator ==(const iter& t) const { return ((b == t.b) & (i == t.i)); }
		inline constexpr bool  operator !=(const iter& t) const { return ((b != t.b) | (i != t.i)); }
		inline constexpr bool  operator < (const iter& t) const { return ((b == t.b) & (i < t.i)) | (b < t.b); }
		inline constexpr i32   operator - (const iter& t) const { return i32(i) - i32(t.i); }
		inline constexpr iter  operator + (u32 n) const { return iter(b, i + n); }
		inline constexpr iter  operator - (u32 n) const { return iter(b, i - n); }
		inline constexpr iter& operator +=(u32 n) { i += n; return *this; }
		inline constexpr iter& operator -=(u32 n) { i -= n; return *this; }
		inline constexpr iter& operator ++() { i += 1; return *this; }
		inline constexpr iter& operator --() { i -= 1; return *this; }
		inline constexpr iter  operator ++(int) { return iter(b, ++i - 1); }
		inline constexpr iter  operator --(int) { return iter(b, --i + 1); }
	protected:
		inline constexpr iter(const board& b, u32 i) : tile(b, i) {};
	};
	inline constexpr tile operator [](u32 i) const { return tile(*this, i); }
	inline constexpr iter begin() const { return iter(*this, 0); }
	inline constexpr iter end() const { return iter(*this, 16); }

	class style {
	public:
		style() = delete;
		enum fmtcode : u32 {
			index  = 0x00000000u,
			exact  = 0x10000000u,
			alter  = 0x20000000u,
			binary = 0x40000000u,
			extend = 0x80000000u,
			full   = 0xf0000000u,

			at     = index,
			at4    = index,
			at5    = index | extend,
			ext    = extend,
			exact4 = index | exact,
			exact5 = index | exact | extend,
			actual = index | exact | extend,
			lite   = alter,
			lite64 = alter,
			lite80 = alter | extend,
			raw    = binary,
			raw64  = binary,
			raw80  = binary | extend,
		};
	};
	inline constexpr board& format(u32 i = style::index) { info((i & style::full) | (inf & ~style::full)); return *this; }

	inline constexpr u32 info(u32 i) { u32 val = inf; inf = i; return val; }
	inline constexpr u32 info() const { return inf; }

	friend std::ostream& operator <<(std::ostream& out, const board& b) {
		if (b.inf & style::binary) {
			moporgic::write<u64>(out, b.raw);
			if (b.inf & style::extend) moporgic::write_cast<u16>(out, b.ext >> 16);
			if (b.inf & style::alter)  moporgic::write_cast<u16>(out, b.ext & 0xffff);
			if (b.inf & style::exact)  moporgic::write<u32>(out, b.inf);
		} else if (b.inf & style::alter) {
			char buf[32];
			std::snprintf(buf, sizeof(buf), "[%016" PRIx64 "]", b.raw);
			if (b.inf & style::extend) std::snprintf(buf + 17, sizeof(buf) - 17, "|%04x]", b.ext >> 16);
			out << buf;
		} else {
			char buf[64];
			u32 w = (b.inf & style::exact) ? 6 : 4;
			std::snprintf(buf, sizeof(buf), "+%.*s+", (w * 4), "------------------------");
			out << buf << std::endl;
			for (u32 i = 0; i < 16; i += 4) {
				u32 t[4] = { b[i + 0], b[i + 1], b[i + 2], b[i + 3] };
				std::snprintf(buf, sizeof(buf), "|%*u%*u%*u%*u|", w, t[0], w, t[1], w, t[2], w, t[3]);
				out << buf << std::endl;
			}
			std::snprintf(buf, sizeof(buf), "+%.*s+", (w * 4), "------------------------");
			out << buf << std::endl;
		}
		return out;
	}

	friend std::istream& operator >>(std::istream& in, board& b) {
		if (b.inf & style::binary) {
			moporgic::read<u64>(in, b.raw);
			if (b.inf & style::extend) moporgic::read_cast<u16>(in, raw_cast<u16, 1>(b.ext));
			if (b.inf & style::alter)  moporgic::read_cast<u16>(in, raw_cast<u16, 0>(b.ext));
			if (b.inf & style::exact)  moporgic::read<u32>(in, b.inf);
		} else if (b.inf & style::alter) {
			bool nobox(in >> std::hex >> b.raw);
			if (!nobox) (in.clear(), in.ignore(1)) >> std::hex >> b.raw;
			if (b.inf & style::extend) in.ignore(1) >> std::hex >> raw_cast<u16, 1>(b.ext);
			if (!nobox) in.ignore(1);
		} else {
			for (u32 k, i = 0; i < 16; i++) {
				for (k = 0; !(in >> k) && !in.eof(); in.clear(), in.ignore(1));
				b[i] = k;
			}
		}
		return in;
	}

};

} // namespace moporgic
