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
#include "moporgic/io.h"
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
#include <string>
#include <thread>
#include <limits>
#include <cctype>
#include <iterator>
#include <sstream>
#include <list>

namespace moporgic {

typedef double numeric;
numeric alpha = 0.0025;

class weight {
public:
	weight() : sign(0), value(nullptr), size(0) {}
	weight(const weight& w) : sign(w.sign), value(w.value), size(w.size) {}
	~weight() {}

	inline u32 signature() const { return sign; }
	inline numeric& operator [](const u64& i) { return value.get()[i]; }
	inline bool operator ==(const u32& s) const { return sign == s; }
	inline size_t length() const { return size; }

	void operator >>(std::ostream& out) const {
		const int LE = moporgic::endian::le;
		const char serial = 1;
		out.write(&serial, 1);
		switch (serial) {
		case 0:
			out.write(r32(sign).endian(LE), 4);
			out.write(r64(size).endian(LE), 8);
			for (u64 i = 0; i < size; i++)
				out.write(r32(value.get()[i]).endian(LE), 4);
			break;
		case 1:
			out.write(r32(sign).le(), 4);
			out.write(r64(size).le(), 8);
			switch (sizeof(numeric)) {
			case 4: write<r32>(out);
				break;
			case 8: write<r64>(out);
				break;
			}
			break;
		default:
			std::cerr << "unknown serial at weight::>>" << std::endl;
			break;
		}
	}
	void operator <<(std::istream& in) {
		char buf[8];
		auto load = moporgic::make_load(in, buf);
		const int LE = moporgic::endian::le;
		switch (*load(1)) {
		case 0:
			sign = r32(load(4), LE);
			size = r64(load(8), LE);
			value = std::shared_ptr<numeric>(new numeric[size]());
			for (u64 i = 0; i < size; i++)
				value.get()[i] = r32(load(4), LE);
			break;
		case 1:
			sign = r32(load(4)).le();
			size = r64(load(8)).le();
			value = std::shared_ptr<numeric>(new numeric[size]());
			switch (sizeof(numeric)) {
			case 4: read<r32>(in);
				break;
			case 8: read<r64>(in);
				break;
			}
			break;
		default:
			std::cerr << "unknown serial at weight::<<" << std::endl;
			break;
		}
	}

	static void save(std::ostream& out) {
		const int LE = moporgic::endian::le;
		const char serial = 0;
		out.write(&serial, 1);
		switch (serial) {
		case 0:
			out.write(r32(u32(weights().size())).endian(LE), 4);
			for (weight w : weights())
				w >> out;
			break;
		default:
			std::cerr << "unknown serial at weight::save" << std::endl;
			break;
		}
		out.flush();
	}

	static void load(std::istream& in) {
		const int LE = moporgic::endian::le;
		char buf[8];
		char serial;
		in.read(&serial, 1);
		switch (serial) {
		case 0:
			in.read(buf, 4);
			for (u32 size = r32(buf, LE); size; size--) {
				weights().push_back(weight());
				weights().back() << in;
			}
			break;
		default:
			std::cerr << "unknown serial at weight::load" << std::endl;
			break;
		}
	}

	static bool save(const std::string& path) {
		std::ofstream out;
		char buf[1 << 20];
		out.rdbuf()->pubsetbuf(buf, sizeof(buf));
		out.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) return false;
		weight::save(out);
		out.flush();
		out.close();
		return true;
	}
	static bool load(const std::string& path) {
		std::ifstream in;
		char buf[1 << 20];
		in.rdbuf()->pubsetbuf(buf, sizeof(buf));
		in.open(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) return false;
		weight::load(in);
		in.close();
		return true;
	}

	static weight make(const u32& sign, const u64& size) {
		weights().push_back(weight(sign, size));
		return weights().back();
	}
	static inline weight at(const u32& sign) {
		for (weight w : weights())
			if (w.signature() == sign) return w;
		return weights().at(-1);
	}
	typedef std::vector<weight>::iterator iter;
	static inline iter begin() { return weights().begin(); }
	static inline iter end() { return weights().end(); }
	static inline iter find(const u32& sign) {
		return std::find(begin(), end(), sign);
	}
private:
	weight(const u32& sign, const u64& size) : sign(sign), value(new numeric[size]()), size(size) {}
	static inline std::vector<weight>& weights() { static std::vector<weight> w; return w; }

	template<typename rxx> void write(std::ostream& out) const {
		numeric *v = value.get();
		for (u64 i = 0; i < size; i++)
			out.write(rxx(v[i]).le(), sizeof(rxx));
	}
	template<typename rxx> void read(std::istream& in) {
		char buf[8];
		auto load = moporgic::make_load(in, buf);
		numeric *v = value.get();
		for (u64 i = 0; i < size; i++)
			v[i] = rxx(load(sizeof(rxx))).le();
	}

	u32 sign;
	std::shared_ptr<numeric> value;
	u64 size;
};

class indexer {
public:
	indexer() : sign(0), map(nullptr) {}
	indexer(const indexer& i) : sign(i.sign), map(i.map) {}
	~indexer() {}

	inline u32 signature() const { return sign; }
	inline u64 operator ()(const board& b) const { return map(b); }
	inline bool operator ==(const u32& s) const { return sign == s; }

	typedef std::function<u64(const board&)> mapper;
	static indexer make(const u32& sign, mapper map) {
		indexers().push_back(indexer(sign, map));
		return indexers().back();
	}
	static inline indexer at(const u32& sign) {
		for (indexer i : indexers())
			if (i.signature() == sign) return i;
		return indexers().at(-1);
	}
	typedef std::vector<indexer>::iterator iter;
	static inline iter begin() { return indexers().begin(); }
	static inline iter end() { return indexers().end(); }
	static inline iter find(const u32& sign) {
		return std::find(begin(), end(), sign);
	}
private:
	indexer(const u32& sign, mapper map) : sign(sign), map(map) {}
	static inline std::vector<indexer>& indexers() { static std::vector<indexer> i; return i; }

	u32 sign;
	mapper map;
};

