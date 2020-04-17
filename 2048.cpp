//============================================================================
// Name        : 2048.cpp
// Author      : Hung Guei
// Version     : beta
// Description : 2048
//============================================================================

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include "moporgic/type.h"
#include "moporgic/util.h"
#include "moporgic/math.h"
#include "moporgic/shm.h"
#include "board.h"
#include <vector>
#include <functional>
#include <memory>
#include <map>
#include <cmath>
#include <ctime>
#include <tuple>
#include <string>
#include <numeric>
#include <limits>
#include <cctype>
#include <iterator>
#include <sstream>
#include <iomanip>
#include <list>
#include <random>
#include <thread>
#include <future>
#if defined(__linux__)
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace moporgic {

typedef float numeric;

class weight {
public:
	inline weight() : name(), length(0), raw(nullptr) {}
	inline weight(const weight& w) = default;
	inline ~weight() {}

	typedef std::string sign_t;
	typedef moporgic::numeric numeric;
	typedef weight::numeric segment;

	inline sign_t sign() const { return name; }
	inline size_t size() const { return length; }
	constexpr inline segment& operator [](u64 i) { return raw[i]; }
	constexpr inline segment* data(u64 i = 0) { return raw + i; }
	constexpr inline clip<numeric> value() const { return { raw, raw + length }; }
	inline operator bool() const { return raw; }
	declare_comparators(const weight&, sign(), inline);

	friend std::ostream& operator <<(std::ostream& out, const weight& w) {
		u32 code = 4;
		write_cast<u8>(out, code);
		switch (code) {
		default:
		case 4:
			try { // write sign as 32-bit integer if possible
				size_t idx = 0;
				u32 sign = std::stoul(w.sign(), &idx, 16);
				if (idx != w.sign().size()) throw std::invalid_argument("unresolved");
				write_cast<u32>(out, sign);
				write_cast<u16>(out, w.sign().size());
				write_cast<u16>(out, 0);
			} catch (std::logic_error&) { // otherwise, write it as raw string
				out.write(w.sign().append(8, ' ').c_str(), 8);
			}
			// write value table
			write_cast<u16>(out, sizeof(weight::numeric));
			write_cast<u64>(out, w.size());
			write_cast<numeric>(out, w.value().begin(), w.value().end());
			// reserved for fields
			write_cast<u16>(out, 0);
			break;
		}
		return out;
	}
	friend std::istream& operator >>(std::istream& in, weight& w) {
		u32 code = 4;
		read_cast<u8>(in, code);
		switch (code) {
		case 0:
		case 1:
		case 2:
			// read name and size
			w.name = format("%08x", read<u32>(in));
			w.length = read<u64>(in);
			if (math::ones64(w.length) == 1) // remove extra beginning '0'
				w.name = w.name.substr(8 - (math::lg64(w.length) >> 2));
			// read value table
			w.raw = weight::alloc(w.length);
			switch ((code == 2) ? read<u16>(in) : (code == 1 ? 8 : 4)) {
			case 4: read_cast<f32>(in, w.value().begin(), w.value().end()); break;
			case 8: read_cast<f64>(in, w.value().begin(), w.value().end()); break;
			}
			break;
		default:
		case 4:
			// read name
			in.read(const_cast<char*>(w.name.assign(8, ' ').data()), 8);
			if (raw_cast<u16>(w.name[6]) == 0) { // name is written as integer
				u32 width = raw_cast<u16>(w.name[4]);
				if (width == 0) { // name display width is missing, try recalculate it
					read_cast<u64>(in.ignore(2), w.length).seekg(-10, std::ios::cur);
					width = (math::ones64(w.length) == 1) ? (math::lg64(w.length) >> 2) : 8;
					if (format("%x", raw_cast<u32>(w.name[0])).size() > width) width = 8;
				}
				w.name = format(format("%%0%ux", width), raw_cast<u32>(w.name[0]));
			} else { // name is written as string
				w.name = w.name.substr(0, w.name.find(' '));
			}
			// read value table
			read_cast<u16>(in, code);
			read_cast<u64>(in, w.length);
			w.raw = weight::alloc(w.length);
			switch (code) {
			case 2: read_cast<f16>(in, w.value().begin(), w.value().end()); break;
			case 4: read_cast<f32>(in, w.value().begin(), w.value().end()); break;
			case 8: read_cast<f64>(in, w.value().begin(), w.value().end()); break;
			}
			// ignore unrecognized extra fields
			while (read_cast<u16>(in, code) && code)
				in.ignore(code * read<u64>(in));
			break;
		}
		return in;
	}

	static void save(std::ostream& out) {
		u32 code = 0;
		write_cast<u8>(out, code);
		switch (code) {
		default:
		case 0:
			write_cast<u32>(out, wghts().size());
			for (weight w : wghts()) out << w;
			break;
		}
	}
	static void load(std::istream& in) {
		u32 code = 0;
		read_cast<u8>(in, code);
		switch (code) {
		default:
		case 0:
			for (read_cast<u32>(in, code); code; code--)
				in >> list<weight>::as(wghts()).emplace_back();
			break;
		}
	}

	class container : public clip<weight> {
	public:
		constexpr container() noexcept : clip<weight>() {}
		constexpr container(const clip<weight>& w) : clip<weight>(w) {}
	public:
		weight& make(sign_t sign, size_t size) { return list<weight>::as(*this).emplace_back(weight(sign, size)); }
		weight erase(sign_t sign) { auto it = find(sign); auto w = *it; free(it->data()); list<weight>::as(*this).erase(it); return w; }
		weight* find(sign_t sign) const { return std::find_if(begin(), end(), [=](const weight& w) { return w.sign() == sign; }); }
		weight& at(sign_t sign) const { auto it = find(sign); if (it != end()) return *it; throw std::out_of_range("weight::at"); }
		weight& operator[](sign_t sign) const { return (*find(sign)); }
		weight operator()(sign_t sign) const { auto it = find(sign); return it != end() ? *it : ({ weight w; w.name = sign; w; }); }
	};

	static inline weight::container& wghts() { static container w; return w; }
	static inline weight& make(sign_t sign, size_t size, container& src = wghts()) { return src.make(sign, size); }
	static inline size_t erase(sign_t sign, container& src = wghts()) { return src.erase(sign); }
	inline weight(sign_t sign, const container& src = wghts()) : weight(src(sign)) {}

private:
	inline weight(sign_t sign, size_t size) : name(sign), length(size), raw(alloc(size)) {}

	static inline segment* alloc(size_t size) { return shm::enable() ? shm::alloc<segment>(size) : new segment[size](); }
	static inline void free(segment* v) { shm::enable() ? shm::free(v) : delete[] v; }

	sign_t name;
	size_t length;
	segment* raw;
};

class indexer {
public:
	inline indexer() : name(), map(nullptr) {}
	inline indexer(const indexer& i) = default;
	inline ~indexer() {}

	typedef std::string sign_t;
	typedef u64(*mapper)(const board&);

	inline sign_t sign() const { return name; }
	constexpr inline mapper index() const { return map; }
	constexpr inline u64 operator ()(const board& b) const { return (*map)(b); }
	inline operator bool() const { return map; }
	declare_comparators(const indexer&, sign(), inline);

	class container : public clip<indexer> {
	public:
		constexpr container() noexcept : clip<indexer>() {}
		constexpr container(const clip<indexer>& i) : clip<indexer>(i) {}
	public:
		indexer& make(sign_t sign, mapper map) { return list<indexer>::as(*this).emplace_back(indexer(sign, map)); }
		indexer erase(sign_t sign) { auto it = find(sign); auto x = *it; list<indexer>::as(*this).erase(it); return x; }
		indexer* find(sign_t sign) const { return std::find_if(begin(), end(), [=](const indexer& i) { return i.sign() == sign; }); }
		indexer& at(sign_t sign) const { auto it = find(sign); if (it != end()) return *it; throw std::out_of_range("indexer::at"); }
		indexer& operator[](sign_t sign) const { return (*find(sign)); }
		indexer operator()(sign_t sign) const { auto it = find(sign); return it != end() ? *it : ({ indexer x; x.name = sign; x; }); }
	};

	static inline indexer::container& idxrs() { static container i; return i; }
	static inline indexer& make(sign_t sign, mapper map, container& src = idxrs()) { return src.make(sign, map); }
	static inline size_t erase(sign_t sign, container& src = idxrs()) { return src.erase(sign); }
	inline indexer(sign_t sign, const container& src = idxrs()) : indexer(src(sign)) {}

private:
	inline indexer(sign_t sign, mapper map) : name(sign), map(map) {}

	sign_t name;
	mapper map;
};

class feature {
public:
	inline feature() : name(), raw(), map() {}
	inline feature(const feature& t) = default;
	inline ~feature() {}

	typedef std::string sign_t;

	inline sign_t sign() const { return name; }
	constexpr inline weight::segment& operator [](const board& b) { return raw[map(b)]; }
	constexpr inline weight::segment& operator [](u64 idx) { return raw[idx]; }
	constexpr inline u64 operator ()(const board& b) const { return map(b); }

	inline indexer index() const { return map; }
	inline weight  value() const { return raw; }
	inline operator bool() const { return map && raw; }
	declare_comparators(const feature&, sign(), inline);

	class container : public clip<feature> {
	public:
		constexpr container() noexcept : clip<feature>() {}
		constexpr container(const clip<feature>& f) : clip<feature>(f) {}
	public:
		feature& make(sign_t wgt, sign_t idx) { return list<feature>::as(*this).emplace_back(feature(weight(wgt), indexer(idx))); }
		feature& make(sign_t sign) { return make(sign.substr(0, sign.find(':')), sign.substr(sign.find(':') + 1)); }
		feature erase(sign_t wgt, sign_t idx) { return erase(wgt + ':' + idx); }
		feature erase(sign_t sign) { auto it = find(sign); auto f = *it; list<feature>::as(*this).erase(it); return f; }
		feature* find(sign_t wgt, sign_t idx) const { return find(wgt + ':' + idx); }
		feature* find(sign_t sign) const { return std::find_if(begin(), end(), [=](const feature& f) { return f.sign() == sign; }); }
		feature& at(sign_t wgt, sign_t idx) const { return at(wgt + ':' + idx); }
		feature& at(sign_t sign) const { auto it = find(sign); if (it != end()) return *it; throw std::out_of_range("feature::at"); }
		feature& operator[](sign_t sign) const { return (*find(sign)); }
		feature operator()(sign_t wgt, sign_t idx) const { return operator()(wgt + ':' + idx); }
		feature operator()(sign_t sign) const { auto it = find(sign); return it != end() ? *it : ({ feature f; f.name = sign; f; }); }
	};

	static inline feature::container& feats() { static container f; return f; }
	static inline feature& make(sign_t wgt, sign_t idx, container& src = feats()) { return src.make(wgt, idx); }
	static inline size_t erase(sign_t wgt, sign_t idx, container& src = feats()) { return src.erase(wgt, idx); }
	inline feature(sign_t wgt, sign_t idx, const container& src = feats()) : feature(src(wgt, idx)) {}

private:
	inline feature(const weight& value, const indexer& index) : name(value.sign() + ':' + index.sign()), raw(value), map(index) {}

	sign_t name;
	weight raw;
	indexer map;
};

class cache {
public:
	class block {
	public:
		class access {
		public:
			constexpr access(u64 sign, u32 hold, block& blk) : hash(blk.hash), info(blk.info), sign(sign), hold(hold), blk(blk) {}
			constexpr access(u64 sign, u32 hold) : access(sign, hold, raw_cast<block>(*this)) {}
			constexpr access(access&& acc) : access(acc.sign, acc.hold, &(acc.blk) != &(raw_cast<block>(acc)) ? acc.blk : raw_cast<block>(*this)) {}
			constexpr access(const access&) = delete;
			constexpr access& operator =(const access&) = delete;

			constexpr operator bool() const {
				return ((hash ^ info) == sign) & (raw_cast<u16, 2>(info) >= hold);
			}
			constexpr numeric fetch() {
				numeric esti = raw_cast<f32, 0>(info);
				u64 sign = hash ^ info;
				raw_cast<u16, 3>(info) = std::min(raw_cast<u16, 3>(info) + 1, 65535);
				hash = sign ^ info;
				blk = raw_cast<block>(*this);
				return esti;
			}
			constexpr numeric store(numeric esti) {
				raw_cast<f32, 0>(info) = esti;
				raw_cast<u16, 2>(info) = hold;
				raw_cast<u16, 3>(info) = (hash ^ info) == sign ? raw_cast<u16, 3>(info) : 0;
				hash = sign ^ info;
				blk = raw_cast<block>(*this);
				return esti;
			}
		private:
			u64 hash;
			u64 info; // f32 esti; u16 hold; u16 hits;
			u64 sign;
			u32 hold;
			block& blk;
		};

		constexpr block(const block& e) = default;
		constexpr block(u64 sign = 0, u64 info = 0) : hash(sign ^ info), info(info) {}
		constexpr access operator()(u64 x, u32 n) { return access(x, n, *this); }
		constexpr u64 sign() const { return hash ^ info; }
		constexpr f32 esti() const { return raw_cast<f32, 0>(info); }
		constexpr u16 hold() const { return raw_cast<u16, 2>(info); }
		constexpr u16 hits() const { return raw_cast<u16, 3>(info); }

	private:
		u64 hash;
		u64 info; // f32 esti; u16 hold; u16 hits;
	};

	constexpr cache() : cached(&initial), length(1), mask(0), nmap{} {}
	cache(const cache& tp) = default;
	~cache() {}
	constexpr inline size_t size() const { return length; }
	constexpr inline block& operator[] (size_t i) { return cached[i]; }
	constexpr inline const block& operator[] (size_t i) const { return cached[i]; }
	inline block::access operator() (const board& b, u32 n) {
		u64 x = min_isomorphic(b);
		block& blk = operator[]((math::fmix64(x) ^ nmap[n >> 1]) & mask);
		return blk(x, n);
	}

    friend std::ostream& operator <<(std::ostream& out, const cache& c) {
		u32 code = 4;
		write_cast<byte>(out, code);
		switch (code) {
		default:
		case 4:
			// reserved for header
			write_cast<u16>(out, 0);
			write_cast<u64>(out, 0);
			// write blocks
			write_cast<u16>(out, sizeof(block));
			write_cast<u64>(out, c.size());
			write<block>(out, c.cached, c.cached + c.size());
			// write depth-map (nmap)
			write_cast<u16>(out, sizeof(size_t));
			write_cast<u64>(out, c.nmap.size());
			write_cast<u64>(out, c.nmap.begin(), c.nmap.end());
			// reserved for fields
			write_cast<u16>(out, 0);
			break;
		}
		return out;
	}
	friend std::istream& operator >>(std::istream& in, cache& c) {
		u32 code = 0;
		read_cast<byte>(in, code);
		switch (code) {
		default:
		case 4:
			// ignore unused header
			in.ignore(read<u16>(in) * read<u64>(in));
			// read blocks
			c.init(read<u64>(in.ignore(2)));
			read<block>(in, c.cached, c.cached + c.size());
			// read depth-map (nmap)
			read_cast<u64>(in, c.nmap.begin(), c.nmap.begin() + read<u64>(in.ignore(2)));
			// ignore unrecognized fields
			while ((code = read<u16>(in)) != 0)
				in.ignore(code * read<u64>(in));
			break;
		}
		return in;
	}

	static void save(std::ostream& out) {
		u32 code = 0;
		write_cast<byte>(out, code);
		switch (code) {
		default:
		case 0:
			out << instance();
			break;
		}
		out.flush();
	}
	static void load(std::istream& in) {
		u32 code;
		read_cast<byte>(in, code);
		switch (code) {
		default:
		case 0:
			in >> instance();
			break;
		}
	}

	static inline block::access find(const board& b, u32 n) { return instance()(b, n); }
	static inline cache& make(size_t len, bool peek = false) { return instance().init(std::max(len, size_t(1)), peek); }
	static inline cache& instance() { static cache tp; return tp; }

private:
	static inline block* alloc(size_t len) { return shm::enable() ? shm::alloc<block>(len) : new block[len](); }
	static inline void free(block* alloc) { shm::enable() ? shm::free(alloc) : delete[] alloc; }

	cache& init(size_t len, bool peek = false) {
		length = (1ull << (math::lg64(len)));
		mask = length - 1;
		if (cached != &initial) free(cached);
		cached = length > 1 ? alloc(length) : &initial;
		for (size_t i = 0; i < nmap.size(); i++)
			nmap[i] = peek ? 0 : math::fmix64(i);
		return *this;
	}

	constexpr inline u64 min_isomorphic(board t) const {
		u64 x = u64(t);
		t.mirror64();    x = std::min(x, u64(t));
		t.transpose64(); x = std::min(x, u64(t));
		t.mirror64();    x = std::min(x, u64(t));
		t.transpose64(); x = std::min(x, u64(t));
		t.mirror64();    x = std::min(x, u64(t));
		t.transpose64(); x = std::min(x, u64(t));
		t.mirror64();    x = std::min(x, u64(t));
		return x;
	}

private:
	block* cached;
	block initial;
	size_t length;
	size_t mask;
	std::array<size_t, 16> nmap;
};

namespace index {

template<u32... patt>
inline constexpr u32 order() {
	if (sizeof...(patt) > 8 || sizeof...(patt) == 0) return -1;
	constexpr u32 x[] = { patt... };
	for (u32 i = 1; i < sizeof...(patt); i++)
		if (x[i] <= x[i - 1]) return 0; // unordered or duplicated, e.g. {4,3,2,1} or {0,1,2,2}
	for (u32 i = 1; i < sizeof...(patt); i++)
		if (x[i] != x[i - 1] + 1) return 1; // ordered, e.g. {0,1,2,4}
	return 2; // strictly ordered, e.g. {0,1,2,3}
}

template<u32... patt>
inline constexpr typename std::enable_if<order<patt...>() == 0, u64>::type indexpt(const board& b) {
	constexpr u32 x[] = { patt... };
	register u32 index = 0;
	for (register u32 i = 0; i < sizeof...(patt); i++)
		index += b.at(x[i]) << (i << 2);
	return index;
}
template<u32... patt>
inline constexpr typename std::enable_if<order<patt...>() == 1, u64>::type indexpt(const board& b) {
	u64 mask = 0;
	for (u64 p : { patt... }) mask |= 0xfull << (p << 2);
	return math::pext64(b, mask);
}
template<u32 p, u32... x>
inline constexpr typename std::enable_if<order<p, x...>() == 2, u64>::type indexpt(const board& b) {
	return u32(u64(b) >> (p << 2)) & u32((1ull << ((sizeof...(x) + 1) << 2)) - 1);
}

u64 indexptv(const board& b, const std::vector<u32>& p) {
	register u64 index = 0;
	for (size_t i = 0; i < p.size(); i++)
		index += b.at(p[i]) << (i << 2);
	return index;
}

u64 indexmerge(const board& b) { // 16-bit
	board q = b; q.transpose();
	register u32 hori = 0, vert = 0;
	hori |= b.query(0).merge << 0;
	hori |= b.query(1).merge << 2;
	hori |= b.query(2).merge << 4;
	hori |= b.query(3).merge << 6;
	vert |= q.query(0).merge << 0;
	vert |= q.query(1).merge << 2;
	vert |= q.query(2).merge << 4;
	vert |= q.query(3).merge << 6;
	return hori | (vert << 8);
}

u64 indexnum(const board& b) { // 24-bit
	auto num = b.numof();
	register u64 index = 0;
	index |= (num[0] + num[1] + num[2] + num[3]) << 0; // 0+2+4+8, 4-bit
	index |= (num[4] + num[5] + num[6]) << 4; // 16+32+64, 4-bit
	index |= (num[7] + num[8]) << 8; // 128+256, 4-bit
	index |= std::min(u32(num[9] + num[10]), 7u) << 12; // 512+1024, 3-bit
	index |= std::min(u32(num[11]), 3u) << 15; // 2048~16384, 2-bit ea.
	index |= std::min(u32(num[12]), 3u) << 17;
	index |= std::min(u32(num[13]), 3u) << 19;
	index |= std::min(u32(num[14]), 3u) << 21;
	index |= std::min(u32(num[15]), 1u) << 23; // 32768, 1-bit
	return index;
}

u64 indexnumlt(const board& b) { // 24-bit
	auto num = b.numof();
	register u64 index = 0;
	index |= std::min(u32(num[8]),  7u) <<  0; // 256, 3-bit
	index |= std::min(u32(num[9]),  7u) <<  3; // 512, 3-bit
	index |= std::min(u32(num[10]), 7u) <<  6; // 1024, 3-bit
	index |= std::min(u32(num[11]), 7u) <<  9; // 2048, 3-bit
	index |= std::min(u32(num[12]), 7u) << 12; // 4096, 3-bit
	index |= std::min(u32(num[13]), 7u) << 15; // 8192, 3-bit
	index |= std::min(u32(num[14]), 7u) << 18; // 16384, 3-bit
	index |= std::min(u32(num[15]), 7u) << 21; // 32768, 3-bit
	return index;
}

u64 indexnumst(const board& b) { // 24-bit
	auto num = b.numof();
	register u64 index = 0;
	index |= std::min(u32(num[0]), 7u) <<  0; // 0, 3-bit
	index |= std::min(u32(num[1]), 7u) <<  3; // 2, 3-bit
	index |= std::min(u32(num[2]), 7u) <<  6; // 4, 3-bit
	index |= std::min(u32(num[3]), 7u) <<  9; // 8, 3-bit
	index |= std::min(u32(num[4]), 7u) << 12; // 16, 3-bit
	index |= std::min(u32(num[5]), 7u) << 15; // 32, 3-bit
	index |= std::min(u32(num[6]), 7u) << 18; // 64, 3-bit
	index |= std::min(u32(num[7]), 7u) << 21; // 128, 3-bit
	return index;
}

template<u32 p0, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6, u32 p7>
u64 indexmono(const board& b) { // 24-bit
	u32 h0 = (b.at(p0)) | (b.at(p1) << 4) | (b.at(p2) << 8) | (b.at(p3) << 12);
	u32 h1 = (b.at(p4)) | (b.at(p5) << 4) | (b.at(p6) << 8) | (b.at(p7) << 12);
	return (board::cache::load(h0).left.mono) | (board::cache::load(h1).left.mono << 12);
}

template<u32 tile, u32 isomorphic>
u64 indexmask(const board& b) { // 16-bit
	board k = b;
	k.isomorphic(isomorphic);
	return k.mask(tile);
}

template<u32 isomorphic>
u64 indexmax(const board& b) { // 16-bit
	board k = b;
	k.isomorphic(isomorphic);
	return k.mask(k.max());
}

struct adapter {
	static inline auto& wlist() { static moporgic::list<indexer::mapper> w; return w; }
	static inline auto& hlist() { static moporgic::list<std::function<u64(const board&)>> h; return h; }

	inline operator indexer::mapper() const { return wlist().front(); }
	inline adapter(std::function<u64(const board&)> hdr) { hlist().push_back(hdr); }
	inline ~adapter() { wlist().pop_front(); }

	template<u32 idx>
	static u64 adapt(const board& b) { return hlist()[idx](b); }

	template<u32 idx, u32 lim>
	static void make() { make_wrappers<idx, lim>(); }

	template<u32 idx, u32 lim>
	struct make_wrappers {
		make_wrappers() { wlist().push_back(adapter::adapt<idx>); }
		~make_wrappers() { make_wrappers<idx + 1, lim>(); }
	};
	template<u32 lim>
	struct make_wrappers<lim, lim> {};
};

struct make {
	make(indexer::sign_t sign, indexer::mapper func) {
		if (!indexer(sign)) indexer::make(sign, func);
		else if (indexer(sign).index() != func) std::exit(127);
	}

	template<u32... patt>
	struct indexpt {
		indexpt(bool iso = true) { isomorphic<0>(iso); }
		template<u32 i> static typename std::enable_if<(i != 8), void>::type isomorphic(bool iso) {
			constexpr board x = isoindex(i);
			make(vtos({x[patt]...}), index::indexpt<x[patt]...>);
			if (iso) isomorphic<i + 1>(iso);
		}
		template<u32 i> static typename std::enable_if<(i >= 8), void>::type isomorphic(bool iso) {}

		static constexpr board isoindex(u32 i) {
			board x = 0xfedcba9876543210ull;
			x.isomorphic((i & 4) + (8 - i) % 4);
			return x;
		}
		static std::string vtos(const std::initializer_list<u32>& v) {
			std::string name;
			for (u32 i : v) name += char((i < 10) ? ('0' + i) : ('a' + i - 10));
			return name;
		}
	};
};

__attribute__((constructor)) void init() {
	make::indexpt<0x0,0x1,0x2,0x3,0x4,0x5>(); // 012345!
	make::indexpt<0x4,0x5,0x6,0x7,0x8,0x9>(); // 456789!
	make::indexpt<0x0,0x1,0x2,0x4,0x5,0x6>(); // 012456!
	make::indexpt<0x4,0x5,0x6,0x8,0x9,0xa>(); // 45689a!
	make::indexpt<0x8,0x9,0xa,0xb,0xc,0xd>(); // 89abcd!
	make::indexpt<0x2,0x3,0x4,0x5,0x6,0x9>(); // 234569!
	make::indexpt<0x0,0x1,0x2,0x5,0x9,0xa>(); // 01259a!
	make::indexpt<0x3,0x4,0x5,0x6,0x7,0x8>(); // 345678!
	make::indexpt<0x1,0x3,0x4,0x5,0x6,0x7>(); // 134567!
	make::indexpt<0x0,0x1,0x4,0x8,0x9,0xa>(); // 01489a!

	make::indexpt<0x0,0x1,0x2,0x3,0x4>(); // 01234!
	make::indexpt<0x4,0x5,0x6,0x7,0x8>(); // 45678!
	make::indexpt<0x0,0x1,0x2,0x4,0x5>(); // 01245!
	make::indexpt<0x4,0x5,0x6,0x8,0x9>(); // 45689!
	make::indexpt<0x0,0x1,0x2,0x3,0x5>(); // 01235!
	make::indexpt<0x4,0x5,0x6,0x7,0x9>(); // 45679!
	make::indexpt<0x8,0x9,0xa,0xb,0xc>(false); // 89abc
	make::indexpt<0x8,0x9,0xa,0xc,0xd>(false); // 89acd
	make::indexpt<0x8,0x9,0xa,0xb,0xd>(false); // 89abd
	make::indexpt<0x1,0x2,0x3,0x5,0x6>(false); // 12356
	make::indexpt<0x5,0x6,0x7,0x9,0xa>(false); // 5679a
	make::indexpt<0x9,0xa,0xb,0xd,0xe>(false); // 9abde

	make::indexpt<0x0,0x1,0x2,0x3>(); // 0123!
	make::indexpt<0x4,0x5,0x6,0x7>(); // 4567!
	make::indexpt<0x0,0x1,0x4,0x5>(); // 0145!
	make::indexpt<0x1,0x2,0x5,0x6>(); // 1256!
	make::indexpt<0x5,0x6,0x9,0xa>(); // 569a!
	make::indexpt<0x8,0x9,0xa,0xb>(false); // 89ab
	make::indexpt<0xc,0xd,0xe,0xf>(false); // cdef
	make::indexpt<0x0,0x4,0x8,0xc>(false); // 048c
	make::indexpt<0x1,0x5,0x9,0xd>(false); // 159d
	make::indexpt<0x2,0x6,0xa,0xe>(false); // 26ae
	make::indexpt<0x3,0x7,0xb,0xf>(false); // 37bf
	make::indexpt<0x0,0x1,0x2,0x4>(false); // 0124
	make::indexpt<0x1,0x2,0x3,0x5>(false); // 1235
	make::indexpt<0x4,0x5,0x6,0x8>(false); // 4568
	make::indexpt<0x5,0x6,0x7,0x9>(false); // 5679
	make::indexpt<0x8,0x9,0xa,0xc>(false); // 89ac
	make::indexpt<0x9,0xa,0xb,0xd>(false); // 9abd
	make::indexpt<0x0,0x1,0x2,0x5>(false); // 0125
	make::indexpt<0x4,0x5,0x6,0x9>(false); // 4569
	make::indexpt<0x8,0x9,0xa,0xd>(false); // 89ad

	make::indexpt<0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7>(); // 01234567!
	make::indexpt<0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb>(); // 456789ab!
	make::indexpt<0x0,0x1,0x2,0x4,0x5,0x6,0x8,0x9>(); // 01245689!
	make::indexpt<0x0,0x1,0x2,0x3,0x4,0x5,0x8,0xc>(); // 0123458c!

	make::indexpt<0x0,0x1,0x2,0x3,0x4,0x5,0x6>(); // 0123456!
	make::indexpt<0x4,0x5,0x6,0x7,0x8,0x9,0xa>(); // 456789a!
	make::indexpt<0x8,0x9,0xa,0xb,0xc,0xd,0xe>(); // 89abcde!
	make::indexpt<0x0,0x1,0x2,0x3,0x4,0x5,0x8>(); // 0123458!
	make::indexpt<0x4,0x5,0x6,0x7,0x8,0x9,0xc>(); // 456789c!

	make("merge",  indexmerge);
	make("num",    indexnum);
	make("num@lt", indexnumlt);
	make("num@st", indexnumst);

	make("m@0123", indexmono<0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7>);
	make("m@37bf", indexmono<0x3,0x7,0xb,0xf,0x2,0x6,0xa,0xe>);
	make("m@fedc", indexmono<0xf,0xe,0xd,0xc,0xb,0xa,0x9,0x8>);
	make("m@c840", indexmono<0xc,0x8,0x4,0x0,0xd,0x9,0x5,0x1>);
	make("m@3210", indexmono<0x3,0x2,0x1,0x0,0x7,0x6,0x5,0x4>);
	make("m@fb73", indexmono<0xf,0xb,0x7,0x3,0xe,0xa,0x6,0x2>);
	make("m@cdef", indexmono<0xc,0xd,0xe,0xf,0x8,0x9,0xa,0xb>);
	make("m@048c", indexmono<0x0,0x4,0x8,0xc,0x1,0x5,0x9,0xd>);
	make("m@4567", indexmono<0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb>);
	make("m@26ae", indexmono<0x2,0x6,0xa,0xe,0x1,0x5,0x9,0xd>);
	make("m@ba98", indexmono<0xb,0xa,0x9,0x8,0x7,0x6,0x5,0x4>);
	make("m@d951", indexmono<0xd,0x9,0x5,0x1,0xe,0xa,0x6,0x2>);
	make("m@7654", indexmono<0x7,0x6,0x5,0x4,0xb,0xa,0x9,0x8>);
	make("m@ea62", indexmono<0xe,0xa,0x6,0x2,0xd,0x9,0x5,0x1>);
	make("m@89ab", indexmono<0x8,0x9,0xa,0xb,0x4,0x5,0x6,0x7>);
	make("m@159d", indexmono<0x1,0x5,0x9,0xd,0x2,0x6,0xa,0xe>);

	make("max#0", indexmax<0>);
	make("max#1", indexmax<1>);
	make("max#2", indexmax<2>);
	make("max#3", indexmax<3>);
	make("max#4", indexmax<4>);
	make("max#5", indexmax<5>);
	make("max#6", indexmax<6>);
	make("max#7", indexmax<7>);

	adapter::make<0, 256>();
}

} // namespace utils

namespace utils {

class options {
public:
	options() {}
	options(const options& opts) : opts(opts.opts) {}
	typedef std::list<std::string> list;

	class opinion {
		friend class option;
	public:
		opinion() = delete;
		opinion(const opinion& i) = default;
		opinion(std::string& token) : token(token) {}
		operator std::string() const { return value(); }
		operator numeric() const { return std::stod(value()); }
		std::string label() const { return token.substr(0, token.find('=')); }
		std::string value() const { return token.find('=') != std::string::npos ? token.substr(token.find('=') + 1) : ""; }
		std::string value(const std::string& def) const { auto val = value(); return val.size() ? val : def; }
		numeric value(numeric def) const { try { return numeric(*this); } catch (std::invalid_argument&) {} return def; }
		std::string operator +(const std::string& val) { return value() + val; }
		friend std::string operator +(const std::string& val, const opinion& opi) { return val + opi.value(); }
		friend std::ostream& operator <<(std::ostream& out, const opinion& i) { return out << i.value(); }
		opinion& operator  =(const opinion& opi) { return operator =(opi.value()); }
		opinion& operator  =(const numeric& val) { return operator =(ntos(val)); }
		opinion& operator  =(const std::string& val) { token = label() + (val.size() ? ("=" + val) : ""); return (*this); }
		opinion& operator +=(const std::string& val) { return operator =(value() + val); }
		opinion& operator  =(const list& vec) { return operator =(vtos(vec)); }
		opinion& operator +=(const list& vec) { return operator =(value() + vtos(vec)); }
		bool operator ==(const std::string& val) const { return value() == val; }
		bool operator !=(const std::string& val) const { return value() != val; }
		bool operator ()(const std::string& val) const { return value().find(val) != std::string::npos; }
		static bool comp(std::string token, std::string label) { return opinion(token).label() == label; }
	private:
		std::string& token;
	};

	class option : public list {
		friend class options;
	public:
		option(const list& opt = {}) : list(opt) {}
		operator std::string() const { return value(); }
		operator numeric() const { return std::stod(value()); }
		std::string value() const { return vtos(*this); }
		std::string value(const std::string& def) const { auto val = value(); return val.size() ? val : def; }
		numeric value(numeric def) const { try { return numeric(*this); } catch (std::invalid_argument&) {} return def; }
		std::string operator +(const std::string& val) { return value() + val; }
		friend std::string operator +(const std::string& val, const option& opt) { return val + opt.value(); }
		friend std::ostream& operator <<(std::ostream& out, const option& opt) { return out << opt.value(); }
		option& operator  =(const numeric& val) { return operator =(ntos(val)); }
		option& operator  =(const std::string& val) { clear(); return operator +=(val); }
		option& operator +=(const std::string& val) { push_back(val); return *this; }
		option& operator  =(const list& vec) { clear(); return operator +=(vec); }
		option& operator +=(const list& vec) { insert(end(), vec.begin(), vec.end()); return *this; }
		bool operator ==(const std::string& val) const { return value() == val; }
		bool operator !=(const std::string& val) const { return value() != val; }
		bool operator ()(const std::string& ext) const {
			return std::find_if(cbegin(), cend(), std::bind(opinion::comp, std::placeholders::_1, ext)) != cend();
		}
		bool operator ()(const std::string& ext, const std::string& val) const {
			return operator ()(ext) && const_cast<option&>(*this)[ext](val);
		}
		opinion operator [](const std::string& ext) {
			auto pos = std::find_if(begin(), end(), std::bind(opinion::comp, std::placeholders::_1, ext));
			return (pos != end()) ? opinion(*pos) : operator +=(ext)[ext];
		}
		std::string find(const std::string& ext, const std::string& val = {}) const {
			return operator() (ext) ? const_cast<option&>(*this)[ext] : val;
		}
	};

	bool operator ()(const std::string& opt) const {
		return opts.find(opt) != opts.end();
	}
	bool operator ()(const std::string& opt, const std::string& ext) const {
		return operator ()(opt) && const_cast<options&>(*this)[opt](ext);
	}
	bool operator ()(const std::string& opt, const std::string& ext, const std::string& val) const {
		return operator ()(opt, ext) && const_cast<options&>(*this)[opt][ext](val);
	}
	option& operator [](const std::string& opt) {
		if (opts.find(opt) == opts.end()) opts[opt] = option();
		return opts[opt];
	}
	std::string find(const std::string& opt, const std::string& val = {}) const {
		return operator()(opt) ? const_cast<options&>(*this)[opt] : val;
	}

private:
	std::map<std::string, option> opts;

	static std::string vtos(const list& vec) {
		std::string str = std::accumulate(vec.cbegin(), vec.cend(), std::string(),
		    [](std::string& r, const std::string& v){ return std::move(r) + v + " "; });
		if (str.size()) str.pop_back();
		return str;
	}
	static std::string ntos(const numeric& num) {
		std::string val = std::to_string(num);
		if (val.find('.') != std::string::npos) {
			while (val.back() == '0') val.pop_back();
			if (val.back() == '.') val.pop_back();
		}
		return val;
	}
};

void init_logging(utils::options::option opt) {
	static std::ofstream logout;
	for (std::string path : opt) {
		char type = path[path.find_last_of(".") + 1];
		if (logout.is_open() || (type != 'x' && type != 'l')) continue; // .x and .log are suffix for log files
		logout.open(path, std::ios::out | std::ios::app);
	}
	if (!logout.is_open()) return;
	static moporgic::teestream tee(std::cout, logout);
	static moporgic::redirector redirect(tee, std::cout);
}

void config_thread(utils::options::option opt) {
	shm::enable(shm::support() && !opt("noshm") && (opt("shm") || opt.value(1) > 1));
}

template<typename statistic>
statistic invoke(statistic(*run)(utils::options,std::string), utils::options opts, std::string type) {
#if defined(__linux__)
	if (shm::enable()) {
		u32 thdnum = std::stol(opts[type]["thread"] = opts["thread"].value(1)), thdid = thdnum;
		statistic* stats = shm::alloc<statistic>(thdnum);
		while (std::stol(opts[type]["thread#"] = --thdid) && fork());
		statistic stat = stats[thdid] = run(opts, type);
		if (thdid == 0) while (wait(nullptr) > 0); else std::quick_exit(0);
		for (u32 i = 1; i < thdnum; i++) stat += stats[i];
		shm::free(stats);
		return stat;
	}
#endif
	std::list<std::future<statistic>> thdpool;
	u32 thdid = std::stol(opts[type]["thread"] = opts["thread"].value(1));
	while (std::stol(opts[type]["thread#"] = (--thdid)))
		thdpool.push_back(std::async(std::launch::async, run, opts, type));
	statistic stat = run(opts, type);
	for (std::future<statistic>& thd : thdpool) stat += thd.get();
	return stat;
}

std::string resolve(const std::string& token) {
	std::map<std::string, std::string> alias;

	alias["4x6patt/khyeh"]       = "012345:012345! 456789:456789! 012456:012456! 45689a:45689a! ";
	alias["5x6patt/42-33"]       = "012345:012345! 456789:456789! 89abcd:89abcd! 012456:012456! 45689a:45689a! ";
	alias["2x4patt/4"]           = "0123:0123! 4567:4567! ";
	alias["5x4patt/4-22"]        = alias["2x4patt/4"] + "0145:0145! 1256:1256! 569a:569a! ";
	alias["8x4patt/legacy"]      = "0123 4567 89ab cdef 048c 159d 26ae 37bf ";
	alias["1x8patt/44"]          = "01234567:01234567! ";
	alias["2x8patt/44"]          = "01234567:01234567! 456789ab:456789ab! ";
	alias["3x8patt/44-332"]      = alias["2x8patt/44"] + "01245689:01245689! ";
	alias["3x8patt/44-4211"]     = alias["2x8patt/44"] + "0123458c:0123458c! ";
	alias["4x8patt/44-332-4211"] = alias["3x8patt/44-332"] + "0123458c:0123458c! ";
	alias["2x7patt/43"]          = "0123456:0123456! 456789a:456789a! ";
	alias["3x7patt/43"]          = alias["2x7patt/43"] + "89abcde:89abcde! ";
	alias["4x6patt/k.matsuzaki"] = "012456:012456! 456789:456789! 012345:012345! 234569:234569! ";
	alias["5x6patt/k.matsuzaki"] = alias["4x6patt/k.matsuzaki"] + "01259a:01259a! ";
	alias["6x6patt/k.matsuzaki"] = alias["5x6patt/k.matsuzaki"] + "345678:345678! ";
	alias["7x6patt/k.matsuzaki"] = alias["6x6patt/k.matsuzaki"] + "134567:134567! ";
	alias["8x6patt/k.matsuzaki"] = alias["7x6patt/k.matsuzaki"] + "01489a:01489a! ";

	alias["4x6patt"]   = alias["4x6patt/khyeh"];
	alias["5x6patt"]   = alias["5x6patt/42-33"];
	alias["2x4patt"]   = alias["2x4patt/4"];
	alias["5x4patt"]   = alias["5x4patt/4-22"];
	alias["8x4patt"]   = alias["8x4patt/legacy"];
	alias["1x8patt"]   = alias["1x8patt/44"];
	alias["2x8patt"]   = alias["2x8patt/44"];
	alias["3x8patt"]   = alias["3x8patt/44-4211"];
	alias["4x8patt"]   = alias["4x8patt/44-332-4211"];
	alias["2x7patt"]   = alias["2x7patt/43"];
	alias["3x7patt"]   = alias["3x7patt/43"];
	alias["6x6patt"]   = alias["6x6patt/k.matsuzaki"];
	alias["7x6patt"]   = alias["7x6patt/k.matsuzaki"];
	alias["8x6patt"]   = alias["8x6patt/k.matsuzaki"];

	alias["mono/0123"] = "m@0123[^24]:m@0123,m@37bf,m@fedc,m@c840,m@3210,m@fb73,m@cdef,m@048c ";
	alias["mono/4567"] = "m@4567[^24]:m@4567,m@26ae,m@ba98,m@d951,m@7654,m@ea62,m@89ab,m@159d ";
	alias["mono"]      = alias["mono/0123"] + alias["mono/4567"];
	alias["num@lt"]    = "num@lt[^24]:num@lt ";
	alias["num@st"]    = "num@st[^24]:num@st ";
	alias["num"]       = alias["num@lt"] + alias["num@st"];
	alias["default"]   = alias["4x6patt"];
	alias["none"]      = "";

	try { return alias.at(token);
	} catch (std::out_of_range&) { return token; }
}

void make_network(utils::options::option opt) {
	std::string tokens = opt;
	if (tokens.empty() && feature::feats().empty())
		tokens = "default";

	const auto npos = std::string::npos;
	for (size_t i; (i = tokens.find(" norm")) != npos; tokens[i] = '/');

	std::stringstream unalias(tokens); tokens.clear();
	for (std::string token; unalias >> token; tokens += (token + ' ')) {
		if (token.find(':') != npos) continue;
		std::string name = token.substr(0, token.find_first_of("&|="));
		std::string info = token != name ? token.substr(name.size()) : "";
		if ((token = utils::resolve(name)).empty() || info.empty()) continue;

		std::string winfo, iinfo, buff;
		for (char set : std::string("&|=")) {
			if (info.find(set) != npos) buff = info.substr(0, info.find_first_of("&|=", info.find(set) + 1)).substr(info.find(set));
			if (buff.find(set) != npos) winfo += buff.substr(0, buff.find(':'));
			if (buff.find(':') != npos) iinfo += buff.substr(buff.find(':') + 1).insert(0, 1, set);
		}
		std::stringstream rephrase(token); token.clear();
		for (std::string wtok, itok; rephrase >> buff; token += (wtok + itok + ' ')) {
			wtok = buff.substr(0, buff.find(':')) + winfo;
			itok = buff.find(':') != npos ? buff.substr(buff.find(':')) + iinfo : "";
		}
	}

	std::stringstream uncomma(tokens); tokens.clear();
	for (std::string token; uncomma >> token; tokens += (token + ' ')) {
		if (token.find(':') == npos || token.find(',') == npos) continue;
		for (size_t i; (i = token.find(',')) != npos; token[i] = ' ');
		std::stringstream lbuf(token.substr(0, token.find(':')));
		std::stringstream rbuf(token.substr(token.find(':') + 1));
		std::vector<std::string> lvals, rvals;
		for (std::string val; lbuf >> val; lvals.push_back(val));
		for (std::string val; rbuf >> val; rvals.push_back(val));
		token.clear();
		for (auto lval : lvals) for (auto rval : rvals) {
			token += (lval + ':' + rval + ' ');
			lval = lval.substr(0, lval.find('='));
		}
	}

	auto stov = [](std::string hash, u32 iso = 0) -> std::vector<u32> {
		std::vector<u32> patt;
		board x(0xfedcba9876543210ull); x.isomorphic(-iso);
		for (char tile : hash) patt.push_back(x.at((tile & 15) + (tile & 64 ? 9 : 0)));
		return patt;
	};
	auto vtos = [](std::vector<u32> patt) -> std::string {
		std::string hash;
		for (u32 tile : patt) hash.push_back(tile <= 9 ? tile + '0' : tile - 10 + 'a');
		return hash;
	};

	std::stringstream unisomorphic(tokens); tokens.clear();
	for (std::string token; unisomorphic >> token; tokens += (token + ' ')) {
		if (token.find('!') == npos) continue;
		std::vector<std::string> lvals, rvals;
		lvals.push_back(token.substr(0, token.find(':')));
		rvals.push_back(token.find(':') != npos ? token.substr(token.find(':')) : "");
		if (lvals.back().find('!') != npos) {
			std::string lval = lvals.back(); lvals.clear();
			std::string hash = lval.substr(0, lval.find('!'));
			std::string tail = lval.substr(lval.find('!') + 1);
			for (u32 iso = 0; iso < 8; iso++) lvals.push_back(vtos(stov(hash, iso)) + tail);
		}
		if (rvals.back().find('!') != npos) {
			std::string rval = rvals.back(); rvals.clear();
			std::string hash = rval.substr(0, rval.find('!')).substr(1);
			std::string tail = rval.substr(rval.find('!') + 1);
			for (u32 iso = 0; iso < 8; iso++) rvals.push_back(':' + vtos(stov(hash, iso)) + tail);
		}
		lvals.resize(8, lvals.back().substr(0, lvals.back().find('=')));
		rvals.resize(8, rvals.front());
		token.clear();
		for (size_t iso = 0; iso < 8; iso++) token += (lvals[iso] + rvals[iso] + ' ');
	}

	size_t num = 0;
	std::stringstream counter(tokens);
	for (std::string token; counter >> token; num++);

	std::stringstream parser(tokens);
	std::string token;
	while (parser >> token) {
		std::string wght, idxr;
		std::string wtok = token.substr(0, token.find(':'));
		std::string itok = token.substr(token.find(':') + 1);

		if (wtok.size()) {
			// allocate: weight[size] weight(size)
			// initialize: ...=0 ...=10000 ...=100000+norm
			// map or remove: destination={source} id={}
			for (size_t i; (i = wtok.find_first_of("[]()")) != npos; wtok[i] = ' ');
			std::string name = wtok.substr(0, wtok.find_first_of("!&|:= "));
			std::string info = wtok.find(' ') != npos ? wtok.substr(0, wtok.find_first_of("!&|:=")).substr(wtok.find(' ') + 1) : "?";
			std::string init = wtok.find('=') != npos ? wtok.substr(wtok.find('=') + 1) : "?";
			std::string sign = wtok.find('&') == npos && wtok.find('|') == npos ? name : ({
				std::stringstream ss;
				u64 mska = wtok.find('&') != npos ? std::stoull(wtok.substr(wtok.find('&') + 1), nullptr, 16) : -1ull;
				u64 msko = wtok.find('|') != npos ? std::stoull(wtok.substr(wtok.find('|') + 1), nullptr, 16) : 0ull;
				ss << std::hex << std::setfill('0') << std::setw(8) << ((std::stoull(name, nullptr, 16) & mska) | msko);
				ss.str();
			});
			size_t size = 0;
			if (info.find_first_of("p^?") != npos) { // ^10 16^10 16^ p ?
				u32 base = 16, power = name.size();
				if (info.find('^') != npos) {
					base = info.front() != '^' ? std::stoul(info.substr(0, info.find('^'))) : 2;
					power = info.back() != '^' ? std::stoul(info.substr(info.find('^') + 1)) : name.size();
				}
				size = std::pow(base, power);
			} else if (info.find_first_not_of("0123456789.-x") == npos) {
				size = std::stoull(info, nullptr, 0);
			}
			if (init.find_first_of("{}") != npos && init != "{}") {
				weight src(init.substr(0, init.find('}')).substr(init.find('{') + 1));
				size = std::max(size, src.size());
			} else if (init == "{}") {
				size = 0;
			}
			if (weight(sign) && weight(sign).size() != size)
				weight::erase(sign);
			if (!weight(sign) && size) { // weight id matching: 012345 <--> 00012345
				weight test(std::string(std::max(8 - sign.size(), size_t(8)), '0') + sign);
				while (!test && (test.sign() + ' ')[0] == '0') test = weight(test.sign().substr(1));
				if (test.size() == size) raw_cast<std::string>(weight::wghts().at(test.sign())) = sign; // unsafe!
			}
			if (!weight(sign) && size) { // create new weight table
				weight dst = weight::make(sign, size);
				if (init.find_first_of("{}") != npos && init != "{}") {
					weight src(init.substr(0, init.find('}')).substr(init.find('{') + 1));
					for (size_t n = 0; n < dst.size(); n += src.size()) {
						std::copy_n(src.data(), src.size(), dst.data() + n);
					}
				} else if (init.find_first_of("0123456789.+-") == 0) {
					numeric val = std::stod(init) * (init.find("norm") != npos ? std::pow(num, -1) : 1);
					std::fill_n(dst.data(), dst.size(), val);
				}
			} else if (weight(sign) && size) { // table already exists
				weight dst = weight(sign);
				if (init.find_first_of("+-") == 0) {
					numeric off = std::stod(init) * (init.find("norm") != npos ? std::pow(num, -1) : 1);
					std::transform(dst.data(), dst.data() + dst.size(), dst.data(), [=](numeric val) { return val += off; });
				}
			}
			wght = weight(sign).sign();
		}

		if (itok.size()) {
			// indexer indexer,indexer,indexer pattern! signature&mask|mask
			std::string name = itok.substr(0, itok.find_first_of("!&|"));
			std::string sign = itok.find('&') == npos && itok.find('|') == npos ? name : ({
				std::stringstream ss;
				u64 mska = itok.find('&') != npos ? std::stoull(itok.substr(itok.find('&') + 1), nullptr, 16) : -1ull;
				u64 msko = itok.find('|') != npos ? std::stoull(itok.substr(itok.find('|') + 1), nullptr, 16) : 0ull;
				ss << std::hex << std::setfill('0') << std::setw(8) << ((std::stoull(name, nullptr, 16) & mska) | msko);
				ss.str();
			});
			if (!indexer(sign)) {
				indexer::mapper index = indexer(name).index();
				if (!index) index = index::adapter(std::bind(index::indexptv, std::placeholders::_1, stov(name)));
				indexer::make(sign, index);
			}
			idxr = indexer(sign).sign();
		}

		if (wght.size() && idxr.size() && !feature(wght, idxr)) feature::make(wght, idxr);
	}
}
void load_network(utils::options::option opt) {
	for (std::string path : opt) {
		std::ifstream in;
		in.open(path, std::ios::in | std::ios::binary);
		while (in.peek() != -1) {
			char type = in.peek();
			if (type != 0) { // new binaries already store its type, so use it for the later loading
				in.ignore(1);
			} else { // legacy binaries always beginning with 0, so use name suffix to determine the type
				type = path[path.find_last_of(".") + 1];
			}
			if (type == 'w')  weight::load(in);
			if (type == 'c')   cache::load(in);
		}
		in.close();
	}
}
void save_network(utils::options::option opt) {
	char buf[1 << 20];
	for (std::string path : opt) {
		char type = path[path.find_last_of(".") + 1];
		if (type == 'x' || type == 'l') continue; // .x and .log are suffix for log files
		std::ofstream out;
		out.rdbuf()->pubsetbuf(buf, sizeof(buf));
		out.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) continue;
		// for upward compatibility, we still write legacy binaries for traditional suffixes
		if (type != 'c') {
			weight::save(type != 'w' ? out.write("w", 1) : out);
		} else { // .c is reserved for cache binary
			cache::save(type != 'c' ? out.write("c", 1) : out);
		}
		out.flush();
		out.close();
	}
}

void list_network() {
	if (weight::wghts().empty()) return;

	for (weight w : weight::wghts()) {
		std::stringstream buf;

		buf << w.sign();
		buf << "[";
		if (w.size() >> 30)
			buf << (w.size() >> 30) << "G";
		else if (w.size() >> 20)
			buf << (w.size() >> 20) << "M";
		else if (w.size() >> 10)
			buf << (w.size() >> 10) << "k";
		else
			buf << (w.size());
		buf << "]";

		buf << " :";
		std::ios::pos_type pos = buf.tellp();
		for (feature f : feature::feats())
			if (f.value() == w)
				buf << " " << f.index().sign();
		if (buf.tellp() == pos)
			buf << " (n/a)";

		std::cout << buf.rdbuf() << std::endl;
	}
	std::cout << std::endl;
}

void init_cache(utils::options::option opt) {
	if (opt.value(0) == 0) return;

	std::string res(opt);
	size_t unit = 0, size = std::stoull(res, &unit);
	if (unit < res.size())
		switch (std::toupper(res[unit])) {
		case 'K': size *= ((1ULL << 10) / sizeof(cache::block)); break;
		case 'M': size *= ((1ULL << 20) / sizeof(cache::block)); break;
		case 'G': size *= ((1ULL << 30) / sizeof(cache::block)); break;
		}
	bool peek = opt("peek") & !opt("nopeek");
	cache::make(size, peek);
}

} // utils


