Grapevine
=========

A multi-language message passing architecture using ZeroMQ and Zeroconf.

To build gtest
==============
##The cmake configuration currently doesn't build gtest for us, so we have to
##do it manually start at top-level directory.

cd gtest

### for g++
./configure CXX=g++ CXXFLAGS="-std=c++11"

### or, for clang++
./configure CXX=clang++ CXXFLAGS="-std=c++11 -stdlib=libc++ -DGTEST_USE_OWN_TR1_TUPLE=1"

make
make all
cd ..

To build and run tests
======================
### start at top-level directory
mkdir build
cd build
cmake -DGV_BUILD=test ..
./test_grapevine