class feature {
public:
	feature() {}
	feature(const feature& t) : index(t.index), value(t.value) {}
	~feature() {}

	template<numeric* value, u64 (*index)(const board&)>
	static inline numeric& pass(const board& b) { return value[index(b)]; }
	template<weight& value, indexer& index>
	static inline numeric& pass(const board& b) { return value[index(b)]; }

	inline u64 signature() const {
		return (u64(value.signature()) << 32) | index.signature();
	}
	inline numeric& operator [](const board& b) { return value[index(b)]; }
	inline numeric& operator [](const u64& idx) { return value[idx]; }
	inline u64 operator ()(const board& b) const { return index(b); }
	inline bool operator ==(const u64& s) const { return signature() == s; }
	inline operator indexer() const { return index; }
	inline operator weight() const { return value; }

	void operator >>(std::ostream& out) const {
		const int LE = moporgic::endian::le;
		const char serial = 0;
		out.write(&serial, 1);
		switch (serial) {
		case 0:
			out.write(r32(index.signature()).endian(LE), 4);
			out.write(r32(value.signature()).endian(LE), 4);
			break;
		default:
			std::cerr << "unknown serial at feature::>>" << std::endl;
			break;
		}
	}
	void operator <<(std::istream& in) {
		char buf[8];
		auto load = moporgic::make_load(in, buf);
		const int LE = moporgic::endian::le;
		switch (*load(1)) {
		case 0:
			index = indexer::at(r32(load(4), LE));
			value = weight::at(r32(load(4), LE));
			break;
		default:
			std::cerr << "unknown serial at feature::<<" << std::endl;
			break;
		}
	}

	static void save(std::ostream& out) {
		const int LE = moporgic::endian::le;
		const char serial = 0;
		out.write(&serial, 1);
		switch (serial) {
		case 0:
			out.write(r32(u32(feats().size())).endian(LE), 4);
			for (u32 i = 0, size = feats().size(); i < size; i++)
				feats()[i] >> out;
			break;
		default:
			std::cerr << "unknown serial at feature::save" << std::endl;
			break;
		}
		out.flush();
	}
	static void load(std::istream& in) {
		const int LE = moporgic::endian::le;
		char buf[4];
		char serial;
		in.read(&serial, 1);
		switch (serial) {
		case 0:
			in.read(buf, 4);
			for (u32 size = r32(buf, LE); size; size--) {
				feats().push_back(feature());
				feats().back() << in;
			}
			break;
		default:
			std::cerr << "unknown serial at feature::load" << std::endl;
			break;
		}
	}

	static bool save(const std::string& path) {
		std::ofstream out;
		out.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) return false;
		feature::save(out);
		out.flush();
		out.close();
		return true;
	}
	static bool load(const std::string& path) {
		std::ifstream in;
		in.open(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) return false;
		feature::load(in);
		in.close();
		return true;
	}

	static feature make(const u32& wgt, const u32& idx) {
		feats().push_back(feature(weight::at(wgt), indexer::at(idx)));
		return feats().back();
	}
	static feature at(const u32& wgt, const u32& idx) {
		for (auto feat : feats()) {
			if (weight(feat).signature() == wgt && indexer(feat).signature() == idx)
				return feat;
		}
		return feats().at(-1);
	}
	typedef std::vector<feature>::iterator iter;
	static inline iter begin() { return feats().begin(); }
	static inline iter end() { return feats().end(); }
	static inline iter find(const u32& wgt, const u32& idx) {
		return std::find(begin(), end(), (u64(wgt) << 32) | idx);
	}

private:
	feature(const weight& value, const indexer& index) : index(index), value(value) {}
	static inline std::vector<feature>& feats() { static std::vector<feature> f; return f; }

	indexer index;
	weight value;
};

namespace utils {

struct options : public std::list<std::string> {
	options() : std::list<std::string>() {}
	options(const std::list<std::string>& opts) : std::list<std::string>(opts) {}

	options& operator +=(const std::string& opt) {
		push_back(opt);
		return *this;
	}
	options& operator -=(const std::string& opt) {
		auto it = std::find(begin(), end(), opt);
		if (it != end()) this->erase(it);
		return *this;
	}

	bool exists(const std::string& opt) const {
		return std::find(begin(), end(), opt) != end();
	}
	std::string& operator [](const std::string& opt) {
		auto it = std::find(begin(), end(), opt);
		if (it == end()) return (operator +=(opt))[opt];
		if (++it == end()) return (operator +=(""))[opt];
		return (*it);
	}