struct method {
	typedef numeric(*estimator)(const board&, clip<feature>);
	typedef numeric(*optimizer)(const board&, numeric, clip<feature>);

	estimator estim;
	optimizer optim;
	constexpr inline method(estimator estim = estimate, optimizer optim = optimize) : estim(estim), optim(optim) {}
	constexpr inline operator estimator() const { return estim; }
	constexpr inline operator optimizer() const { return optim; }

	constexpr static inline numeric estimate(const board& state, clip<feature> range = feature::feats()) {
		register numeric esti = 0;
		for (register feature& feat : range)
			esti += feat[state];
		return esti;
	}
	constexpr static inline numeric optimize(const board& state, numeric error, clip<feature> range = feature::feats()) {
		register numeric esti = 0;
		for (register feature& feat : range)
			esti += (feat[state] += error);
		return esti;
	}
	constexpr static inline numeric illegal(const board& state, clip<feature> range = feature::feats()) {
		return -std::numeric_limits<numeric>::max();
	}

	struct isomorphic {
		constexpr inline operator method() { return { isomorphic::estimate, isomorphic::optimize }; }

		constexpr static inline_always numeric invoke(const board& iso, clip<feature> f) {
			register numeric esti = 0;
			for (register auto feat = f.begin(); feat != f.end(); feat += 8)
				esti += (*feat)[iso];
			return esti;
		}
		constexpr static inline_always numeric invoke(const board& iso, numeric updv, clip<feature> f) {
			register numeric esti = 0;
			for (register auto feat = f.begin(); feat != f.end(); feat += 8)
				esti += ((*feat)[iso] += updv);
			return esti;
		}

