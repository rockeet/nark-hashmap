#!/bin/sh

#g++ -Ofast -DSP_ALIGN=8 -DNDEBUG gold_hash_map_bench.cpp $RT_LIB -I/opt/include -Wall $CXXFLAGS -Wextra
set -x
set -e

CXXFLAGS="$CXXFLAGS -D_GNU_SOURCE"
CXXFLAGS="$CXXFLAGS -I../../../nark-bone/src"

loop=29000
if [ $# -ge 1 ]; then
	loop=$1
fi

if [ -z "$CXX" ]; then
	CXX=g++
fi

#CXX="$CXX -fmax-errors=10"

if [ -z "$BOOST_INC" ]; then
	for dir in / /usr/ /usr/local/ /opt/ $HOME/
	do
		if [ -f ${dir}include/boost/version.hpp ]; then
			BOOST_INC=${dir}include
			break
		fi
	done
elif [ ! -f $BOOST_INC/boost/version.hpp ]; then
	echo boost is missing
	exit 1
fi

if [ "`uname`" = Darwin ]; then
	RT_LIB=""
else
	RT_LIB=-lrt
fi

echo "########################################################################################"
$CXX -O0 -g3 -DSP_ALIGN=0          gold_hash_map_bench.cpp $RT_LIB -I$BOOST_INC -I../../src -Wall $CXXFLAGS -Wextra -g3 -o bench-d
if [ $? -ne 0 ]; then
	exit
fi
./bench-d $loop

echo "########################################################################################"
$CXX -O3 -g3 -DSP_ALIGN=0 -DNDEBUG gold_hash_map_bench.cpp $RT_LIB -I$BOOST_INC -I../../src -Wall $CXXFLAGS -Wextra -o bench-r && ./bench-r $loop