	operator std::string() const {
		std::stringstream ss;
		std::copy(begin(), end(), std::ostream_iterator<std::string>(ss, " "));
		return ss.str();
	}};

inline void rotfx(int& i) {
	i = (3 - (i >> 2)) + ((i % 4) << 2);
}
inline void mirfx(int& s) {
	s = ((s >> 2) << 2) + (3 - (s % 4));
}
std::vector<std::function<void(int&)>> mapfx = { rotfx, rotfx, rotfx, mirfx, rotfx, rotfx, rotfx, mirfx };
std::vector<std::vector<int>> patt6t =
	{ { 0, 1, 2, 3, 6, 7 }, { 4, 5, 6, 7, 10, 11 }, { 0, 1, 2, 4, 5, 6 }, { 4, 5, 6, 8, 9, 10 }, };
inline u32 hashfx(std::vector<int>& p) {
	u32 h = 0; for (int t : p) h = (h << 4) | t; return h;
}


const u32 base = 16;
template<int p0, int p1, int p2, int p3, int p4, int p5>
u64 index6t(const board& b) {
//	std::cout << p0 << "\t" << p1 << "\t" << p2 << "\t" << p3 << "\t" << p4 << "\t" << p5 << std::endl;
	register u64 index = 0;
	index += b.at(p0) <<  0;
	index += b.at(p1) <<  4;
	index += b.at(p2) <<  8;
	index += b.at(p3) << 12;
	index += b.at(p4) << 16;
	index += b.at(p5) << 20;
	return index;
}
u64 index6t(const board& b,
		const int& p0, const int& p1, const int& p2,
		const int& p3, const int& p4, const int& p5) {
//	std::cout << p0 << "\t" << p1 << "\t" << p2 << "\t" << p3 << "\t" << p4 << "\t" << p5 << std::endl;
	register u64 index = 0;
	index += b.at(p0) <<  0;
	index += b.at(p1) <<  4;
	index += b.at(p2) <<  8;
	index += b.at(p3) << 12;
	index += b.at(p4) << 16;
	index += b.at(p5) << 20;
	return index;
}
template<int p0, int p1, int p2, int p3>
u64 index4t(const board& b) {
	register u64 index = 0;
	index += b.at(p0) <<  0;
	index += b.at(p1) <<  4;
	index += b.at(p2) <<  8;
	index += b.at(p3) << 12;
	return index;
}
u64 index4t(const board& b, const int& p0, const int& p1, const int& p2, const int& p3) {
	register u64 index = 0;
	index += b.at(p0) <<  0;
	index += b.at(p1) <<  4;
	index += b.at(p2) <<  8;
	index += b.at(p3) << 12;
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

u64 indexnum0(const board& b) { // 10-bit
	// 2k ~ 32k, 2-bit ea.
	u16 num[16];
	b.count(num, 11, 16);
	register u64 index = 0;
	index += (num[11] & 0x03) << 0;
	index += (num[12] & 0x03) << 2;
	index += (num[13] & 0x03) << 4;
	index += (num[14] & 0x03) << 6;
	index += (num[15] & 0x03) << 8;
	return index;
}

u64 indexnum1(const board& b) { // 25-bit
	u16 num[16];
	b.count(num, 5, 16);
	register u64 index = 0;
	index += ((num[5] + num[6]) & 0x0f) << 0; // 32 & 64, 4-bit
	index += (num[7] & 0x07) << 4; // 128, 3-bit
	index += (num[8] & 0x07) << 7; // 256, 3-bit
	index += (num[9] & 0x07) << 10; // 512, 3-bit
	index += (num[10] & 0x03) << 13; // 1k ~ 32k, 2-bit ea.
	index += (num[11] & 0x03) << 15;
	index += (num[12] & 0x03) << 17;
	index += (num[13] & 0x03) << 19;
	index += (num[14] & 0x03) << 21;
	index += (num[15] & 0x03) << 23;
	return index;
}

u64 indexnum2(const board& b) { // 25-bit
	u16 num[16];
	b.count(num);
	register u64 index = 0;
	index += ((num[1] + num[2]) & 0x07) << 0; // 2 & 4, 3-bit
	index += ((num[3] + num[4]) & 0x07) << 3; // 8 & 16, 3-bit
	index += ((num[5] + num[6]) & 0x07) << 6; // 32 & 64, 3-bit
	index += ((num[7] + num[8]) & 0x07) << 9; // 126 & 256, 3-bit
	index += ((num[9] + num[10]) & 0x07) << 12; // 512 & 1k, 3-bit
	index += ((num[11]) & 0x03) << 15; // 2k ~ 32k, 2-bit ea.
	index += ((num[12]) & 0x03) << 17;
	index += ((num[13]) & 0x03) << 19;
	index += ((num[14]) & 0x03) << 21;
	index += ((num[15]) & 0x03) << 23;

	return index;
}

template<int transpose, int qu0, int qu1>
u64 indexnum2x(const board& b) { // 25-bit
	board o = b;
	if (transpose) o.transpose();
	auto& m = o.query(qu0).count;
	auto& n = o.query(qu1).count;

	register u64 index = 0;
	index += ((m[1] + n[1] + m[2] + n[2]) & 0x07) << 0; // 2 & 4, 3-bit
	index += ((m[3] + n[3] + m[4] + n[4]) & 0x07) << 3; // 8 & 16, 3-bit
	index += ((m[5] + n[5] + m[6] + n[6]) & 0x07) << 6; // 32 & 64, 3-bit
	index += ((m[7] + n[7] + m[8] + n[8]) & 0x07) << 9; // 126 & 256, 3-bit
	index += ((m[9] + n[9] + m[10] + n[10]) & 0x07) << 12; // 512 & 1k, 3-bit
	index += ((m[11] + n[11]) & 0x03) << 15; // 2k ~ 32k, 2-bit ea.
	index += ((m[12] + n[12]) & 0x03) << 17;
	index += ((m[13] + n[13]) & 0x03) << 19;
	index += ((m[14] + n[14]) & 0x03) << 21;
	index += ((m[15] + n[15]) & 0x03) << 23;

	return index;
}

template<int isomorphic>
u64 indexmono(const board& b) { // 24-bit
	board k = b;
	k.rotate(isomorphic);
	if (isomorphic >= 4) k.mirror();
	return k.mono();
}

template<int isomorphic>
u64 indexmax(const board& b) { // 16-bit
	board k = b;
	k.rotate(isomorphic);
	if (isomorphic >= 4) k.mirror();
	return k.mask(k.max());
}

void make_indexers() {

	indexer::make(0x00012367, utils::index6t<0,1,2,3,6,7>);
	indexer::make(0x0037bfae, utils::index6t<3,7,11,15,10,14>);
	indexer::make(0x00fedc98, utils::index6t<15,14,13,12,9,8>);
	indexer::make(0x00c84051, utils::index6t<12,8,4,0,5,1>);
	indexer::make(0x00fb7362, utils::index6t<15,11,7,3,6,2>);
	indexer::make(0x00cdefab, utils::index6t<12,13,14,15,10,11>);
	indexer::make(0x00048c9d, utils::index6t<0,4,8,12,9,13>);
	indexer::make(0x00321054, utils::index6t<3,2,1,0,5,4>);
	indexer::make(0x004567ab, utils::index6t<4,5,6,7,10,11>);
	indexer::make(0x0026ae9d, utils::index6t<2,6,10,14,9,13>);
	indexer::make(0x00ba9854, utils::index6t<11,10,9,8,5,4>);
	indexer::make(0x00d95162, utils::index6t<13,9,5,1,6,2>);
	indexer::make(0x00ea6251, utils::index6t<14,10,6,2,5,1>);
	indexer::make(0x0089ab67, utils::index6t<8,9,10,11,6,7>);
	indexer::make(0x00159dae, utils::index6t<1,5,9,13,10,14>);
	indexer::make(0x00765498, utils::index6t<7,6,5,4,9,8>);
	indexer::make(0x00012456, utils::index6t<0,1,2,4,5,6>);
	indexer::make(0x0037b26a, utils::index6t<3,7,11,2,6,10>);
	indexer::make(0x00fedba9, utils::index6t<15,14,13,11,10,9>);
	indexer::make(0x00c84d95, utils::index6t<12,8,4,13,9,5>);
	indexer::make(0x00fb7ea6, utils::index6t<15,11,7,14,10,6>);
	indexer::make(0x00cde89a, utils::index6t<12,13,14,8,9,10>);
	indexer::make(0x00048159, utils::index6t<0,4,8,1,5,9>);
	indexer::make(0x00321765, utils::index6t<3,2,1,7,6,5>);
	indexer::make(0x0045689a, utils::index6t<4,5,6,8,9,10>);
	indexer::make(0x0026a159, utils::index6t<2,6,10,1,5,9>);
	indexer::make(0x00ba9765, utils::index6t<11,10,9,7,6,5>);
	indexer::make(0x00d95ea6, utils::index6t<13,9,5,14,10,6>);
	indexer::make(0x00ea6d95, utils::index6t<14,10,6,13,9,5>);
	indexer::make(0x0089a456, utils::index6t<8,9,10,4,5,6>);
	indexer::make(0x0015926a, utils::index6t<1,5,9,2,6,10>);
	indexer::make(0x00765ba9, utils::index6t<7,6,5,11,10,9>);
	indexer::make(0x00000123, utils::index4t<0,1,2,3>);
	indexer::make(0x00004567, utils::index4t<4,5,6,7>);
	indexer::make(0x000089ab, utils::index4t<8,9,10,11>);
	indexer::make(0x0000cdef, utils::index4t<12,13,14,15>);
	indexer::make(0x000037bf, utils::index4t<3,7,11,15>);
	indexer::make(0x000026ae, utils::index4t<2,6,10,14>);
	indexer::make(0x0000159d, utils::index4t<1,5,9,13>);
	indexer::make(0x0000048c, utils::index4t<0,4,8,12>);
	indexer::make(0x0000fedc, utils::index4t<15,14,13,12>);
	indexer::make(0x0000ba98, utils::index4t<11,10,9,8>);
	indexer::make(0x00007654, utils::index4t<7,6,5,4>);
	indexer::make(0x00003210, utils::index4t<3,2,1,0>);
	indexer::make(0x0000c840, utils::index4t<12,8,4,0>);
	indexer::make(0x0000d951, utils::index4t<13,9,5,1>);
	indexer::make(0x0000ea62, utils::index4t<14,10,6,2>);
	indexer::make(0x0000fb73, utils::index4t<15,11,7,3>);
	indexer::make(0x00000145, utils::index4t<0,1,4,5>);
	indexer::make(0x00001256, utils::index4t<1,2,5,6>);
	indexer::make(0x00002367, utils::index4t<2,3,6,7>);
	indexer::make(0x00004589, utils::index4t<4,5,8,9>);
	indexer::make(0x0000569a, utils::index4t<5,6,9,10>);
	indexer::make(0x000067ab, utils::index4t<6,7,10,11>);
	indexer::make(0x000089cd, utils::index4t<8,9,12,13>);
	indexer::make(0x00009ade, utils::index4t<9,10,13,14>);
	indexer::make(0x0000abef, utils::index4t<10,11,14,15>);
	indexer::make(0x00003726, utils::index4t<3,7,2,6>);
	indexer::make(0x00007b6a, utils::index4t<7,11,6,10>);
	indexer::make(0x0000bfae, utils::index4t<11,15,10,14>);
	indexer::make(0x00002615, utils::index4t<2,6,1,5>);
	indexer::make(0x00006a59, utils::index4t<6,10,5,9>);
	indexer::make(0x0000ae9d, utils::index4t<10,14,9,13>);
	indexer::make(0x00001504, utils::index4t<1,5,0,4>);
	indexer::make(0x00005948, utils::index4t<5,9,4,8>);
	indexer::make(0x00009d8c, utils::index4t<9,13,8,12>);
	indexer::make(0x0000feba, utils::index4t<15,14,11,10>);
	indexer::make(0x0000eda9, utils::index4t<14,13,10,9>);
	indexer::make(0x0000dc98, utils::index4t<13,12,9,8>);
	indexer::make(0x0000ba76, utils::index4t<11,10,7,6>);
	indexer::make(0x0000a965, utils::index4t<10,9,6,5>);
	indexer::make(0x00009854, utils::index4t<9,8,5,4>);
	indexer::make(0x00007632, utils::index4t<7,6,3,2>);
	indexer::make(0x00006521, utils::index4t<6,5,2,1>);
	indexer::make(0x00005410, utils::index4t<5,4,1,0>);
	indexer::make(0x0000c8d9, utils::index4t<12,8,13,9>);
	indexer::make(0x00008495, utils::index4t<8,4,9,5>);
	indexer::make(0x00004051, utils::index4t<4,0,5,1>);
	indexer::make(0x0000d9ea, utils::index4t<13,9,14,10>);
	indexer::make(0x000095a6, utils::index4t<9,5,10,6>);
	indexer::make(0x00005162, utils::index4t<5,1,6,2>);
	indexer::make(0x0000eafb, utils::index4t<14,10,15,11>);
	indexer::make(0x0000a6b7, utils::index4t<10,6,11,7>);
	indexer::make(0x00006273, utils::index4t<6,2,7,3>);
	indexer::make(0x00003276, utils::index4t<3,2,7,6>);
	indexer::make(0x00002165, utils::index4t<2,1,6,5>);
	indexer::make(0x00001054, utils::index4t<1,0,5,4>);
	indexer::make(0x000076ba, utils::index4t<7,6,11,10>);
	indexer::make(0x000065a9, utils::index4t<6,5,10,9>);
	indexer::make(0x00005498, utils::index4t<5,4,9,8>);
	indexer::make(0x0000bafe, utils::index4t<11,10,15,14>);
	indexer::make(0x0000a9ed, utils::index4t<10,9,14,13>);
	indexer::make(0x000098dc, utils::index4t<9,8,13,12>);
	indexer::make(0x0000fbea, utils::index4t<15,11,14,10>);
	indexer::make(0x0000b7a6, utils::index4t<11,7,10,6>);
	indexer::make(0x00007362, utils::index4t<7,3,6,2>);
	indexer::make(0x0000ead9, utils::index4t<14,10,13,9>);
	indexer::make(0x0000a695, utils::index4t<10,6,9,5>);
	indexer::make(0x00006251, utils::index4t<6,2,5,1>);
	indexer::make(0x0000d9c8, utils::index4t<13,9,12,8>);
	indexer::make(0x00009584, utils::index4t<9,5,8,4>);
	indexer::make(0x00005140, utils::index4t<5,1,4,0>);
	indexer::make(0x0000cd89, utils::index4t<12,13,8,9>);
	indexer::make(0x0000de9a, utils::index4t<13,14,9,10>);
	indexer::make(0x0000efab, utils::index4t<14,15,10,11>);
	indexer::make(0x00008945, utils::index4t<8,9,4,5>);
	indexer::make(0x00009a56, utils::index4t<9,10,5,6>);
	indexer::make(0x0000ab67, utils::index4t<10,11,6,7>);
	indexer::make(0x00004501, utils::index4t<4,5,0,1>);
	indexer::make(0x00005612, utils::index4t<5,6,1,2>);
	indexer::make(0x00006723, utils::index4t<6,7,2,3>);
	indexer::make(0x00000415, utils::index4t<0,4,1,5>);
	indexer::make(0x00004859, utils::index4t<4,8,5,9>);
	indexer::make(0x00008c9d, utils::index4t<8,12,9,13>);
	indexer::make(0x00001526, utils::index4t<1,5,2,6>);
	indexer::make(0x0000596a, utils::index4t<5,9,6,10>);
	indexer::make(0x00009dae, utils::index4t<9,13,10,14>);
	indexer::make(0x00002637, utils::index4t<2,6,3,7>);
	indexer::make(0x00006a7b, utils::index4t<6,10,7,11>);
	indexer::make(0x0000aebf, utils::index4t<10,14,11,15>);
	indexer::make(0xff000000, utils::indexmerge);
	indexer::make(0xfe000000, utils::indexnum0);
	indexer::make(0xfe000001, utils::indexnum1);
	indexer::make(0xfe000002, utils::indexnum2);
	indexer::make(0xfe800002, utils::indexnum2x<0, 0, 1>);
	indexer::make(0xfe900002, utils::indexnum2x<0, 2, 3>);
	indexer::make(0xfec00002, utils::indexnum2x<1, 0, 1>);
	indexer::make(0xfed00002, utils::indexnum2x<1, 2, 3>);
	indexer::make(0xfd000000, utils::indexmono<0>);
	indexer::make(0xfd100000, utils::indexmono<1>);
	indexer::make(0xfd200000, utils::indexmono<2>);
	indexer::make(0xfd300000, utils::indexmono<3>);
	indexer::make(0xfd400000, utils::indexmono<4>);
	indexer::make(0xfd500000, utils::indexmono<5>);
	indexer::make(0xfd600000, utils::indexmono<6>);
	indexer::make(0xfd700000, utils::indexmono<7>);
	indexer::make(0xfc000000, utils::indexmax<0>);
	indexer::make(0xfc100000, utils::indexmax<1>);
	indexer::make(0xfc200000, utils::indexmax<2>);
	indexer::make(0xfc300000, utils::indexmax<3>);
	indexer::make(0xfc400000, utils::indexmax<4>);
	indexer::make(0xfc500000, utils::indexmax<5>);
	indexer::make(0xfc600000, utils::indexmax<6>);
	indexer::make(0xfc700000, utils::indexmax<7>);
}

void make_weights(const std::string& value = "") {

	if (value.empty()) {
		// make default weights
		for (auto& p : utils::patt6t) {
			weight::make(utils::hashfx(p), std::pow(utils::base, 6));
		}
		weight::make(0xfe000001, 1 << 25);
//		weight::make(0xfe000002, 1 << 25);
//		weight::make(0xfd000000, 1 << 24);
		weight::make(0xff000000, 1 << 16);

	} else {
		// 0x0a:100 0xab(10) 0x123456a[100]
		std::string in(value);
		while (in.find_first_of(":()[],") != std::string::npos)
			in[in.find_first_of(":()[],")] = ' ';
		std::stringstream wghtin(in);
		u32 sign, size;
		while (wghtin >> std::hex >> sign && wghtin >> std::dec >> size) {
			weight::make(sign, size);
		}
	}

}
void make_features(const std::string& value = "") {

	if (value.empty()) {
		// make default features
		for (auto& p : utils::patt6t) {
			const u32 wsign = utils::hashfx(p);
			for (auto fx : utils::mapfx) {
				feature::make(wsign, utils::hashfx(p));
				std::for_each(p.begin(), p.end(), fx);
			}
		}
		feature::make(0xfe000001, 0xfe000001);
//		feature::make(0xfe000002, 0xfe000002);
//		feature::make(0xfe000002, 0xfe800002);
//		feature::make(0xfe000002, 0xfe900002);
//		feature::make(0xfe000002, 0xfec00002);
//		feature::make(0xfe000002, 0xfed00002);
//		feature::make(0xfd000000, 0xfd000000);
//		feature::make(0xfd000000, 0xfd000001);
//		feature::make(0xfd000000, 0xfd000002);
//		feature::make(0xfd000000, 0xfd000003);
		feature::make(0xff000000, 0xff000000);
	} else {
		// weight:indexer weight(indexer) weight[indexer]
		std::string in(value);
		while (in.find_first_of(":()[],") != std::string::npos)
			in[in.find_first_of(":()[],")] = ' ';
		std::stringstream featin(in);
		u32 wght, idxr;
		while (featin >> std::hex >> wght && featin >> std::hex >> idxr) {
			feature::make(wght, idxr);
		}
	}
}

inline numeric estimate(const board& state,
		const feature::iter begin = feature::begin(), const feature::iter end = feature::end()) {
	register numeric esti = 0;
	for (auto f = begin; f != end; f++)
		esti += (*f)[state];
	return esti;
}

inline numeric update(const board& state, const numeric& updv,
		const feature::iter begin = feature::begin(), const feature::iter end = feature::end()) {
	register numeric esti = 0;
	for (auto f = begin; f != end; f++)
		esti += ((*f)[state] += updv);
	return esti;
}

inline numeric update(const board& state,
		const numeric& curr, const numeric& accu, const numeric& alpha = moporgic::alpha,
		const feature::iter begin = feature::begin(), const feature::iter end = feature::end()) {
	return update(state, alpha * (accu - curr), begin, end);
}

void list_mapping() {
	for (auto it = weight::begin(); it != weight::end(); it++) {
		u32 usageK = ((sizeof(numeric) * it->length()) >> 10);
		u32 usageM = usageK >> 10;
		printf("weight(%08x)[%llu] = %d%c", it->signature(), it->length(),
				usageM ? usageM : usageK, usageM ? 'M' : 'K');
		std::vector<u32> feats;
		for (auto ft = feature::begin(); ft != feature::end(); ft++) {
			if (weight(*ft).signature() == it->signature())
				feats.push_back(feature(*ft).signature());
		}
		if (feats.size()) {
			std::cout << " :";
			for (auto f : feats) printf(" %08x", f);
		}
		std::cout << std::endl;
	}
}

} // utils


struct state {
	board move;
	i32 (board::*oper)();
	i32 score;
	numeric esti;
	state() : state(nullptr) {}
	state(i32 (board::*oper)()) : oper(oper), score(-1), esti(0) {}
	state(const state& s) = default;