		template<estimator estim = isomorphic::invoke>
		constexpr static inline numeric estimate(const board& state, clip<feature> range = feature::feats()) {
			register numeric esti = 0;
			register board iso;
			esti += estim(({ iso = state;     iso; }), range);
			esti += estim(({ iso.mirror();    iso; }), range);
			esti += estim(({ iso.transpose(); iso; }), range);
			esti += estim(({ iso.mirror();    iso; }), range);
			esti += estim(({ iso.transpose(); iso; }), range);
			esti += estim(({ iso.mirror();    iso; }), range);
			esti += estim(({ iso.transpose(); iso; }), range);
			esti += estim(({ iso.mirror();    iso; }), range);
			return esti;
		}
		template<optimizer optim = isomorphic::invoke>
		constexpr static inline numeric optimize(const board& state, numeric updv, clip<feature> range = feature::feats()) {
			register numeric esti = 0;
			register board iso;
			esti += optim(({ iso = state;     iso; }), updv, range);
			esti += optim(({ iso.mirror();    iso; }), updv, range);
			esti += optim(({ iso.transpose(); iso; }), updv, range);
			esti += optim(({ iso.mirror();    iso; }), updv, range);
			esti += optim(({ iso.transpose(); iso; }), updv, range);
			esti += optim(({ iso.mirror();    iso; }), updv, range);
			esti += optim(({ iso.transpose(); iso; }), updv, range);
			esti += optim(({ iso.mirror();    iso; }), updv, range);
			return esti;
		}

