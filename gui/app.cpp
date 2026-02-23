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
#include <cmath>
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

void SimWindowsApp::start_voltage_sweep()
{
    if (sweep_running || solving) return;

    sweep_total_steps = (int)std::round((sweep_end - sweep_start) / sweep_step) + 1;
    if (sweep_total_steps <= 0) {
        add_log("Invalid sweep parameters");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(sweep_mutex);
        sweep_iv_data.clear();
    }

    sweep_running = true;
    sweep_current_step = 0;

    sweep_thread = std::thread([this]() {
        for (int i = 0; i < sweep_total_steps && sweep_running; i++) {
            double V = sweep_start + i * (double)sweep_step;
            sweep_current_voltage = V;
            sweep_current_step = i + 1;

            environment.put_value(CONTACT, APPLIED_BIAS, V, sweep_contact);
            environment.solve();

            if (error_handler.fail()) {
                add_log("Sweep failed at V=" + std::to_string(V));
                break;
            }

            double I = environment.get_value(CONTACT, TOTAL_CURRENT, sweep_contact);
            {
                std::lock_guard<std::mutex> lock(sweep_mutex);
                sweep_iv_data.push_back({V, I});
            }

            char buf[128];
            snprintf(buf, sizeof(buf), "V=%.4f  I=%.6e A/cm2", V, I);
            add_log(buf);
        }

        sweep_running = false;
        add_log("Voltage sweep complete.");
    });
    sweep_thread.detach();
}

void SimWindowsApp::stop_voltage_sweep()
{
    if (sweep_running) {
        sweep_running = false;
        environment.set_stop_solution(TRUE);
        add_log("Stopping voltage sweep...");
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
    if (sweep_running) {
        sweep_running = false;
        environment.set_stop_solution(TRUE);
        for (int i = 0; i < 50 && sweep_running; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    if (solving) {
        environment.set_stop_solution(TRUE);
        // Give solver a moment to stop
        for (int i = 0; i < 50 && solving; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
