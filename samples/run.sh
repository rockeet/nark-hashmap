#!/bin/sh

#g++ -Ofast -DSP_ALIGN=8 -DNDEBUG hash_strmap_bench.cpp $RT_LIB -I/opt/include -Wall -Wextra $CXXFLAGS
set -x
set -e

loop=29000
if [ $# -ge 1 ]; then
	loop=$1
fi

if [ -z "$CXX" ]; then
	CXX=g++
fi
CXXFLAGS="$CXXFLAGS -pg -Wno-strict-aliasing -D_GNU_SOURCE"
CXXFLAGS="$CXXFLAGS -I../../../nark-bone/src"
export LC_ALL=C
export LANG=en_US

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
$CXX -O3 -DSP_ALIGN=0          hash_strmap_bench.cpp $RT_LIB -I../../src -I$BOOST_INC -Wall -Wextra $CXXFLAGS -o bench1d && ./bench1d $loop

echo "########################################################################################"
$CXX -O3 -DSP_ALIGN=0 -DNDEBUG hash_strmap_bench.cpp $RT_LIB -I../../src -I$BOOST_INC -Wall -Wextra $CXXFLAGS -o bench1a && ./bench1a $loop

echo "########################################################################################"
$CXX -O3 -DSP_ALIGN=4          hash_strmap_bench.cpp $RT_LIB -I../../src -I$BOOST_INC -Wall -Wextra $CXXFLAGS -o bench4d && ./bench4d $loop

echo "########################################################################################"
$CXX -O3 -DSP_ALIGN=4 -DNDEBUG hash_strmap_bench.cpp $RT_LIB -I../../src -I$BOOST_INC -Wall -Wextra $CXXFLAGS -o bench4a && ./bench4a $loop

echo "########################################################################################"
$CXX -O3 -DSP_ALIGN=8          hash_strmap_bench.cpp $RT_LIB -I../../src -I$BOOST_INC -Wall -Wextra $CXXFLAGS -o bench8d && ./bench8d $loop

echo "########################################################################################"
$CXX -O3 -DSP_ALIGN=8 -DNDEBUG hash_strmap_bench.cpp $RT_LIB -I../../src -I$BOOST_INC -Wall -Wextra $CXXFLAGS -o bench8a && ./bench8a $loop