		template<indexer::mapper... indexes>
		struct static_index {
			constexpr static std::array<indexer::mapper, sizeof...(indexes)> index = { indexes... };
			constexpr inline operator method() { return { static_index::estimate, static_index::optimize }; }

			template<indexer::mapper index, indexer::mapper... follow> constexpr static
			inline_always typename std::enable_if<(sizeof...(follow) != 0), numeric>::type invoke(const board& iso, clip<feature> f) {
				return (f[(sizeof...(indexes) - sizeof...(follow) - 1) << 3][index(iso)]) + invoke<follow...>(iso, f);
			}
			template<indexer::mapper index, indexer::mapper... follow> constexpr static
			inline_always typename std::enable_if<(sizeof...(follow) == 0), numeric>::type invoke(const board& iso, clip<feature> f) {
				return (f[(sizeof...(indexes) - sizeof...(follow) - 1) << 3][index(iso)]);
			}
			template<indexer::mapper index, indexer::mapper... follow> constexpr static
			inline_always typename std::enable_if<(sizeof...(follow) != 0), numeric>::type invoke(const board& iso, numeric updv, clip<feature> f) {
				return (f[(sizeof...(indexes) - sizeof...(follow) - 1) << 3][index(iso)] += updv) + invoke<follow...>(iso, updv, f);
			}
			template<indexer::mapper index, indexer::mapper... follow> constexpr static
			inline_always typename std::enable_if<(sizeof...(follow) == 0), numeric>::type invoke(const board& iso, numeric updv, clip<feature> f) {
				return (f[(sizeof...(indexes) - sizeof...(follow) - 1) << 3][index(iso)] += updv);
			}

