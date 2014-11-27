
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
#include <vector>
#include <typeinfo>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <boost/current_function.hpp>
#include <nark/hash_strmap.hpp>
#include <nark/util/profiling.cpp> // for simplify
#include <nark/fstring.cpp> // for simplify

#if defined(_WIN32) || defined(_WIN64)
	#include <Windows.h>
#endif

using namespace nark;

static long Data_cnt = 0;
struct Data {
	int x;
	enum state_t{
		cons_def,
		cons_copy,
		destroyed,
	};
	state_t state;
	Data() : x(), state(cons_def) {
		Data_cnt++;
	}
	Data(const Data& y) {
		Data_cnt++;
		x = y.x;
		state = cons_copy;
	}
	~Data() {
		assert(destroyed != state);
		Data_cnt--;
		state = destroyed;
	}
	friend bool operator==(const Data& x, const Data& y) {
		assert(destroyed != x.state);
		assert(destroyed != y.state);
		return x.x == y.x;
	}
	friend bool operator!=(const Data& x, const Data& y) {
		return !(x == y);
	}
};

static char buf[] = "-Map----++++----++++----12345678";
int start = 0; // make it easy for unaligned test

// this function should be as fast as, so it don't impact the timing very much
void gen(unsigned x) {
	x = (x + 101) * 100001 + 1;
	for (int i = 7; i >= 0; --i) {
		buf[sizeof(buf)-9+i] = "0123456789ABCDEF"[x & 15];
		x >>= 4;
	}
}

template<class Map>
void insert(Map& m, int loop) {
//	printf("%s\n", BOOST_CURRENT_FUNCTION);
	for (int i = 0; i < loop; ++i) {
		gen(i);
//		printf("%09d: %s\n", i, buf);
		m[typename Map::key_type(buf + start,sizeof(buf)-1-start)].x = i;
	}
}

template<class Map>
void lookup(Map& m, int loop) {
//	printf("%s\n", BOOST_CURRENT_FUNCTION);
	for (int i = 0; i < loop; ++i) {
		gen(i);
//		printf("%09d: %s\n", i, buf);
		for (int j = 0; j < 20; ++j)
			m[typename Map::key_type(buf + start,sizeof(buf)-1-start)].x++;
	}
}

template<class Map1, class Map2>
void check(const Map1& m1, const Map2& m2, int loop) {
	for (int i = 0; i < loop; ++i) {
		gen(i);
		typename Map1::const_iterator i1 = m1.find(buf + start);
		typename Map2::const_iterator i2 = m2.find(buf + start);
		if ((m1.end() == i1 && m2.end() == i2) ||
			(m1.end() != i1 && m2.end() != i2))
			i = i; // match
		else
			printf("mismatch: %s->[%d, %d]\n", buf + start, i1->second.x, i2->second.x);
	}
}

struct EraseCond {
//	bool operator()(const std::pair<fstring, long>& x) const
	template<class KV>
	bool operator()(KV& x) const
	{
		return x.second.x < n * 5 / 8;
	}
	int n;
};

// microsoft's default hash for std::string is very bad and even make the test can't run
// gcc's hash for std::string is also bad than my hash in this test
#ifdef _MSC_VER
//	typedef SYS_HASH_MAP<std::string, Data, fstring_func::hash, fstring_func::equal> SysHashMap;
	class SysHashMap : public SYS_HASH_MAP<std::string, Data> {};
//	typedef std::map<std::string, long> SysHashMap; // MS unordered_map is very poor, 2000 times slower
#else
//	typedef SYS_HASH_MAP<std::string, long, fstring_func::hash, fstring_func::equal> SysHashMap;
//	typedef SYS_HASH_MAP<std::string, long> SysHashMap;
	class SysHashMap : public SYS_HASH_MAP<std::string, Data> {};
#endif

class SysTreeMap : public std::map<std::string, Data> {};

template<class ValuePlace, class CopyStrategy>
class FastStrMap : public fast_hash_strmap<std::string, Data, fstring_func::hash, fstring_func::equal, ValuePlace, CopyStrategy> {};

