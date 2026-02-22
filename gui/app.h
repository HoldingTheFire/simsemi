/*
    SimWindows - 1D Semiconductor Device Simulator
    Copyright (C) 2013 David W. Winston

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#ifndef SIMGUI_APP_H
#define SIMGUI_APP_H

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

// Forward declaration - NUMERIC types come from comincl.h
// included in the .cpp files, not here.

struct PlotWindow {
    std::string title;
    std::string y_label;
    FlagType y_flag_type;
    flag y_flag;
    bool open = true;
    bool log_y = false;
    // For multi-line plots (band diagram)
    struct ExtraLine {
        std::string label;
        FlagType flag_type;
        flag flag_value;
    };
    std::vector<ExtraLine> extra_lines;
};

struct SimWindowsApp {
    // State
    bool device_loaded = false;
    std::atomic<bool> solving{false};
    std::string device_file_path;
    std::string material_file_path;
    bool material_loaded = false;

    // Log buffer (WIOFUNC output)
    std::vector<std::string> log_lines;
    std::mutex log_mutex;
    bool log_scroll_to_bottom = false;

    // Device editor
    std::string device_text;
    bool show_device_editor = false;

    // Plot windows
    std::vector<PlotWindow> open_plots;

    // Solver thread
    std::thread solver_thread;

    // Dialog state
    bool show_about = false;
    bool show_contacts = false;
    bool show_surfaces = false;
    bool show_electrical_models = false;
    bool show_thermal_models = false;
    bool show_optical_input = false;
    bool show_preferences = false;
    bool show_device_info = false;

    // Methods
    void load_material(const char* path);
    void load_device(const char* path);
    void start_simulation();
    void stop_simulation();
    void reset_device();
    void save_state(const char* path);

    void add_log(const std::string& msg);

    // Rendering
    void render();
    void render_menu_bar();
    void render_status_panel();
    void render_plots();
    void render_device_editor();
    void render_dialogs();

    // Plot helpers
    void open_band_diagram();
    void open_plot(const char* title, const char* ylabel,
                   FlagType ft, flag fv, bool log_y = false);

    // Cleanup
    void shutdown();
};

// Global app instance
extern SimWindowsApp app;

#endif // SIMGUI_APP_H