			constexpr static estimator estimate = isomorphic::estimate<invoke<indexes...>>;
			constexpr static optimizer optimize = isomorphic::optimize<invoke<indexes...>>;
		};
	};

	template<typename source = method>
	struct expectimax {
		constexpr inline operator method() { return { expectimax<source>::estimate, expectimax<source>::optimize }; }
		constexpr inline expectimax(utils::options::option opt) {
			u32 n = expectimax<source>::depth(opt.value(1));
			std::string limit = opt["limit"].value("");
			while (limit.find(',') != std::string::npos)
				limit[limit.find(',')] = ' ';
			std::stringstream in(limit);
			for (u32& lim : expectimax<source>::limit())
				in >> n, lim = n & -2u;
		}

		static inline numeric search_expt(const board& after, u32 depth, clip<feature> range = feature::feats()) {
			numeric expt = 0;
			hexa spaces;
			if (depth) depth = std::min(depth, limit((spaces = after.spaces()).size()));
			cache::block::access lookup = cache::find(after, depth);
			if (lookup) return lookup.fetch();
			if (!depth) return source::estimate(after, range);
			for (u32 pos : spaces) {
				board before = after;
				expt += 0.9 * search_best(({ before.set(pos, 1); before; }), depth - 1, range);
				expt += 0.1 * search_best(({ before.set(pos, 2); before; }), depth - 1, range);
			}
			expt = lookup.store(expt / spaces.size());
			return expt;
		}

