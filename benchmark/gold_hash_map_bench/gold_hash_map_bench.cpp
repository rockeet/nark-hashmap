
#ifdef _MSC_VER
//	#include <unordered_map>
//	#define SYS_HASH_MAP std::tr1::unordered_map
	#include <hash_map>
	#define SYS_HASH_MAP stdext::hash_map
#else
	#if defined(__GNUC__) && __GNUC__ <= 3
		#include <ext/hash_map>
		#define SYS_HASH_MAP __gnu_cxx::hash_map
	#else
		#include <tr1/unordered_map> // microsoft fails on this line
		#define SYS_HASH_MAP std::tr1::unordered_map
	#endif
#endif

#include <map>
#include <typeinfo>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <boost/current_function.hpp>
#include <nark/gold_hash_map.hpp>
#include <nark/hash_strmap.hpp>
#include <nark/util/profiling.cpp> // for simplify

#if defined(_WIN32) || defined(_WIN64)
	#include <Windows.h>
#endif

using namespace nark;

static char buf[] = "-Map----++++----++++----12345678";
int start = 0; // make it easy for unaligned test

// For minimized symbol length:
struct String : std::string {
	String(const char* s, size_t n) : std::string(s, n) {}
	String(const char* s) : std::string(s) {}
	String() {}
};
struct First {
	template<class XY>
	const String& operator()(const XY& xy) const { return xy.first;	}
};
struct Alloc : std::allocator<std::pair<const String, long int> > {
	template<class Y>
	Alloc(const Y& y) : std::allocator<std::pair<const String, long int> >(y) {}
	Alloc() {}
};
// End. For minimized symbol length

// this function should be as fast as, so it don't impact the timing very much
void gen(unsigned x, unsigned loop) {
	x = (x + 101) * 100001 % loop + 1;
//	if (x == 0x65) {
//		unsigned y = x; // for break point
//	}
	for (int i = 7; i >= 0; --i) {
		buf[sizeof(buf)-9+i] = "0123456789ABCDEF"[x & 15];
		x >>= 4;
	}
}

template<class Map>
void insert(Map& m, int loop) {
	printf("%s\n", BOOST_CURRENT_FUNCTION);
	for (int i = 0; i < loop; ++i) {
		gen(i, loop);
	//	printf("%09d: %s\n", i, buf);
		m.insert(std::make_pair(typename Map::key_type(buf + start,sizeof(buf)-1-start), 0)).first->second = i;
	}
}

template<class Map>
void lookup(Map& m, int loop) {
	printf("%s\n", BOOST_CURRENT_FUNCTION);
	for (int i = 0; i < loop; ++i) {
		gen(i, loop);
//		printf("%09d: %s\n", i, buf);
		for (int j = 0; j < 20; ++j)
			m.insert(std::make_pair(typename Map::key_type(buf + start,sizeof(buf)-1-start), 0)).first->second++;
	}
}

template<class Gold, class Map2>
void check(const Gold& fm, const Map2& m2, int loop, const char* title) {
	for (int i = 0; i < loop; ++i) {
		gen(i, loop);
		typename Gold::const_iterator i1 = fm.find(buf + start);
		typename Map2::const_iterator i2 = m2.find(buf + start);
		if ((fm.end() == i1 && m2.end() == i2) ||
			(fm.end() != i1 && m2.end() != i2))
			i = i; // match
		else
			printf("%s:[i=%d, id=%d] %s->[%ld, %ld]\n", title, i, int(i1.get_index())
					, buf + start, i1->second, i2->second);
	}
}
template<class Gold, class Map2>
void check(const Gold& fm, const Map2& m2, int loop) {
	check(fm, m2, loop, "mismatch");
}

struct EraseCond {
	bool operator()(const std::pair<String, long>& x) const {
		return x.second < n * 5 / 8;
	}
	int n;
};
struct HashEq : fstring_hash_equal_align {};
struct ElemTp : std::pair<String, long> {
	ElemTp(const std::pair<String, long>& x) : std::pair<String, long>(x) {}
	ElemTp() {}
};
template<class CopyStrategy, class ValuePlace>
struct Layout : node_layout<ElemTp, unsigned, CopyStrategy, ValuePlace>
{};

