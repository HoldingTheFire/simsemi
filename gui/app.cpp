/*
    SimWindows - 1D Semiconductor Device Simulator
    Copyright (C) 2013 David W. Winston

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "comincl.h"
#include "simparse.h"
#include "imgui.h"
#include "implot.h"
#include "app.h"
#include <cstdio>
#include <filesystem>

// NUMERIC globals (declared extern in SIMSTRCT.H)
extern TEnvironment      environment;
extern TMaterialStorage  material_parameters;
extern TErrorHandler     error_handler;
extern TPreferences      preferences;
extern NormalizeConstants normalization;
extern char              executable_path[];

SimWindowsApp app;

void SimWindowsApp::add_log(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(log_mutex);
    log_lines.push_back(msg);
    log_scroll_to_bottom = true;
}

void SimWindowsApp::load_material(const char* path)
{
    add_log(std::string("Loading materials: ") + path);
    TParseMaterial parser(path);
    parser.parse_material();

    if (error_handler.fail()) {
        add_log("ERROR: " + error_handler.get_error_string());
        material_loaded = false;
    } else {
        material_file_path = path;
        material_loaded = true;
        add_log("Materials loaded successfully.");
    }
}

void SimWindowsApp::load_device(const char* path)
{
    if (!material_loaded) {
        add_log("ERROR: Load material parameters first.");
        return;
    }

    add_log(std::string("Loading device: ") + path);
    environment.load_file(path);

    if (error_handler.fail()) {
        add_log("ERROR: " + error_handler.get_error_string());
        device_loaded = false;
    } else {
        device_file_path = path;
        device_loaded = true;
        add_log("Device loaded successfully.");

        // Load device text into editor
        std::ifstream ifs(path);
        if (ifs.good()) {
            device_text.assign(std::istreambuf_iterator<char>(ifs),
                               std::istreambuf_iterator<char>());
        }
    }
}

void SimWindowsApp::start_simulation()
{
    if (!device_loaded || solving) return;

    solving = true;
    environment.set_stop_solution(FALSE);
    add_log("Solving...");

    solver_thread = std::thread([this]() {
        environment.solve();

        if (error_handler.fail()) {
            add_log("ERROR: " + error_handler.get_error_string());
        } else {
            add_log("Simulation complete.");
        }
        solving = false;
    });
    solver_thread.detach();
}

void SimWindowsApp::stop_simulation()
{
    if (solving) {
        environment.set_stop_solution(TRUE);
        add_log("Stopping simulation...");
    }
}

void SimWindowsApp::reset_device()
{
    if (solving) return;
    if (!device_loaded) return;

    environment.init_device();
    add_log("Device reset.");
}

void SimWindowsApp::save_state(const char* path)
{
    if (!device_loaded) return;

    add_log(std::string("Writing state: ") + path);
    environment.write_state_file(path);

    if (error_handler.fail()) {
        add_log("ERROR: " + error_handler.get_error_string());
    } else {
        add_log("State saved.");
    }
}

void SimWindowsApp::open_band_diagram()
{
    PlotWindow pw;
    pw.title = "Band Diagram";
    pw.y_label = "Energy (eV)";
    pw.y_flag_type = ELECTRON;
    pw.y_flag = BAND_EDGE;
    pw.extra_lines.push_back({"Ev", HOLE, BAND_EDGE});
    pw.extra_lines.push_back({"Efn", ELECTRON, QUASI_FERMI});
    pw.extra_lines.push_back({"Efp", HOLE, QUASI_FERMI});
    open_plots.push_back(pw);
}

void SimWindowsApp::open_plot(const char* title, const char* ylabel,
                              FlagType ft, flag fv, bool log_y)
{
    PlotWindow pw;
    pw.title = title;
    pw.y_label = ylabel;
    pw.y_flag_type = ft;
    pw.y_flag = fv;
    pw.log_y = log_y;
    open_plots.push_back(pw);
}

void SimWindowsApp::render_status_panel()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float panel_height = 200.0f;

    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x,
               viewport->WorkPos.y + viewport->WorkSize.y - panel_height));
    ImGui::SetNextWindowSize(
        ImVec2(viewport->WorkSize.x, panel_height));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
                           | ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Status", nullptr, flags)) {
        // Show log lines
        ImGui::BeginChild("LogScroll", ImVec2(0, 0), ImGuiChildFlags_None,
                           ImGuiWindowFlags_HorizontalScrollbar);
        {
            std::lock_guard<std::mutex> lock(log_mutex);
            for (auto& line : log_lines) {
                // Color errors red
                if (line.find("ERROR") != std::string::npos) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                    ImGui::TextUnformatted(line.c_str());
                    ImGui::PopStyleColor();
                } else {
                    ImGui::TextUnformatted(line.c_str());
                }
            }
        }

        if (log_scroll_to_bottom) {
            ImGui::SetScrollHereY(1.0f);
            log_scroll_to_bottom = false;
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void SimWindowsApp::render()
{
    render_menu_bar();
    render_status_panel();
    render_plots();
    if (show_device_editor)
        render_device_editor();
    render_dialogs();
}

void SimWindowsApp::shutdown()
{
    if (solving) {
        environment.set_stop_solution(TRUE);
        // Give solver a moment to stop
        for (int i = 0; i < 50 && solving; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
