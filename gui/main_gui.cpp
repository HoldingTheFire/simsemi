/*
    SimWindows - 1D Semiconductor Device Simulator
    Copyright (C) 2013 David W. Winston

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

/*
 * main_gui.cpp -- GUI entry point for SimWindows.
 *
 * Uses Dear ImGui + ImPlot + SDL2 + OpenGL3 for the graphical interface.
 * Defines all NUMERIC globals (same pattern as cli/main.cpp).
 */

#include "comincl.h"
// formulc.h defines `#define UCHAR unsigned char` which conflicts with the
// Windows SDK typedef. Undefine before any Windows/GL headers are pulled in.
#undef UCHAR
#include "simparse.h"
// strtable.h defines global string tables -- must be included in exactly one TU
#include "strtable.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "app.h"

#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <filesystem>

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

// Global objects required by the NUMERIC library (declared extern in SIMSTRCT.H).
TEnvironment      environment;
TMaterialStorage  material_parameters;
TErrorHandler     error_handler;
TPreferences      preferences;
NormalizeConstants normalization;

// Set executable_path global to the directory containing the executable.
static void init_executable_path(const char* argv0)
{
    namespace fs = std::filesystem;
    std::string exe_dir;

    fs::path exe_path(argv0);
    if (exe_path.has_parent_path()) {
        exe_dir = exe_path.parent_path().string();
    } else {
        exe_dir = fs::current_path().string();
    }

    if (!exe_dir.empty() && exe_dir.back() != '/' && exe_dir.back() != '\\')
        exe_dir += '/';

    strncpy(executable_path, exe_dir.c_str(), MAXPATH - 1);
    executable_path[MAXPATH - 1] = '\0';
}

int main(int argc, char* argv[])
{
    // 1. Initialize NUMERIC globals
    rnd_init();
    init_executable_path(argv[0]);

    // 2. Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // OpenGL 3.0+ context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* window = SDL_CreateWindow(
        "SimWindows",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 800,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // VSync

    // 3. Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;

    // Setup backends
    const char* glsl_version = "#version 130";
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // 4. Auto-load materials from exe directory
    {
        std::string default_mat = std::string(executable_path)
                                + preferences.get_material_parameters_file();
        if (std::filesystem::exists(default_mat)) {
            app.load_material(default_mat.c_str());
        } else {
            app.add_log("No material.prm found in exe directory. Use Environment > Load Material Parameters.");
        }
    }

    // 5. If a device file was passed as argument, load it
    if (argc > 1) {
        app.load_device(argv[1]);
    }

    // 6. Main loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window)) {
                running = false;
            }

            // Keyboard shortcuts
            if (event.type == SDL_KEYDOWN && !io.WantCaptureKeyboard) {
                SDL_Keymod mod = SDL_GetModState();
                switch (event.key.keysym.sym) {
                    case SDLK_F5:
                        app.start_simulation();
                        break;
                    case SDLK_ESCAPE:
                        if (app.solving)
                            app.stop_simulation();
                        break;
                    case SDLK_o:
                        if (mod & KMOD_CTRL) {
                            // Ctrl+O handled by menu via pfd
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        // Start new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Render app
        app.render();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        SDL_GL_GetDrawableSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    // 7. Cleanup
    app.shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
