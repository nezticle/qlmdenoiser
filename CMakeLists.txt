cmake_minimum_required(VERSION 3.14)
project(qlmdenoiser LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)

find_library(OPENIMAGEDENOISE_LIBRARY1 NAMES tbb PATHS "deps/lib" NO_DEFAULT_PATH)
find_library(OPENIMAGEDENOISE_LIBRARY2 NAMES common PATHS "deps/lib" NO_DEFAULT_PATH)
find_library(OPENIMAGEDENOISE_LIBRARY3 NAMES dnnl PATHS "deps/lib" NO_DEFAULT_PATH)
find_library(OPENIMAGEDENOISE_LIBRARY4 NAMES OpenImageDenoise PATHS "deps/lib" NO_DEFAULT_PATH)

qt_add_executable(qlmdenoiser
    qlmdenoiser.cpp
    miniz.c
)

target_include_directories(qlmdenoiser PRIVATE
    deps/include
)

target_link_libraries(qlmdenoiser PUBLIC
    Qt::Core
    Qt::Gui
    ${OPENIMAGEDENOISE_LIBRARY4}
    ${OPENIMAGEDENOISE_LIBRARY3}
    ${OPENIMAGEDENOISE_LIBRARY2}
    ${OPENIMAGEDENOISE_LIBRARY1}
)