template<class CopyStrategy, class ValuePlace>
void test(int loop) {
	printf("enter %s : SP_ALIGN=%d : %s\n", BOOST_CURRENT_FUNCTION, SP_ALIGN, start ? "unaligned" : "aligned");
//	using namespace std;
	nark::profiling pf;
	hash_strmap<> sset, sset2 = sset;
#define fstrmap FastStrMap<ValuePlace, CopyStrategy>
	fstrmap fm, fm2(fm);
	SysHashMap sm;
	SysTreeMap mm;
	fm.set_load_factor(0.98);
	fm.reserve(loop);
	fm.disable_freelist();
	fm.enable_freelist();
	fm.disable_hash_cache();
    fm.enable_hash_cache();
//	fm.reserve_strpool((sizeof(buf)+7) * loop);
//	sm.rehash(loop * 3);
	long long t0,t1,t2,t3;
	printf("-loop=%d, fm.size=%ld, sm.size=%ld, insert...\n", loop, (long)fm.size(), (long)sm.size());
	 t0 = pf.now();
	insert(fm, loop);
	 t1 = pf.now();
	insert(sm, loop);
	 t2 = pf.now();
	insert(mm, loop);
	 t3 = pf.now();
	printf("+loop=%d, fm.size=%ld, sm.size=%ld, mm.size=%ld\n", loop, (long)fm.size(), (long)sm.size(), (long)mm.size());
	printf("seconds[fm=%f, sm=%f, mm=%f, s/f=%f, m/f=%f]\n", pf.sf(t0,t1), pf.sf(t1,t2), pf.sf(t2,t3), (t2-t1+.1)/(t1-t0+.1), (t3-t2+.1)/(t1-t0+.1));
	std::vector<unsigned> hist;
	fm.bucket_histogram(hist);
	printf("bucket_size=%ld\n", long(fm.bucket_size()));
	double total_lookup_len = 0;
	for (size_t i = 0; i < hist.size(); ++i) {
		total_lookup_len += (1 + i) * i * hist[i] * 0.5;
		if (hist[i])
			printf("collision_listlen=%u count=%07u ratio=%f\n", unsigned(i), hist[i], double(i*hist[i])/fm.size());
	}
	printf("average-lookup-length=%f\n", total_lookup_len/fm.size());
	printf("lookup...\n");
	 t0 = pf.now();
	lookup(fm, loop);
	 t1 = pf.now();
	lookup(sm, loop);
	 t2 = pf.now();
	lookup(mm, loop);
	 t3 = pf.now();
	printf("seconds[fm=%f, sm=%f, mm=%f, s/f=%f, m/f=%f]\n", pf.sf(t0,t1), pf.sf(t1,t2), pf.sf(t2,t3), (t2-t1+.1)/(t1-t0+.1), (t3-t2+.1)/(t1-t0+.1));
	printf("__MQPS_[fm=%f, sm=%f, mm=%f]\n", loop*20/pf.uf(t0,t1), loop*20/pf.uf(t1,t2), loop*20/pf.uf(t2,t3));
	printf("+loop=%d, fm.size=%ld, sm.size=%ld, mm.size=%ld\n", loop, (long)fm.size(), (long)sm.size(), (long)mm.size());

	check(fm, sm, loop);

	EraseCond cond; cond.n = loop;
	fm2 = fm;
	check(fm2, sm, loop);
	for (SysTreeMap::iterator ti = mm.begin(); ti != mm.end(); ) {
		if (cond(*ti)) {
			const std::string k = ti->first;
			fm.erase(k);
			sm.erase(k);
			mm.erase(ti++);
		} else
			++ti;
	}
	fm2.erase_if(cond);
	check(fm, fm2, loop);
	fm.hash_value(0);
	assert(fm.size() == sm.size());
	assert(fm.size() == fm2.size());

	for (int i = 0; i < loop; ++i) {
		gen(i);
		typename fstrmap::const_iterator fi = fm.find(buf + start);
		SysHashMap::const_iterator iter = sm.find(buf + start);
		if ( (fm.end() == fi && sm.end() == iter) || (fm.end() != fi && sm.end() != iter) ) {
			// ok
		} else {
			printf("erased mismatch: %s->[i=%d, fi=(%s:%ld)]\n", buf + start, i, fi->first.p, (long)fi.get_index());
		}
	}

	t0 = pf.now();
	fm.sort_fast();
	t1 = pf.now();
	{
		printf("sort time = %f's, checking result\n", pf.sf(t0,t1));
		SysTreeMap::const_iterator iter = mm.begin();
		for (int i = 0, n = (int)fm.size(); i < n; ++i) {
			assert(!fm.is_deleted(i));
			fstring mmk(iter->first);
			fstring fmk(fm.key(i));
			if (fmk != mmk || iter->second != fm.val(i)) {
				printf("fm[buf + start=%s, val=%d]\n", fmk.p, fm.val(i).x);
				printf("mm[buf + start=%s, val=%d]\n", mmk.p, iter->second.x);
			}
			assert(fmk  == mmk);
			++iter;
		}
		assert(mm.end() == iter);

	//	check(fm, sm, loop); // after sort
	}
	lookup(fm, loop);
	lookup(sm, loop);
	lookup(mm, loop);
	check(fm, sm, loop);
#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
	long t4, t5;
	long fsum1 = 0, fsum2 = 0, fsum3 = 0, msum = 0, ssum = 0;
	t0 = pf.now();
	fm.for_each([&](fstring, long v){ fsum1 += v; });
	t1 = pf.now();
	for (auto x : fm) fsum2 += x.second;
	t2 = pf.now();
	{for (int i = 0, n = (int)fm.size(); i < n; ++i) fsum3 += fm.val(i);}
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
	printf("SP_ALIGN=%d sizeof(sizt_t)=%d-------------------\n", SP_ALIGN, int(sizeof(size_t)));
	int loop = argc >= 2 ? atoi(argv[1]) : 2000;
	start = 0;
	test<FastCopy, ValueInline>(loop);
	assert(0 == Data_cnt);

	start = 1;
	test<FastCopy, ValueInline>(loop);
	assert(0 == Data_cnt);
	test<FastCopy, ValueOut   >(loop);
	assert(0 == Data_cnt);
	test<SafeCopy, ValueInline>(loop);
	assert(0 == Data_cnt);
	test<SafeCopy, ValueOut   >(loop);
	assert(0 == Data_cnt);

	return 0;
}

