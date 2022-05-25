cmake_minimum_required(VERSION 3.14)
project(qlmdenoiser LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "64-bit only")
endif()
if(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
    set(QLM_ARCH "ARM64")
else()
    set(QLM_ARCH "X64")
endif()

qt_add_executable(qlmdenoiser
    qlmdenoiser.cpp
    miniz.c
)

target_include_directories(qlmdenoiser PRIVATE
    build/install/include
)

set(DEP_LIB_LOCATION "build/install/lib")

find_library(OPENIMAGEDENOISE_LIBRARY1 NAMES tbb PATHS ${DEP_LIB_LOCATION} NO_DEFAULT_PATH)
find_library(OPENIMAGEDENOISE_LIBRARY2 NAMES common PATHS ${DEP_LIB_LOCATION} NO_DEFAULT_PATH)
if(NOT(APPLE AND OIDN_ARCH STREQUAL "ARM64"))
    find_library(OPENIMAGEDENOISE_LIBRARY3 NAMES dnnl PATHS ${DEP_LIB_LOCATION} NO_DEFAULT_PATH)
else()
    find_library(FWAccelerate Accelerate)
    target_link_libraries(qlmdenoiser PRIVATE ${FWAccelerate})
endif()
find_library(OPENIMAGEDENOISE_LIBRARY4 NAMES OpenImageDenoise PATHS ${DEP_LIB_LOCATION} NO_DEFAULT_PATH)

target_link_libraries(qlmdenoiser PUBLIC
    Qt::Core
    Qt::Gui
    ${OPENIMAGEDENOISE_LIBRARY4}
    ${OPENIMAGEDENOISE_LIBRARY3}
    ${OPENIMAGEDENOISE_LIBRARY2}
    ${OPENIMAGEDENOISE_LIBRARY1}
)
