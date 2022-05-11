cmake_minimum_required(VERSION 3.14)
project(qlmdenoiser LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)

find_package(OpenImageDenoise)
if(NOT OPENIMAGEDENOISE_FOUND)
    if(NOT DEFINED OPENIMAGEDENOISE_ROOT_DIR)
        set(OPENIMAGEDENOISE_ROOT_DIR "c:/oidn")
    endif()
    set(OPENIMAGEDENOISE_INCLUDE_DIR "${OPENIMAGEDENOISE_ROOT_DIR}/include")
    find_library(OPENIMAGEDENOISE_LIBRARY NAMES OpenImageDenoise PATHS "${OPENIMAGEDENOISE_ROOT_DIR}/lib" NO_DEFAULT_PATH)
    if(NOT OPENIMAGEDENOISE_LIBRARY)
        message(FATAL_ERROR "Could not find OpenImageDenoise. Try setting OPENIMAGEDENOISE_ROOT_DIR.")
    endif()
endif()

qt_add_executable(qlmdenoiser
    main.cpp
)

target_include_directories(qlmdenoiser PRIVATE
    ${OPENIMAGEDENOISE_INCLUDE_DIR}
)

target_link_libraries(qlmdenoiser PUBLIC
    Qt::Core
    Qt::Gui
    ${OPENIMAGEDENOISE_LIBRARY}
)