	inline void assign(const board& b) {
		move = b;
		score = (move.*oper)();
	}
	inline numeric estimate(const feature::iter begin = feature::begin(), const feature::iter end = feature::end()) {
		if (score >= 0) {
			esti = score + utils::estimate(move, begin, end);
		} else {
			esti = -std::numeric_limits<numeric>::max();
		}
		return esti;
	}
	inline numeric update(const numeric& accu, const numeric& alpha = moporgic::alpha,
			const feature::iter begin = feature::begin(), const feature::iter end = feature::end()) {
		esti = score + utils::update(move, alpha * (accu - (esti - score)), begin, end);
		return esti;
	}

	inline void operator <<(const board& b) {
		assign(b);
		estimate();
	}
	inline numeric operator +=(const numeric& v) {
		estimate();
		return update(v);
	}
	inline numeric operator +=(const state& s) {
		return operator +=(s.esti);
	}

	inline void operator >>(board& b) const { b = move; }
	inline bool operator >(const state& s) const { return esti > s.esti; }

	void operator >>(std::ostream& out) const {
		move >> out;
		moporgic::write(out, score);
	}
	void operator <<(std::istream& in) {
		move << in;
		moporgic::read(in, score);
	}
	inline operator bool() const { return score >= 0; }
};
struct select {
	state move[4];
	state *best;
	select() : best(move) {
		move[0] = state(&board::up);
		move[1] = state(&board::right);
		move[2] = state(&board::down);
		move[3] = state(&board::left);
	}
	inline select& operator ()(const board& b) {
		move[0] << b;
		move[1] << b;
		move[2] << b;
		move[3] << b;
		return update();
	}
	inline select& operator ()(const board& b, const feature::iter begin, const feature::iter end) {
		move[0].assign(b);
		move[1].assign(b);
		move[2].assign(b);
		move[3].assign(b);
		move[0].estimate(begin, end);
		move[1].estimate(begin, end);
		move[2].estimate(begin, end);
		move[3].estimate(begin, end);
		return update();
	}
	inline select& update() {
		return update_rand();
		best = move;
		if (move[1] > *best) best = move + 1;
		if (move[2] > *best) best = move + 2;
		if (move[3] > *best) best = move + 3;
		return *this;
	}
	inline select& update_rand() {
		const u32 i = std::rand() % 4;
		best = move + i;
		if (move[(i + 1) % 4] > *best) best = move + ((i + 1) % 4);
		if (move[(i + 2) % 4] > *best) best = move + ((i + 2) % 4);
		if (move[(i + 3) % 4] > *best) best = move + ((i + 3) % 4);
		return *this;
	}
	inline void shuffle() { std::random_shuffle(move, move + 4); }
	inline select& operator <<(const board& b) { return operator ()(b); }
	inline void operator >>(std::vector<state>& path) const { path.push_back(*best); }
	inline void operator >>(state& s) const { s = (*best); }
	inline void operator >>(board& b) const { *best >> b; }
	inline operator bool() const { return score() != -1; }
	inline i32 score() const { return best->score; }
	inline numeric esti() const { return best->esti; }
};
struct statistic {
	u64 limit;
	u64 loop;
	u64 check;

