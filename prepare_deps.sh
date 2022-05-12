#!/bin/sh
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DTBB_TEST=Off -DBUILD_SHARED_LIBS=Off -DCMAKE_CXX_FLAGS=-D__TBB_DYNAMIC_LOAD_ENABLED=0 -DCMAKE_INSTALL_PREFIX=build/install -S tbb -B build/tbb
cmake --build build/tbb --target install
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DTBB_ROOT=build/install -DISPC_EXECUTABLE=$HOME/ispc/bin/ispc -DOIDN_STATIC_LIB=On -DOIDN_FILTER_RT=Off -DOIDN_APPS=Off -DCMAKE_INSTALL_PREFIX=build/install -S oidn -B build/oidn
cmake --build build/oidn --target install
