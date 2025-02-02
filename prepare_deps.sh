#!/bin/sh

# Check for ISPC_EXECUTABLE from environment variable
if [ -n "$ISPC_EXECUTABLE" ]; then
    ISPC_PATH="$ISPC_EXECUTABLE"
elif command -v ispc >/dev/null 2>&1; then
    # Check if ISPC is in the PATH
    ISPC_PATH=$(command -v ispc)
else
    # Default path if ISPC is not in PATH
    ISPC_PATH="$HOME/ispc/bin/ispc"
fi

# Print ISPC Path for verification
echo "Using ISPC executable at: $ISPC_PATH"

# Run CMake commands
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DTBB_TEST=Off -DBUILD_SHARED_LIBS=Off -DCMAKE_CXX_FLAGS=-D__TBB_DYNAMIC_LOAD_ENABLED=0 -DCMAKE_INSTALL_PREFIX=build/install -S tbb -B build/tbb
cmake --build build/tbb --target install
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DTBB_ROOT=build/install -DISPC_EXECUTABLE="$ISPC_PATH" -DOIDN_STATIC_LIB=On -DOIDN_FILTER_RT=Off -DOIDN_APPS=Off -DCMAKE_INSTALL_PREFIX=build/install -S oidn -B build/oidn
cmake --build build/oidn --target install