		static inline numeric search_best(const board& before, u32 depth, clip<feature> range = feature::feats()) {
			numeric best = 0;
			for (const board& after : before.afters()) {
				auto search = after.info() != -1u ? search_expt : search_illegal;
				best = std::max(best, after.info() + search(after, depth - 1, range));
			}
			return best;
		}

		static inline numeric search_illegal(const board& after, u32 depth, clip<feature> range = feature::feats()) {
			return -std::numeric_limits<numeric>::max();
		}

		static inline numeric estimate(const board& after, clip<feature> range = feature::feats()) {
			return search_expt(after, depth() - 1, range);
		}
		static inline numeric optimize(const board& state, numeric updv, clip<feature> range = feature::feats()) {
			return source::optimize(state, updv, range);
		}

		static inline u32& depth() { static u32 depth = 1; return depth; }
		static inline u32& depth(u32 n) { return (expectimax<source>::depth() = n); }
		static inline std::array<u32, 17>& limit() { static std::array<u32, 17> limit = {}; return limit; }
		static inline u32& limit(u32 e) { return limit()[e]; }
	};

	typedef isomorphic::static_index<
			index::indexpt<0x0,0x1,0x2,0x3,0x4,0x5>,
			index::indexpt<0x4,0x5,0x6,0x7,0x8,0x9>,
			index::indexpt<0x0,0x1,0x2,0x4,0x5,0x6>,
			index::indexpt<0x4,0x5,0x6,0x8,0x9,0xa>> iso4x6patt;
	typedef isomorphic::static_index<
			index::indexpt<0x0,0x1,0x2,0x3,0x4,0x5>,
			index::indexpt<0x4,0x5,0x6,0x7,0x8,0x9>,
			index::indexpt<0x8,0x9,0xa,0xb,0xc,0xd>,
			index::indexpt<0x0,0x1,0x2,0x4,0x5,0x6>,
			index::indexpt<0x4,0x5,0x6,0x8,0x9,0xa>> iso5x6patt;
	typedef isomorphic::static_index<
			index::indexpt<0x0,0x1,0x2,0x4,0x5,0x6>,
			index::indexpt<0x4,0x5,0x6,0x7,0x8,0x9>,
			index::indexpt<0x0,0x1,0x2,0x3,0x4,0x5>,
			index::indexpt<0x2,0x3,0x4,0x5,0x6,0x9>,
			index::indexpt<0x0,0x1,0x2,0x5,0x9,0xa>,
			index::indexpt<0x3,0x4,0x5,0x6,0x7,0x8>> iso6x6patt;
	typedef isomorphic::static_index<
			index::indexpt<0x0,0x1,0x2,0x4,0x5,0x6>,
			index::indexpt<0x4,0x5,0x6,0x7,0x8,0x9>,
			index::indexpt<0x0,0x1,0x2,0x3,0x4,0x5>,
			index::indexpt<0x2,0x3,0x4,0x5,0x6,0x9>,
			index::indexpt<0x0,0x1,0x2,0x5,0x9,0xa>,
			index::indexpt<0x3,0x4,0x5,0x6,0x7,0x8>,
			index::indexpt<0x1,0x3,0x4,0x5,0x6,0x7>> iso7x6patt;
	typedef isomorphic::static_index<
			index::indexpt<0x0,0x1,0x2,0x4,0x5,0x6>,
			index::indexpt<0x4,0x5,0x6,0x7,0x8,0x9>,
			index::indexpt<0x0,0x1,0x2,0x3,0x4,0x5>,
			index::indexpt<0x2,0x3,0x4,0x5,0x6,0x9>,
			index::indexpt<0x0,0x1,0x2,0x5,0x9,0xa>,
			index::indexpt<0x3,0x4,0x5,0x6,0x7,0x8>,
			index::indexpt<0x1,0x3,0x4,0x5,0x6,0x7>,
			index::indexpt<0x0,0x1,0x4,0x8,0x9,0xa>> iso8x6patt;
	typedef isomorphic::static_index<
			index::indexpt<0x0,0x1,0x2,0x3,0x4,0x5,0x6>,
			index::indexpt<0x4,0x5,0x6,0x7,0x8,0x9,0xa>> iso2x7patt;
	typedef isomorphic::static_index<
			index::indexpt<0x0,0x1,0x2,0x3,0x4,0x5,0x6>,
			index::indexpt<0x4,0x5,0x6,0x7,0x8,0x9,0xa>,
			index::indexpt<0x8,0x9,0xa,0xb,0xc,0xd,0xe>> iso3x7patt;
	typedef isomorphic::static_index<
			index::indexpt<0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7>> iso1x8patt;
	typedef isomorphic::static_index<
			index::indexpt<0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7>,
			index::indexpt<0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb>> iso2x8patt;

	static method parse(utils::options opts, std::string type) {
		std::string spec = opts["options"]["spec"].value("auto");
		if (spec == "auto" || spec == "on" || spec == "default") {
			spec = opts["make"].value("4x6patt");
			spec = spec.substr(0, spec.find_first_of("&|="));
		}

		if (opts["search"].value(1) > 1) {
			switch (to_hash(spec)) {
			default:                    return method::expectimax<method>(opts["search"]);
			case to_hash("isomorphic"): return method::expectimax<method::isomorphic>(opts["search"]);
			case to_hash("4x6patt"):    return method::expectimax<method::iso4x6patt>(opts["search"]);
			case to_hash("5x6patt"):    return method::expectimax<method::iso5x6patt>(opts["search"]);
			case to_hash("6x6patt"):    return method::expectimax<method::iso6x6patt>(opts["search"]);
			case to_hash("7x6patt"):    return method::expectimax<method::iso7x6patt>(opts["search"]);
			case to_hash("8x6patt"):    return method::expectimax<method::iso8x6patt>(opts["search"]);
			case to_hash("2x7patt"):    return method::expectimax<method::iso2x7patt>(opts["search"]);
			case to_hash("3x7patt"):    return method::expectimax<method::iso3x7patt>(opts["search"]);
			case to_hash("1x8patt"):    return method::expectimax<method::iso1x8patt>(opts["search"]);
			case to_hash("2x8patt"):    return method::expectimax<method::iso2x8patt>(opts["search"]);
			}
		}

		switch (to_hash(spec)) {
		default:                    return method();
		case to_hash("isomorphic"): return method::isomorphic();
		case to_hash("4x6patt"):    return method::iso4x6patt();
		case to_hash("5x6patt"):    return method::iso5x6patt();
		case to_hash("6x6patt"):    return method::iso6x6patt();
		case to_hash("7x6patt"):    return method::iso7x6patt();
		case to_hash("8x6patt"):    return method::iso8x6patt();
		case to_hash("2x7patt"):    return method::iso2x7patt();
		case to_hash("3x7patt"):    return method::iso3x7patt();
		case to_hash("1x8patt"):    return method::iso1x8patt();
		case to_hash("2x8patt"):    return method::iso2x8patt();
		}
	}

	inline static numeric& alpha() { static numeric a = numeric(0.0025); return a; }
	inline static numeric& alpha(numeric a) { return (method::alpha() = a); }
	inline static numeric& lambda() { static numeric l = 0.5; return l; }
	inline static numeric& lambda(numeric l) { return (method::lambda() = l); }
	inline static u32& step() { static u32 n = 5; return n; }
	inline static u32& step(u32 n) { return (method::step() = n); }
};

struct state : board {
	numeric esti;
	inline state() : board(0ull, 0u, -1u), esti(0) {}
	inline state(const state& s) = default;

	inline operator bool() const { return info() != -1u; }
	declare_comparators(const state&, esti, inline);

	inline numeric value() const { return esti - info(); }
	inline i32 reward() const { return info(); }

	inline void assign(const board& b, u32 op = -1) {
		set(b);
		info(move(op));
	}
	inline numeric estimate(
			clip<feature> range = feature::feats(),
			method::estimator estim = method::estimate) {
		estim = info() != -1u ? estim : method::illegal;
		esti = state::reward() + estim(*this, range);
		return esti;
	}
	inline numeric optimize(numeric exact, numeric alpha = method::alpha(),
			clip<feature> range = feature::feats(),
			method::optimizer optim = method::optimize) {
		numeric update = (exact - state::value()) * alpha;
		esti = state::reward() + optim(*this, update, range);
		return esti;
	}
};
struct select {
	state move[4];
	state *best;
	inline select() : best(move) {}
	inline select& operator ()(const board& b, clip<feature> range = feature::feats(), method::estimator estim = method::estimate) {
		b.moves(move[0], move[1], move[2], move[3]);
		move[0].estimate(range, estim);
		move[1].estimate(range, estim);
		move[2].estimate(range, estim);
		move[3].estimate(range, estim);
		best = std::max_element(move, move + 4);
		return *this;
	}
	inline select& operator <<(const board& b) { return operator ()(b); }
	inline void operator >>(std::vector<state>& path) const { path.push_back(*best); }
	inline void operator >>(state& s) const { s = (*best); }
	inline void operator >>(board& b) const { b.set(*best); }

	inline operator bool() const { return score() != -1; }
	inline i32 score() const { return best->info(); }
	inline numeric esti() const { return best->esti; }
	inline u32 opcode() const { return best - move; }

	inline state* begin() { return move; }
	inline state* end() { return move + 4; }
};
struct statistic {
	u64 limit;
	u64 loop;
	u64 unit;
	u32 winv;

	struct record {
		u64 score;
		u64 win;
		u64 time;
		u64 opers;
		u32 max;
		u32 hash;
		record& operator +=(const record& rec) {
			score += rec.score;
			win += rec.win;
			time += rec.time;
			opers += rec.opers;
			hash |= rec.hash;
			max = std::max(max, rec.max);
			return (*this);
		}
	} total, local;