	struct record {
		u64 score;
		u64 win;
		u64 time;
		u64 opers;
		u32 max;
		u32 hash;
		std::array<u32, 32> count;
	} total, local;

	void init(const u64& max, const u64& chk = 1000) {
		limit = max * chk;
		loop = 1;
		check = chk;

		total = {};
		local = {};
		local.time = moporgic::millisec();
	}
	u64 operator++(int) { return (++loop) - 1; }
	u64 operator++() { return (++loop); }
	operator bool() const { return loop <= limit; }
	bool checked() const { return (loop % check) == 0; }

	void update(const u32& score, const u32& hash, const u32& opers) {
		local.score += score;
		local.hash |= hash;
		local.opers += opers;
		if (hash >= 2048) local.win++;
		local.max = std::max(local.max, score);

		if ((loop % check) != 0) return;

		u64 currtimept = moporgic::millisec();
		u64 elapsedtime = currtimept - local.time;
		total.score += local.score;
		total.win += local.win;
		total.time += elapsedtime;
		total.opers += local.opers;
		total.hash |= local.hash;
		total.max = std::max(total.max, local.max);

		std::cout << std::endl;
		char buf[64];
		snprintf(buf, sizeof(buf), "%03llu/%03llu %llums %.2fops",
				loop / check,
				limit / check,
				elapsedtime,
				local.opers * 1000.0 / elapsedtime);
		std::cout << buf << std::endl;
		snprintf(buf, sizeof(buf), "local:  avg=%llu max=%u tile=%u win=%.2f%%",
				local.score / check,
				local.max,
				math::msb32(local.hash),
				local.win * 100.0 / check);
		std::cout << buf << std::endl;
		snprintf(buf, sizeof(buf), "total:  avg=%llu max=%u tile=%u win=%.2f%%",
				total.score / loop,
				total.max,
				math::msb32(total.hash),
				total.win * 100.0 / loop);
		std::cout << buf << std::endl;

		local.score = 0;
		local.win = 0;
		local.time = currtimept;
		local.opers = 0;
		local.hash = 0;
		local.max = 0;
	}

