cmake_minimum_required(VERSION 3.10)

project(Candle VERSION 11.8)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
option(CANDLE_ENABLE_LEGACY_SCRIPTS "Build the retired QtScript runtime and bundled JS plugins" OFF)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)

if(MSVC)
    add_compile_options(/MP)
endif()

add_subdirectory(src/customwidgets)
add_subdirectory(src/candle)
if(QT_VERSION_MAJOR EQUAL 5)
    add_subdirectory(src/designerplugins)
endif()
if(CANDLE_ENABLE_LEGACY_SCRIPTS)
    add_subdirectory(src/scriptbindings/qtbindings)
endif()
