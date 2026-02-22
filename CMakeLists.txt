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

# --- CLI (headless simulation driver) ---
add_executable(simcli
    cli/main.cpp
    cli/wiofunc_cli.cpp
)
target_include_directories(simcli PRIVATE
    ${CMAKE_SOURCE_DIR}/NUMERIC/INCLUDE
    ${CMAKE_SOURCE_DIR}/Formulc
)
target_link_libraries(simcli PRIVATE numeric formulc)

# --- GUI (Dear ImGui + ImPlot + SDL2 + OpenGL) ---
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(simgui
    gui/main_gui.cpp
    gui/app.cpp
    gui/wiofunc_gui.cpp
    gui/menu.cpp
    gui/dialogs.cpp
    gui/plots.cpp
    gui/device_editor.cpp
    gui/vendor/imgui/imgui.cpp
    gui/vendor/imgui/imgui_draw.cpp
    gui/vendor/imgui/imgui_tables.cpp
    gui/vendor/imgui/imgui_widgets.cpp
    gui/vendor/imgui/imgui_demo.cpp
    gui/vendor/imgui_backends/imgui_impl_sdl2.cpp
    gui/vendor/imgui_backends/imgui_impl_opengl3.cpp
    gui/vendor/implot/implot.cpp
    gui/vendor/implot/implot_items.cpp
)
target_include_directories(simgui PRIVATE
    ${CMAKE_SOURCE_DIR}/NUMERIC/INCLUDE
    ${CMAKE_SOURCE_DIR}/Formulc
    ${CMAKE_SOURCE_DIR}/gui
    ${CMAKE_SOURCE_DIR}/gui/vendor/imgui
    ${CMAKE_SOURCE_DIR}/gui/vendor/imgui_backends
    ${CMAKE_SOURCE_DIR}/gui/vendor/implot
    ${CMAKE_SOURCE_DIR}/gui/vendor
)
target_link_libraries(simgui PRIVATE numeric formulc SDL2::SDL2main SDL2::SDL2 OpenGL::GL)

if(NOT MSVC)
    target_compile_options(simgui PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare)
endif()

# On Windows with MinGW, link as a GUI app (no console window) -- use -mconsole for debugging
if(MINGW)
    target_link_options(simgui PRIVATE -mconsole)
endif()

# --- Test harness ---
add_executable(simtest
    test/test_harness.cpp
    cli/wiofunc_cli.cpp
)
target_include_directories(simtest PRIVATE
    ${CMAKE_SOURCE_DIR}/NUMERIC/INCLUDE
    ${CMAKE_SOURCE_DIR}/Formulc
)
target_link_libraries(simtest PRIVATE numeric formulc)

# --- Install rules ---
install(TARGETS simgui simcli RUNTIME DESTINATION bin)
install(FILES PROJECT/MATERIAL.PRM DESTINATION bin RENAME material.prm)
install(DIRECTORY test/ DESTINATION share/simwindows/examples
    FILES_MATCHING PATTERN "*.dev")

# Bundle SDL2 DLL on Windows (MinGW)
if(MINGW)
    # Find SDL2 DLL next to the import library
    get_target_property(_sdl2_imported_loc SDL2::SDL2 IMPORTED_LOCATION)
    if(_sdl2_imported_loc)
        install(FILES "${_sdl2_imported_loc}" DESTINATION bin)
    else()
        # Fallback: look in standard MSYS2 bin dir
        find_file(SDL2_DLL_PATH SDL2.dll
            PATHS "${SDL2_LIBDIR}/../bin" "/ucrt64/bin" "/mingw64/bin"
            NO_DEFAULT_PATH)
        if(SDL2_DLL_PATH)
            install(FILES "${SDL2_DLL_PATH}" DESTINATION bin)
        endif()
    endif()
endif()

# --- CPack packaging ---
set(CPACK_PACKAGE_NAME "SimWindows")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "1D Semiconductor Device Simulator")
set(CPACK_PACKAGE_VENDOR "David W. Winston")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