	struct each {
		std::array<u64, 32> score;
		std::array<u64, 32> opers;
		std::array<u64, 32> count;
		each& operator +=(const each& ea) {
			std::transform(count.begin(), count.end(), ea.count.begin(), count.begin(), std::plus<u64>());
			std::transform(score.begin(), score.end(), ea.score.begin(), score.begin(), std::plus<u64>());
			std::transform(opers.begin(), opers.end(), ea.opers.begin(), opers.begin(), std::plus<u64>());
			return (*this);
		}
	} every;

	struct execinfo {
		u32 thdid;
		u32 thdnum;
	} info;

	statistic() : limit(0), loop(0), unit(0), winv(0), total{}, local{}, every{}, info{0, 1} {}
	statistic(const utils::options::option& opt) : statistic() { init(opt); }
	statistic(const statistic&) = default;

	bool init(const utils::options::option& opt = {}) {
		loop = 1000;
		unit = 1000;
		winv = 2048;

		auto npos = std::string::npos;
		auto it = std::find_if(opt.begin(), opt.end(), [=](std::string v) { return v.find('=') == npos; });
		std::string res = (it != opt.end()) ? *it : (opt.empty() ? "1000" : "0");
		try {
			loop = std::stol(res);
			if (res.find('x') != npos) unit = std::stol(res.substr(res.find('x') + 1));
			if (res.find(':') != npos) winv = std::stol(res.substr(res.find(':') + 1));
		} catch (std::invalid_argument&) {}

		loop = std::stol(opt.find("loop", std::to_string(loop)));
		unit = std::stol(opt.find("unit", std::to_string(unit)));
		winv = std::stol(opt.find("win",  std::to_string(winv)));

		info.thdid = std::stol(opt.find("thread#", "0"));
		info.thdnum = std::stol(opt.find("thread", "1"));
		loop = loop / info.thdnum + (loop % info.thdnum && info.thdid < (loop % info.thdnum) ? 1 : 0);
		limit = loop * unit;
		format(0, (info.thdnum > 1) ? (" [" + std::to_string(info.thdid) + "]") : "");

		every = {};
		total = {};
		local = {};
		for (u32 i = 0; i < info.thdid; i++) moporgic::rand();
		local.time = moporgic::millisec();
		loop = 1;

		return limit;
	}

	class format_t : public std::array<char, 64> {
	public:
		inline void operator =(const std::string& s) { std::copy_n(s.begin(), s.size() + 1, begin()); }
		inline operator const char*() const { return data(); }
	};

	format_t indexf;
	format_t localf;
	format_t totalf;
	format_t summaf;

	void format(u32 dec = 0, const std::string& suffix = "") {
//		indexf = "%03llu/%03llu %llums %.2fops";
//		localf = "local:  avg=%llu max=%u tile=%u win=%.2f%%";
//		totalf = "total:  avg=%llu max=%u tile=%u win=%.2f%%";
//		summaf = "summary %llums %.2fops";
		if (!dec) dec = std::max(std::floor(std::log10(limit / unit)) + 1, 3.0);
		indexf = "%0" + std::to_string(dec) + PRIu64 "/%0" + std::to_string(dec) + PRIu64 " %" PRIu64 "ms %.2fops" + suffix;
		localf = "local: " + std::string(dec * 2 - 5, ' ') + "avg=%" PRIu64 " max=%u tile=%u win=%.2f%%";
		totalf = "total: " + std::string(dec * 2 - 5, ' ') + "avg=%" PRIu64 " max=%u tile=%u win=%.2f%%";
		summaf = "summary" + std::string(dec * 2 - 5, ' ') + "%" PRIu64 "ms %.2fops" + suffix;
	}

	inline u64 operator++(int) { return (++loop) - 1; }
	inline u64 operator++() { return (++loop); }
	inline operator bool() const { return loop <= limit; }
	inline bool checked() const { return (loop % unit) == 0; }

	void update(u32 score, u32 hash, u32 opers) {
		local.score += score;
		local.hash |= hash;
		local.opers += opers;
		local.win += (hash >= winv ? 1 : 0);
		local.max = std::max(local.max, score);
		every.count[math::log2(hash)] += 1;
		every.score[math::log2(hash)] += score;
		every.opers[math::log2(hash)] += opers;

		if ((loop % unit) != 0) return;

		u64 tick = moporgic::millisec();
		local.time = tick - local.time;
		total.score += local.score;
		total.win += local.win;
		total.time += local.time;
		total.opers += local.opers;
		total.hash |= local.hash;
		total.max = std::max(total.max, local.max);

		char buf[256];
		u32 size = 0;

		size += snprintf(buf + size, sizeof(buf) - size, indexf, // "%03llu/%03llu %llums %.2fops",
				loop / unit,
				limit / unit,
				local.time,
				local.opers * 1000.0 / local.time);
		buf[size++] = '\n';
		size += snprintf(buf + size, sizeof(buf) - size, localf, // "local:  avg=%llu max=%u tile=%u win=%.2f%%",
				local.score / unit,
				local.max,
				math::msb32(local.hash),
				local.win * 100.0 / unit);
		buf[size++] = '\n';
		size += snprintf(buf + size, sizeof(buf) - size, totalf, // "total:  avg=%llu max=%u tile=%u win=%.2f%%",
				total.score / loop,
				total.max,
				math::msb32(total.hash),
				total.win * 100.0 / loop);
		buf[size++] = '\n';
		buf[size++] = '\n';
		buf[size++] = '\0';

		std::cout << buf << std::flush;

		local = {};
		local.time = tick;
	}

	void summary() const {
		if (limit == 0) return;
		char buf[1024];
		size_t size = 0;

		size += snprintf(buf + size, sizeof(buf) - size, summaf, // "summary %llums %.2fops",
				total.time / info.thdnum,
				total.opers * 1000.0 * info.thdnum / total.time);
		buf[size++] = '\n';
		size += snprintf(buf + size, sizeof(buf) - size, totalf, // "total:  avg=%llu max=%u tile=%u win=%.2f%%",
				total.score / limit,
				total.max,
				math::msb32(total.hash),
				total.win * 100.0 / limit);
		buf[size++] = '\n';
		size += snprintf(buf + size, sizeof(buf) - size,
		        "%-6s"  "%8s"    "%8s"    "%8s"   "%9s"   "%9s",
		        "tile", "count", "score", "move", "rate", "win");
		buf[size++] = '\n';
		const auto& count = every.count;
		const auto& score = every.score;
		const auto& opers = every.opers;
		auto total = std::accumulate(count.begin(), count.end(), 0);
		for (auto left = total, i = 0; left; left -= count[i++]) {
			if (count[i] == 0) continue;
			size += snprintf(buf + size, sizeof(buf) - size,
					"%-6d" "%8d" "%8d" "%8d" "%8.2f%%" "%8.2f%%",
					board::tile::itov(i), u32(count[i]),
					u32(score[i] / count[i]), u32(opers[i] / count[i]),
					count[i] * 100.0 / total, left * 100.0 / total);
			buf[size++] = '\n';
		}
		buf[size++] = '\n';
		buf[size++] = '\0';

		std::cout << buf << std::flush;
	}

	statistic  operator + (const statistic& stat) const {
		return statistic(*this) += stat;
	}
	statistic& operator +=(const statistic& stat) {
		limit += stat.limit;
		loop += stat.loop;
		if (!unit) unit = stat.unit;
		if (!winv) winv = stat.winv;
		total += stat.total;
		local += stat.local;
		every += stat.every;
		u32 dec = (std::string(summaf).find('%') - std::string(summaf).find('y') + 5) / 2;
		format(dec, (info.thdnum > 1) ? (" (" + std::to_string(info.thdnum) + "x)") : "");
		return *this;
	}
};

statistic run(utils::options opts, std::string type) {
	std::vector<state> path;
	statistic stats;
	select best;
	state last;

	method spec = method::parse(opts, type);
	clip<feature> feats = feature::feats();
	numeric alpha = method::alpha(opts["alpha"].value(0.1) / (opts["alpha"]("norm") ? opts["alpha"]["norm"].value(feats.size()) : 1));
	numeric lambda = method::lambda(opts["lambda"].value(0));
	u32 step = method::step(opts["step"].value(lambda ? 5 : 1));

	switch (to_hash(opts[type]["mode"].value(type))) {
	case to_hash("optimize"):
	case to_hash("optimize:forward"): [&]() {
		for (stats.init(opts[type]); stats; stats++) {
			board b;
			u32 score = 0;
			u32 opers = 0;

			b.init();
			best(b, feats, spec);
			score += best.score();
			opers += 1;
			best >> last;
			best >> b;
			b.next();
			while (best(b, feats, spec)) {
				last.optimize(best.esti(), alpha, feats, spec);
				score += best.score();
				opers += 1;
				best >> last;
				best >> b;
				b.next();
			}
			last.optimize(0, alpha, feats, spec);

			stats.update(score, b.hash(), opers);
		}
		}(); break;

	case to_hash("optimize:backward"): [&]() {
		for (stats.init(opts[type]); stats; stats++) {
			board b;
			u32 score = 0;
			u32 opers = 0;

			for (b.init(); best(b, feats, spec); b.next()) {
				score += best.score();
				opers += 1;
				best >> path;
				best >> b;
			}

			for (numeric esti = 0; path.size(); path.pop_back()) {
				path.back().estimate(feats, spec);
				esti = path.back().optimize(esti, alpha, feats, spec);
			}

			stats.update(score, b.hash(), opers);
		}
		}(); break;

	case to_hash("optimize:step"):
	case to_hash("optimize:step-forward"): [&]() {
		for (stats.init(opts[type]); stats; stats++) {
			board b;
			u32 score = 0;
			u32 opers = 0;
			u32 rsum = 0;

			b.init();
			for (u32 i = 0; i < step && best(b, feats, spec); i++) {
				rsum += best.score();
				score += best.score();
				opers += 1;
				best >> path;
				best >> b;
				b.next();
			}
			while (best(b, feats, spec)) {
				state& update = path[opers - step];
				rsum -= update.info();
				update.estimate(feats, spec);
				update.optimize(rsum + best.esti(), alpha, feats, spec);
				rsum += best.score();
				score += best.score();
				opers += 1;
				best >> path;
				best >> b;
				b.next();
			}
			for (u32 i = opers - std::min(step, opers); i < opers; i++) {
				state& update = path[i];
				rsum -= update.info();
				update.estimate(feats, spec);
				update.optimize(rsum, alpha, feats, spec);
			}
			path.clear();

			stats.update(score, b.hash(), opers);
		}
		}(); break;

	case to_hash("optimize:step-backward"): [&]() {
		for (stats.init(opts[type]); stats; stats++) {
			board b;
			u32 score = 0;
			u32 opers = 0;

			for (b.init(); best(b, feats, spec); b.next()) {
				score += best.score();
				opers += 1;
				best >> path;
				best >> b;
			}

			u32 rsum = 0;
			for (u32 i = opers - 1; i >= opers - std::min(step, opers); i--) {
				state& update = path[i];
				update.estimate(feats, spec);
				update.optimize(rsum, alpha, feats, spec);
				rsum += update.info();
			}
			for (u32 i = opers - 1; i >= step; i--) {
				state& source = path[i];
				state& update = path[i - step];
				rsum -= source.info();
				numeric esti = source.estimate(feats, spec);
				update.estimate(feats, spec);
				update.optimize(rsum + esti, alpha, feats, spec);
				rsum += update.info();
			}
			path.clear();

			stats.update(score, b.hash(), opers);
		}
		}(); break;

	case to_hash("optimize:lambda-forward"): [&]() {
		for (stats.init(opts[type]); stats; stats++) {
			board b;
			u32 score = 0;
			u32 opers = 0;

			b.init();
			for (u32 i = 0; i < step && best(b, feats, spec); i++) {
				score += best.score();
				opers += 1;
				best >> path;
				best >> b;
				b.next();
			}
			while (best(b, feats, spec)) {
				numeric z = best.esti();
				numeric retain = 1 - lambda;
				for (u32 k = 1; k < step; k++) {
					state& source = path[opers - k];
					source.estimate(feats, spec);
					numeric r = source.reward();
					numeric v = source.value();
					z = r + (lambda * z + retain * v);
				}
				state& update = path[opers - step];
				update.estimate(feats, spec);
				update.optimize(z, alpha, feats, spec);
				score += best.score();
				opers += 1;
				best >> path;
				best >> b;
				b.next();
			}
			for (u32 tail = std::min(step, opers), i = 0; i < tail; i++) {
				numeric z = 0;
				numeric retain = 1 - lambda;
				for (u32 k = i + 1; k < tail; k++) {
					state& source = path[opers + i - k];
					source.estimate(feats, spec);
					numeric r = source.reward();
					numeric v = source.value();
					z = r + (lambda * z + retain * v);
				}
				state& update = path[opers + i - tail];
				update.estimate(feats, spec);
				update.optimize(z, alpha, feats, spec);
			}
			path.clear();

			stats.update(score, b.hash(), opers);
		}
		}(); break;

	case to_hash("optimize:lambda"):
	case to_hash("optimize:lambda-backward"): [&]() {
		for (stats.init(opts[type]); stats; stats++) {
			board b;
			u32 score = 0;
			u32 opers = 0;

			for (b.init(); best(b, feats, spec); b.next()) {
				score += best.score();
				opers += 1;
				best >> path;
				best >> b;
			}

			numeric z = 0;
			numeric r = path.back().reward();
			numeric v = path.back().optimize(0, alpha, feats, spec) - r;
			numeric retain = 1 - lambda;
			for (path.pop_back(); path.size(); path.pop_back()) {
				path.back().estimate(feats, spec);
				z = r + (lambda * z + retain * v);
				r = path.back().reward();
				v = path.back().optimize(z, alpha, feats, spec) - r;
			}

			stats.update(score, b.hash(), opers);
		}
		}(); break;

	case to_hash("evaluate"):
	case to_hash("evaluate:best"): [&]() {
		for (stats.init(opts[type]); stats; stats++) {
			board b;
			u32 score = 0;
			u32 opers = 0;

			for (b.init(); best(b, feats, spec); b.next()) {
				score += best.score();
				opers += 1;
				best >> b;
			}

			stats.update(score, b.hash(), opers);
		}
		}(); break;

	case to_hash("evaluate:random"): [&]() {
		for (stats.init(opts[type]); stats; stats++) {
			board b;
			u32 score = 0;
			u32 opers = 0;
			u64 rbuf;
			hex a;

			for (b.init(); (a = b.actions()).size(); b.next()) {
				if (opers % 8 == 0) rbuf = moporgic::rand64();
				score += b.operate(a[raw_cast<u8>(rbuf, opers % 8) % a.size()]);
				opers += 1;
			}

			stats.update(score, b.hash(), opers);
		}
		}(); break;

	case to_hash("evaluate:reward"): [&]() {
		for (stats.init(opts[type]); stats; stats++) {
			board b;
			struct state : board {
				declare_comparators_with(const state&, int(info()), int(v.info()), inline constexpr)
			} moves[4];
			u32 score = 0;
			u32 opers = 0;

			for (b.init(); ({ b.moves(moves); (b = *std::max_element(moves, moves + 4)).info() != -1u; }); b.next()) {
				score += b.info();
				opers += 1;
			}

			stats.update(score, b.hash(), opers);
		}
		}(); break;
	}

	return stats;
}