	inline void updatec(const u32& score, const u32& hash, const u32& opers) {
		update(score, hash, opers);
		u32 max = std::log2(hash);
		local.count[max]++;
		total.count[max]++;
		if ((loop % check) != 0) return;
		local.count = {};
	}

	void summary(const u32& begin = 1, const u32& end = 17) {
		std::cout << "max tile summary" << std::endl;

		auto iter = total.count.begin();
		double sum = std::accumulate(iter + begin, iter + end, 0);
		char buf[64];
		for (u32 i = begin; i < end; ++i) {
			snprintf(buf, sizeof(buf), "%d:\t%8d%8.2f%%%8.2f%%",
					(1 << i) & 0xfffffffeu, total.count[i],
					(total.count[i] * 100.0 / sum),
					(std::accumulate(iter + begin, iter + i + 1, 0) * 100.0 / sum));
			std::cout << buf << std::endl;
		}
	}
};

int main(int argc, const char* argv[]) {
	u32 train = 100;
	u32 test = 10;
	u32 timestamp = std::time(nullptr);
	u32 seed = timestamp;
	numeric& alpha = moporgic::alpha;
	utils::options wopts;
	utils::options fopts;
	utils::options opts;


	auto valueof = [&](int& i, const char* def) -> const char* {
		if (i + 1 < argc && *(argv[i + 1]) != '-') return argv[++i];
		if (def != nullptr) return def;
		std::cerr << "invalid: " << argv[i] << std::endl;
		std::exit(1);
	};
	for (int i = 1; i < argc; i++) {
		switch (to_hash(argv[i])) {
		case to_hash("-a"):
		case to_hash("--alpha"):
			alpha = std::stod(valueof(i, nullptr));
			break;
		case to_hash("-a/"):
		case to_hash("--alpha-divide"):
			alpha /= std::stod(valueof(i, nullptr));
			break;
		case to_hash("-t"):
		case to_hash("--train"):
			train = u32(std::stod(valueof(i, nullptr)));
			break;
		case to_hash("-T"):
		case to_hash("--test"):
			test = u32(std::stod(valueof(i, nullptr)));
			break;
		case to_hash("-s"):
		case to_hash("--seed"):
		case to_hash("--srand"):
			seed = u32(std::stod(valueof(i, nullptr)));
			break;
		case to_hash("-wio"):
		case to_hash("--weight-input-output"):
			wopts["input"] = valueof(i, "tdl2048.weight");
			wopts["output"] = wopts["input"];
			break;
		case to_hash("-wi"):
		case to_hash("--weight-input"):
			wopts["input"] = valueof(i, "tdl2048.weight");
			break;
		case to_hash("-wo"):
		case to_hash("--weight-output"):
			wopts["output"] = valueof(i, "tdl2048.weight");
			break;
		case to_hash("-fio"):
		case to_hash("--feature-input-output"):
			fopts["input"] = valueof(i, "tdl2048.feature");
			fopts["output"] = fopts["input"];
			break;
		case to_hash("-fi"):
		case to_hash("--feature-input"):
			fopts["input"] = valueof(i, "tdl2048.feature");
			break;
		case to_hash("-fo"):
		case to_hash("--feature-output"):
			fopts["output"] = valueof(i, "tdl2048.feature");
			break;
		case to_hash("-w"):
		case to_hash("--weight"):
		case to_hash("--weight-value"):
			for (std::string w; (w = valueof(i, "")).size(); )
				wopts["value"] += (w += ',');
			break;
		case to_hash("-f"):
		case to_hash("--feature"):
		case to_hash("--feature-value"):
			for (std::string f; (f = valueof(i, "")).size(); )
				fopts["value"] += (f += ',');
			break;
		case to_hash("--modify"):
			break;
		case to_hash("-o"):
		case to_hash("--option"):
		case to_hash("--options"):
		case to_hash("--extra"):
			for (std::string o; (o = valueof(i, "")).size(); )
				opts += o;
			break;
		case to_hash("-tt"):
		case to_hash("--train-type"):
			opts["train-type"] = valueof(i, "");
			break;
		default:
			std::cerr << "unknown: " << argv[i] << std::endl;
			std::exit(1);
			break;
		}
	}

	std::srand(seed);
	std::cout << "TDL2048+ LOG" << std::endl;
	std::copy(argv, argv + argc, std::ostream_iterator<const char*>(std::cout, " "));
	std::cout << std::endl;
	std::cout << "timestamp = " << timestamp << std::endl;
	std::cout << "srand = " << seed << std::endl;
	std::cout << "alpha = " << alpha << std::endl;
//	printf("board::look[%d] = %lluM", (1 << 20), ((sizeof(board::cache) * (1 << 20)) >> 20));
	std::cout << std::endl;


	utils::make_indexers();

	if (weight::load(wopts["input"]) == false) {
		if (wopts["input"].size())
			std::cerr << "warning: " << wopts["input"] << " not loaded!" << std::endl;
		utils::make_weights(wopts["value"]);
	}

	if (feature::load(fopts["input"]) == false) {
		if (fopts["input"].size())
			std::cerr << "warning: " << fopts["input"] << " not loaded!" << std::endl;
		utils::make_features(fopts["value"]);
	}

	utils::list_mapping();


	board b;
	state last;
	select best;
	statistic stats;
	std::vector<state> path;
	path.reserve(20000);

	if (train) std::cout << std::endl << "start training..." << std::endl;
	switch (to_hash(opts["train-type"])) {

	case to_hash("backward/online"):
		std::cerr << "warning: use backward/offline instead" << std::endl;
		// no break
	default:
	case to_hash("backward/offline"):
	case to_hash("backward"):
		for (stats.init(train); stats; stats++) {

			register u32 score = 0;
			register u32 opers = 0;

			for (b.init(); best << b; b.next()) {
				score += best.score();
				opers += 1;
				best >> path;
				best >> b;
			}

			for (numeric v = 0; path.size(); path.pop_back()) {
				v = (path.back() += v);
			}

			stats.update(score, b.hash(), opers);
		}
		break;

	case to_hash("forward"):
	case to_hash("forward/online"):
		for (stats.init(train); stats; stats++) {

			register u32 score = 0;
			register u32 opers = 0;

			b.init();
			best << b;
			score += best.score();
			opers += 1;
			best >> last;
			best >> b;
			b.next();
			while (best << b) {
				last += best.esti();
				score += best.score();
				opers += 1;
				best >> last;
				best >> b;
				b.next();
			}
			last += 0;

			stats.update(score, b.hash(), opers);
		}
		break;

	case to_hash("forward/offline"):
		for (stats.init(train); stats; stats++) {

			register u32 score = 0;
			register u32 opers = 0;

			for (b.init(); best << b; b.next()) {
				score += best.score();
				opers += 1;
				best >> path;
				best >> b;
			}

			u32 n = path.size();
			path.push_back(state());
			for (u32 i = 0; i < n; i++) {
				if (path[i + 1].move != 0) path[i + 1].estimate();
				path[i] += path[i + 1];
			}
			path.clear();

			stats.update(score, b.hash(), opers);
		}
		break;

	case to_hash("random/mem"):
	case to_hash("random/mem/offline"):
	{
		if (opts["history-size"].empty()) opts["history-size"] = "200000";
		if (opts["training-batch"].empty()) opts["training-batch"] = "1";
		u32 memsize = std::stol(opts["history-size"]);
		u32 batch = std::stol(opts["training-batch"]);

		auto *history = new std::pair<state, state>[memsize]();
		u32 size = 0;

		for (stats.init(train); stats; stats++) {

			register u32 score = 0;
			register u32 opers = 0;

			b.init();
			best << b;
			score += best.score();
			opers += 1;
			best >> last;
			best >> b;
			for (b.next(); best << b; b.next()) {
				history[(size++) % memsize] = std::make_pair(last, *(best.best));
				score += best.score();
				opers += 1;
				best >> last;
				best >> b;
			}
			history[(size++) % memsize] = std::make_pair(last, state());

			u32 range = std::min(size, memsize);
			for (u32 s = batch * opers; s; --s) {
				auto& pair = history[randx() % range];
				if (pair.second.move != 0) pair.second.estimate();
				pair.first += pair.second;
			}

			stats.update(score, b.hash(), opers);
		}

		delete[] history;

		break;
	}
	case to_hash("random/mem/online"):
	{
		if (opts["history-size"].empty()) opts["history-size"] = "200000";
		if (opts["training-batch"].empty()) opts["training-batch"] = "1";
		u32 memsize = std::stol(opts["history-size"]);
		u32 batch = std::stol(opts["training-batch"]);

		auto *history = new std::pair<state, state>[memsize]();
		u32 size = 0;

		for (stats.init(train); stats; stats++) {

			register u32 score = 0;
			register u32 opers = 0;

			b.init();
			best << b;
			score += best.score();
			opers += 1;
			best >> last;
			best >> b;
			for (b.next(); best << b; b.next()) {
				history[(size++) % memsize] = std::make_pair(last, *(best.best));
				u32 range = std::min(size, memsize);
				for (u32 s = batch; s; --s) {
					auto& pair = history[randx() % range];
					pair.first += pair.second;
				}
				score += best.score();
				opers += 1;
				best >> last;
				best >> b;
			}
			history[(size++) % memsize] = std::make_pair(last, state());
			u32 range = std::min(size, memsize);
			for (u32 s = batch; s; --s) {
				auto& pair = history[randx() % range];
				pair.first += pair.second;
			}

			stats.update(score, b.hash(), opers);
		}

		delete[] history;

		break;
	}

	case to_hash("random/online"):
		std::cerr << "warning: use random/offline instead" << std::endl;
		// no break
	case to_hash("random/offline"):
	case to_hash("random"):
		std::vector<u32> seq;
		seq.reserve(20000);

		for (stats.init(train); stats; stats++) {

			register u32 score = 0;
			register u32 opers = 0;

			for (b.init(); best << b; b.next()) {
				seq.push_back(opers);
				score += best.score();
				opers += 1;
				best >> path;
				best >> b;
			}

			path.push_back(state());
			std::random_shuffle(seq.begin(), seq.end());
			for (u32 i : seq) {
				if (path[i + 1].move != 0) path[i + 1].estimate();
				path[i] += path[i + 1];
			}
			path.clear();
			seq.clear();

			stats.update(score, b.hash(), opers);
		}
		break;
	}

	weight::save(wopts["output"]);
	feature::save(fopts["output"]);




	if (test) std::cout << std::endl << "start testing..." << std::endl;
	for (stats.init(test); stats; stats++) {

		register u32 score = 0;
		register u32 opers = 0;

		for (b.init(); best << b; b.next()) {
			score += best.score();
			opers += 1;
			best >> b;
		}

		stats.update(score, b.hash(), opers);
	}

	return 0;
}

} // namespace moporgic

int main(int argc, const char* argv[]) {
	return moporgic::main(argc, argv);
}
