#include <nark/easy_use_hash_map.hpp>
#include <nark/fstring.cpp>

int main() {
	nark::easy_use_hash_map<std::string, int> str2int;
	nark::easy_use_hash_map<int, int> int2int;
	str2int["111"] = 111;
	int2int[11111] = 111;
	return 0;
}