utils::options parse(int argc, const char* argv[]) {
	utils::options opts;
	for (int i = 1; i < argc; i++) {
		std::string label = argv[i];
		auto next_opt = [&](std::string v = "") -> std::string {
			return (i + 1 < argc && *(argv[i + 1]) != '-') ? argv[++i] : v;
		};
		auto next_opts = [&](std::string v = "") -> utils::options::list {
			utils::options::list args;
			if (v.size()) args.push_back(next_opt(v));
			while ((v = next_opt()).size()) args.push_back(v);
			return args;
		};
		switch (to_hash(label)) {
		case to_hash("-a"): case to_hash("--alpha"):
			opts["alpha"] = next_opts();
			if (opts["alpha"].empty()) (opts["alpha"] = "0.1") += "norm";
			break;
		case to_hash("-l"): case to_hash("--lambda"):
			opts["lambda"] = next_opt("0.5");
			// no break: lambda may also come with step
		case to_hash("--step"): case to_hash("--n-step"):
			opts["step"] = next_opt(opts["lambda"].value(0) ? "5" : "1");
			break;
		case to_hash("-s"): case to_hash("--seed"):
			opts["seed"] = next_opt("moporgic");
			break;
		case to_hash("-r"): case to_hash("--recipe"):
			label = next_opt("optimize");
			// no break: optimize and evaluate are also handled by the same recipe logic
		case to_hash("-t"): case to_hash("--optimize"):
		case to_hash("-e"): case to_hash("--evaluate"):
			label = label[label.find_first_not_of('-')] != 'e' ? "optimize" : "evaluate";
			if ((opts[""] = next_opts()).size()) opts[label] = opts[""];
			if (opts[label].size()) opts["recipes"] += label;
			break;
		case to_hash("--recipes"):
			opts["recipes"] = next_opts();
			break;
		case to_hash("-io"):  case to_hash("--input-output"):
		case to_hash("-nio"): case to_hash("--network-input-output"):
		case to_hash("-wio"): case to_hash("--weight-input-output"):
		case to_hash("-i"):   case to_hash("--input"):
		case to_hash("-wi"):  case to_hash("--weight-input"):
		case to_hash("-ni"):  case to_hash("--network-input"):
		case to_hash("-o"):   case to_hash("--output"):
		case to_hash("-wo"):  case to_hash("--weight-output"):
		case to_hash("-no"):  case to_hash("--network-output"):
			opts[""] = next_opts(opts.find("make", argv[0]) + '.' + label[label.find_first_not_of('-')]);
			if (label.find(label[1] != '-' ? "i" : "input")  != std::string::npos) opts["load"] += opts[""];
			if (label.find(label[1] != '-' ? "o" : "output") != std::string::npos) opts["save"] += opts[""];
			break;
		case to_hash("-w"): case to_hash("--weight"):
		case to_hash("-f"): case to_hash("--feature"):
		case to_hash("-n"): case to_hash("--network"):
			opts["make"] += next_opts("default");
			break;
		case to_hash("-tt"): case to_hash("--optimize-type"):
		case to_hash("-tm"): case to_hash("--optimize-mode"):
		case to_hash("-et"): case to_hash("--evaluate-type"):
		case to_hash("-em"): case to_hash("--evaluate-mode"):
			label = label[label.find_first_not_of('-')] != 'e' ? "optimize" : "evaluate";
			opts[label]["mode"] = next_opt(label);
			if (!opts["recipes"](label)) opts["recipes"] += label;
			break;
		case to_hash("-m"):  case to_hash("--mode"):
			if (opts["recipes"].empty()) opts["recipes"] += "optimize";
			opts[opts["recipes"].back()]["mode"] = next_opt(opts["recipes"].back());
			break;
		case to_hash("-u"): case to_hash("--unit"):
			opts["unit"] = next_opt("1000");
			break;
		case to_hash("-v"): case to_hash("--win"):
			opts["win"] = next_opt("2048");
			break;
		case to_hash("-%"): case to_hash("-I"): case to_hash("--info"):
			opts["info"] = next_opt();
			break;
		case to_hash("-d"): case to_hash("--depth"):
		case to_hash("-S"): case to_hash("--search"):
			opts["search"] = next_opts("3");
			break;
		case to_hash("-c"): case to_hash("--cache"):
			opts["cache"] = next_opts("2048M");
			break;
		case to_hash("-p"): case to_hash("--parallel"): case to_hash("--thread"):
			opts["thread"] = std::thread::hardware_concurrency();
			if ((opts[""] = next_opts()).value(0)) opts["thread"].clear();
			opts["thread"] += opts[""];
			break;
		case to_hash("-x"): case to_hash("--options"):
			opts["options"] += next_opts();
			break;
		case to_hash("-#"): case to_hash("--comment"):
			opts["comment"] += next_opts();
			break;
		case to_hash("-|"):
			opts = {};
			break;
		default:
			opts["options"][label.substr(label.find_first_not_of('-'))] += next_opts();
			break;
		}
	}
	for (std::string recipe : opts["recipes"]) {
		utils::options::option& ropt = opts[recipe];
		for (auto item : {"unit", "win", "info"})
			if (opts(item) && !ropt(item))
				ropt[item] = opts[item];

		if (recipe.find("optimize") == 0 && !ropt("mode")) {
			if (ropt("lambda") || opts("lambda"))  ropt["mode"] = "lambda";
			else if (ropt("step") || opts("step")) ropt["mode"] = "step";
		}
		if (recipe.find("optimize") == 0 || recipe.find("evaluate") == 0)
			if (ropt.find("mode", recipe).find(recipe) != 0)
				ropt["mode"] = recipe + ":" + ropt["mode"];
		ropt.remove("mode");

		if (!ropt("info") && ropt.find("mode", recipe).find("evaluate") == 0)
			ropt["info"];
		ropt.remove("info=none");
	}
	return opts;
}

int main(int argc, const char* argv[]) {
	utils::options opts = parse(argc, argv);
	if (!opts("recipes")) opts["recipes"] = "optimize", opts["optimize"] = 1000;
	if (!opts("alpha")) opts["alpha"] = 0.1, opts["alpha"] += "norm";
	if (!opts("seed")) opts["seed"] = ({std::stringstream ss; ss << std::hex << rdtsc(); ss.str();});
	if (!opts("lambda")) opts["lambda"] = 0;
	if (!opts("step")) opts["step"] = opts["lambda"].value(0) ? 5 : 1;

	utils::init_logging(opts["save"]);
	std::cout << "TDL2048+ by Hung Guei" << std::endl;
	std::cout << "Develop" << " Build GCC " __VERSION__ << " C++" << __cplusplus;
	std::cout << " (" << __DATE_ISO__ << " " << __TIME__ ")" << std::endl;
	std::copy(argv, argv + argc, std::ostream_iterator<const char*>(std::cout, " "));
	std::cout << std::endl;
	std::cout << "time = " << millisec() << " (" << moporgic::put_time(millisec()) << ")" << std::endl;
	std::cout << "seed = " << opts["seed"] << std::endl;
	std::cout << "alpha = " << opts["alpha"] << std::endl;
	std::cout << "lambda = " << opts["lambda"] << ", step = " << opts["step"] << std::endl;
	std::cout << "search = " << opts["search"].value("1") << ", cache = " << opts["cache"].value("none") << std::endl;
	std::cout << "thread = " << opts["thread"].value(1) << "x" << std::endl;
	std::cout << std::endl;

	moporgic::srand(to_hash(opts["seed"]));
	utils::config_thread(opts["thread"]);
	utils::init_cache(opts["cache"]);

	utils::load_network(opts["load"]);
	utils::make_network(opts["make"]);
	utils::list_network();

	for (std::string recipe : opts["recipes"]) {
		std::cout << recipe << ": " << opts[recipe] << std::endl << std::endl;
		statistic stat = utils::invoke(run, opts, recipe);
		if (opts[recipe]("info")) stat.summary();
	}

	utils::save_network(opts["save"]);

	return 0;
}

} // namespace moporgic

int main(int argc, const char* argv[]) {
	return moporgic::main(argc, argv);
}
