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

    // Scale dialog state
    bool show_scale_dialog = false;
    bool use_custom_scale = false;
    double x_min = 0.0;
    double x_max = 1.0;
    double y_min = -1.0;
    double y_max = 1.0;
    // Temporary edit buffers for the scale dialog
    double edit_x_min = 0.0;
    double edit_x_max = 1.0;
    double edit_y_min = -1.0;
    double edit_y_max = 1.0;

    // y-axis operation: 0 = none, 1 = negate (-y), 2 = absolute value (|y|)
    int y_operation = 0;

    // Auto-fit request: set true to trigger SetNextAxesToFit before next BeginPlot
    bool auto_fit_next_frame = false;

    // Trace overlay (show x,y under mouse cursor)
    bool show_trace = false;

    // Spectrum plot (x-axis is photon energy, not position)
    bool is_spectrum_plot = false;

    // Freeze/Melt overlay
    bool frozen = false;
    std::vector<double> frozen_x;
    std::vector<double> frozen_y;
    std::vector<std::vector<double>> frozen_extra_y;  // one per extra_line

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
    bool show_voltage_sweep = false;
    bool show_laser_info = false;

    // Voltage sweep state
    float sweep_start = 0.0f;
    float sweep_end = 1.0f;
    float sweep_step = 0.1f;
    std::atomic<bool> sweep_running{false};
    double sweep_current_voltage = 0.0;       // current voltage being solved (read by UI thread)
    int sweep_current_step = 0;               // current step index (read by UI thread)
    int sweep_total_steps = 0;                // total steps (set before thread launch)
    std::vector<std::pair<double,double>> sweep_iv_data;  // (V, I) results
    std::mutex sweep_mutex;                   // protects sweep_iv_data during append
    int sweep_contact = 0;                    // 0 = left contact, 1 = right contact
    std::thread sweep_thread;

    // Methods
    void load_material(const char* path);
    void load_device(const char* path);
    void start_simulation();
    void stop_simulation();
    void reset_device();
    void save_state(const char* path);
    void start_voltage_sweep();
    void stop_voltage_sweep();

    void add_log(const std::string& msg);

    // Rendering
    void render();
    void render_menu_bar();
    void render_status_panel();
    void render_plots();
    void render_device_editor();
    void render_dialogs();
    void render_voltage_sweep();

    // Plot helpers
    void open_band_diagram();
    void open_plot(const char* title, const char* ylabel,
                   FlagType ft, flag fv, bool log_y = false);

    // Data export
    void export_operating_point(const char* path);
    void export_all_spatial(const char* path);
    void export_band_data(const char* path);

    // Cleanup
    void shutdown();
};

// Global app instance
extern SimWindowsApp app;

#endif // SIMGUI_APP_H