template<class CopyStrategy, class ValuePlace>
void test(int loop) {
	printf("enter %s\n", BOOST_CURRENT_FUNCTION);
//	using namespace std;
	nark::profiling pf;
	typedef gold_hash_tab<String, ElemTp
		, HashEq
		, First
		, Layout<CopyStrategy, ValuePlace>
//		, unsigned long, unsigned long long
	> goldmap_t;
	goldmap_t fm0, fm1(fm0), fm2(fm0), fm3, fm4, fm5, fm6;
// microsoft's default hash for String is very bad and even make the test can't run
// gcc's hash for String is also bad than my hash in this test
#ifdef _MSC_VER
	typedef SYS_HASH_MAP<String, long> syshmap_t;
//	typedef std::map<String, long> syshmap_t; // MS unordered_map is very poor, 2000 times slower
//	fm.reserve(loop);
#else
//	typedef SYS_HASH_MAP<String, long, std::hash<String>, std::equal_to<String> > syshmap_t;
//	#define syshmap_t SYS_HASH_MAP<String, long>
	#define syshmap_t SYS_HASH_MAP<String, long, fstring_func::hash, fstring_func::equal, Alloc>
#endif
	syshmap_t sm;
#define treemap_t std::map<String, long, std::less<String>, Alloc>
	treemap_t mm;
//	fm.set_load_factor(0.50);
	fm0.reserve(loop);
//	sm.rehash(loop * 3);
	long long t0,t1,t2,t3;
	printf("-loop=%d, fm.size=%ld, sm.size=%ld\n", loop, (long)fm0.size(), (long)sm.size());
	printf("insert\n");
	 t0 = pf.now();
	insert(fm0, loop);
	 t1 = pf.now();
	insert(sm, loop);
	 t2 = pf.now();
	insert(mm, loop);
	 t3 = pf.now();
	printf("seconds[fm=%f, sm=%f, mm=%f, s/f=%f, m/f=%f\n", pf.sf(t0,t1), pf.sf(t1,t2), pf.sf(t2,t3), (t2-t1+.1)/(t1-t0+.1), (t3-t2+.1)/(t1-t0+.1));
	printf("+loop=%d, fm.size=%ld, sm.size=%ld, mm.size=%ld\n", loop, (long)fm0.size(), (long)sm.size(), (long)mm.size());

	printf("lookup\n");
	 t0 = pf.now();
	lookup(fm0, loop);
	 t1 = pf.now();
	lookup(sm, loop);
	 t2 = pf.now();
	lookup(mm, loop);
	 t3 = pf.now();
	printf("seconds[fm=%f, sm=%f, mm=%f, s/f=%f, m/f=%f\n", pf.sf(t0,t1), pf.sf(t1,t2), pf.sf(t2,t3), (t2-t1+.1)/(t1-t0+.1), (t3-t2+.1)/(t1-t0+.1));
	printf("+loop=%d, fm.size=%ld, sm.size=%ld, mm.size=%ld\n", loop, (long)fm0.size(), (long)sm.size(), (long)mm.size());
	assert(fm0.size() == sm.size());
	check(fm0, sm, loop);

	EraseCond cond; cond.n = loop;
	fm1 = fm0;
	check(fm1, sm, loop);
	fm0 = fm0; fm0.disable_freelist(); //fm0.revoke_deleted();
	fm1 = fm0; fm1.disable_freelist(); fm1.erase_if(cond);
	fm2 = fm0; fm2. enable_freelist(); fm2.keepid_erase_if(cond);
	fm3 = fm0; fm3. enable_freelist(); fm3.erase_if(cond);
	fm4 = fm0; fm4.disable_freelist(); // erase by key
	fm5 = fm0; fm5. enable_freelist(); // erase by key
	int cnt = 0;
	for (typename treemap_t::iterator ti = mm.begin(); ti != mm.end(); ++cnt) {
		if (cond(*ti)) {
			const String k = ti->first;
			fm4.erase(k);
			fm5.erase(k);
			sm.erase(k);
			mm.erase(ti++);
		} else
			++ti;
	}
	printf("after erase: fm.size=%ld\n", (long)fm0.size());
	check(fm1, sm, loop, "erase mismatch");
	check(fm2, sm, loop, "erase mismatch");
	check(fm3, sm, loop, "erase mismatch");
	check(fm4, sm, loop, "erase mismatch");
	check(fm5, sm, loop, "erase mismatch");
	fm2.revoke_deleted();
	fm5.revoke_deleted();
	check(fm1, sm, loop, "revoke_deleted");
	fm0 = fm2;
	fm0 = fm3;
	assert(fm0.size() == sm.size());
	assert(fm0.size() == fm1.size());
	assert(fm0.size() == fm2.size());
	assert(fm0.size() == fm3.size());
	assert(fm0.size() == fm4.size());
	assert(fm0.size() == fm5.size());

	t0 = pf.now();
	fm0.sort();
	t1 = pf.now();
	for (size_t i = 1, n = fm0.size(); i < n; ++i) {
		assert(fm0.elem_at(i-1) < fm0.elem_at(i));
	}
	{
		printf("sort time = %f's, checking result\n", pf.sf(t0,t1));
		typename treemap_t::const_iterator iter = mm.begin();
		for (size_t i = 0, n = fm0.size(); i < n; ++i) {
			const String& mmk(iter->first);
			const String& fmk(fm0.key(i));
			if (fmk != mmk || iter->second != fm0.elem_at(i).second) {
				printf("after sort, fm[buf + start=%s, val=%ld]\n", fmk.c_str(), fm0.elem_at(i).second);
				printf("after sort, mm[buf + start=%s, val=%ld]\n", mmk.c_str(), iter->second);
			}
			assert(fmk == mmk);
			++iter;
		}
		assert(mm.end() == iter);

		check(fm0, sm, loop); // after sort
	}
	lookup(fm0, loop);
	lookup(sm, loop);
	lookup(mm, loop);
	check(fm0, sm, loop);
#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
	long t4, t5;
	long fsum1 = 0, fsum2 = 0, fsum3 = 0, msum = 0, ssum = 0;
	t0 = pf.now();
	fm0.for_each([&](String, long v){ fsum1 += v; });
	t1 = pf.now();
	for (auto x : fm0) fsum2 += x.second;
	t2 = pf.now();
	{for (int i = 0, n = (int)fm0.size(); i < n; ++i) fsum3 += fm0.elem_at(i).second;}
	t3 = pf.now();
	for (auto x : mm) msum += x.second;
	t4 = pf.now();
	for (auto x : sm) ssum += x.second;
	t5 = pf.now();
	if (fsum1 != msum || fsum2 != msum || fsum3 != msum || ssum != msum) {
		printf("sum don't match[fsum1=%ld, fsum2=%ld, msum=%ld]\n", fsum1, fsum2, msum);
	}
	printf("iteration(msec): fm.for_each=%f for:[fm=%f sf=%f mm=%f sm=%f], fm/ff=%f mm/ff=%f mm/fm=%f, mm/sf=%f sm/sf=%f\n",
			pf.mf(t0,t1), pf.mf(t1,t2), pf.mf(t2,t3), pf.mf(t3,t4), pf.mf(t4,t5),
			(t2-t1+.1)/(t1-t0+.1), (t4-t3+.1)/(t1-t0+.1), (t4-t3+.1)/(t2-t1+.1), (t4-t3+.1)/(t3-t2+.1), (t5-t4+.1)/(t3-t2+.1)
		  );
#endif
	printf("leave %s\n", BOOST_CURRENT_FUNCTION);
	printf("--------------------------------------------------------\n");
}

int main(int argc, char** argv) {
	int loop = argc >= 2 ? atoi(argv[1]) : 2000;
	start = 0;
	test<FastCopy, ValueInline>(loop);
	test<FastCopy, ValueOut   >(loop);
	test<SafeCopy, ValueInline>(loop);
	test<SafeCopy, ValueOut   >(loop);
    printf("All successed\n");
	return 0;
}

