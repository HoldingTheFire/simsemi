cmake_minimum_required(VERSION 3.16)
project(SimWindows
    VERSION 1.0.0
    DESCRIPTION "SimWindows - 1D Semiconductor Device Simulator"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- Formulc (C library: formula parser) ---
add_library(formulc STATIC
    Formulc/formulc.c
)
target_include_directories(formulc PUBLIC
    ${CMAKE_SOURCE_DIR}/Formulc
)

# --- NUMERIC (C++ library: physics simulation core) ---
set(NUMERIC_SOURCES
    NUMERIC/2dcclass.cpp
    NUMERIC/bdcclass.cpp
    NUMERIC/carclass.cpp
    NUMERIC/cavclass.cpp
    NUMERIC/conclass.cpp
    NUMERIC/devclass.cpp
    NUMERIC/elcclass.cpp
    NUMERIC/eleclass.cpp
    NUMERIC/envclass.cpp
    NUMERIC/frcclass.cpp
    NUMERIC/funclass.cpp
    NUMERIC/globfunc.cpp
    NUMERIC/grdclass.cpp
    NUMERIC/infclass.cpp
    NUMERIC/matclass.cpp
    NUMERIC/mirclass.cpp
    NUMERIC/modclass.cpp
    NUMERIC/nodclass.cpp
    NUMERIC/parclass.cpp
    NUMERIC/qwclass.cpp
    NUMERIC/solclass.cpp
    NUMERIC/supclass.cpp
    NUMERIC/surclass.cpp
    NUMERIC/thrclass.cpp
)

add_library(numeric STATIC ${NUMERIC_SOURCES})
target_include_directories(numeric PUBLIC
    ${CMAKE_SOURCE_DIR}/NUMERIC/INCLUDE
    ${CMAKE_SOURCE_DIR}/Formulc
)
target_link_libraries(numeric PUBLIC formulc)

# Enable warnings
if(MSVC)
    target_compile_options(numeric PRIVATE /W3)
    target_compile_options(formulc PRIVATE /W3)
else()
    target_compile_options(numeric PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare)
    target_compile_options(formulc PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare)
endif()

# Platform-specific filesystem linkage (needed on older GCC)
if(NOT MSVC)
    target_link_libraries(numeric PUBLIC stdc++fs)
endif()
