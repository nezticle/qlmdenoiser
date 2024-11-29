@echo off

:: Check for ISPC_EXECUTABLE from environment variable
if defined ISPC_EXECUTABLE (
    set ISPC_PATH=%ISPC_EXECUTABLE%
) else (
    :: Check if ISPC is in the PATH
    where ispc >nul 2>nul
    if %ERRORLEVEL%==0 (
        for /f "usebackq tokens=*" %%i in (`where ispc`) do set ISPC_PATH=%%i
    ) else (
        :: Default path if ISPC is not in PATH
        set ISPC_PATH=c:\ispc\bin\ispc.exe
    )
)

:: Print ISPC Path for verification
echo Using ISPC executable at: %ISPC_PATH%

cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DTBB_TEST=Off -DBUILD_SHARED_LIBS=Off -DCMAKE_CXX_FLAGS=-D__TBB_DYNAMIC_LOAD_ENABLED=0 -DCMAKE_INSTALL_PREFIX=build/install -S tbb -B build/tbb
cmake --build build/tbb --target install
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DTBB_ROOT=build/install -DISPC_EXECUTABLE=%ISPC_PATH% -DOIDN_STATIC_LIB=On -DOIDN_FILTER_RT=Off -DOIDN_APPS=Off -DCMAKE_INSTALL_PREFIX=build/install -S oidn -B build/oidn
cmake --build build/oidn --target install