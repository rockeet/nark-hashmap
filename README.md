nark-hashmap
============

A very fast and small(memory efficient) hash map: [Bechmark](http://nark.cc/p/?p=142)

## Quick Start

`nark::easy_use_hash_map<Key, Value>` is a source code level compatible alternative of `std::unordered_map<Key, Value>`. Just run `sed` commands:

1.  replace include header file<br/>
<code>sed -i 's:&lt;unordered\_map&gt;:&lt;nark/easy\_use\_hash\_map.hpp&gt;:g' somefile.cpp</code>

2.  replace types<br/>
<code>sed -i 's/std::unordered\_map/nark::easy\_use\_hash\_map/g'  somefile.cpp</code>

template parameter HashFunc, KeyEqual, Alloc is absent for `easy_use_hash_map`, if you need these params, you are not `easy-use`.

## More ...
