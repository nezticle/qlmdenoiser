cd tbb
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DTBB_TEST=Off -DBUILD_SHARED_LIBS=Off -DCMAKE_CXX_FLAGS=-D__TBB_DYNAMIC_LOAD_ENABLED=0 -DCMAKE_INSTALL_PREFIX=../deps .
ninja install
cd ..
cd oidn
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DTBB_ROOT=../deps -DISPC_EXECUTABLE=c:\ispc\bin\ispc.exe -DOIDN_STATIC_LIB=On -DOIDN_FILTER_RT=Off -DOIDN_APPS=Off -DCMAKE_INSTALL_PREFIX=../deps .
ninja install
cd ..