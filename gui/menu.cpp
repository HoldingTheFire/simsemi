/*
    SimWindows - 1D Semiconductor Device Simulator
    Copyright (C) 2013 David W. Winston

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "comincl.h"

// formulc.h defines `#define UCHAR unsigned char` which conflicts with the
// Windows SDK typedef of the same name. Undefine it before including any
// Windows headers (portable-file-dialogs.h pulls in <windows.h>).
#undef UCHAR

#include "imgui.h"
#include "app.h"
#include "portable-file-dialogs.h"
#include <SDL.h>
#include <filesystem>

extern TEnvironment      environment;
extern TErrorHandler     error_handler;
extern TPreferences      preferences;
extern char              executable_path[];

void SimWindowsApp::render_menu_bar()
{
    if (!ImGui::BeginMainMenuBar())
        return;

    bool has_device = device_loaded && !solving;
    bool is_solving = solving;

    // --- File ---
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open Device...", "Ctrl+O")) {
            auto sel = pfd::open_file("Open Device File", ".",
                {"Device Files", "*.dev *.sta",
                 "All Files", "*"}).result();
            if (!sel.empty()) {
                load_device(sel[0].c_str());
            }
        }

        if (ImGui::MenuItem("Save State...", "Ctrl+S", false, has_device)) {
            auto dest = pfd::save_file("Save State File", "output.sta",
                {"State Files", "*.sta",
                 "All Files", "*"}).result();
            if (!dest.empty()) {
                save_state(dest.c_str());
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Quit", "Alt+F4")) {
            SDL_Event quit_event;
            quit_event.type = SDL_QUIT;
            SDL_PushEvent(&quit_event);
        }

        ImGui::EndMenu();
    }

    // --- Environment ---
    if (ImGui::BeginMenu("Environment")) {
        if (ImGui::MenuItem("Load Material Parameters...")) {
            auto sel = pfd::open_file("Open Material Parameters", ".",
                {"Parameter Files", "*.prm *.PRM",
                 "All Files", "*"}).result();
            if (!sel.empty()) {
                load_material(sel[0].c_str());
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Optical Input...", nullptr, false, has_device)) {
            show_optical_input = true;
        }

        if (ImGui::MenuItem("Preferences...", nullptr, false, true)) {
            show_preferences = true;
        }

        ImGui::EndMenu();
    }

    // --- Device ---
    if (ImGui::BeginMenu("Device")) {
        if (ImGui::MenuItem("Device Editor", nullptr, show_device_editor)) {
            show_device_editor = !show_device_editor;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Start Simulation", "F5", false, has_device)) {
            start_simulation();
        }
        if (ImGui::MenuItem("Stop Simulation", "Esc", false, is_solving)) {
            stop_simulation();
        }
        if (ImGui::MenuItem("Reset Device", nullptr, false, has_device)) {
            reset_device();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Voltage Sweep...", nullptr, false, has_device && !sweep_running)) {
            show_voltage_sweep = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Contacts...", nullptr, false, has_device)) {
            show_contacts = true;
        }
        if (ImGui::MenuItem("Surfaces...", nullptr, false, has_device)) {
            show_surfaces = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Electrical Models...", nullptr, false, has_device)) {
            show_electrical_models = true;
        }
        if (ImGui::MenuItem("Thermal Models...", nullptr, false, has_device)) {
            show_thermal_models = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Device Info...", nullptr, false, device_loaded)) {
            show_device_info = true;
        }
        if (ImGui::MenuItem("Laser Info...", nullptr, false, device_loaded)) {
            show_laser_info = true;
        }

        ImGui::EndMenu();
    }

    // --- Plot ---
    if (ImGui::BeginMenu("Plot", device_loaded)) {
        if (ImGui::MenuItem("Band Diagram")) {
            open_band_diagram();
        }

        ImGui::Separator();
        ImGui::TextDisabled("Carriers");

        if (ImGui::MenuItem("Electron Concentration")) {
            open_plot("Electron Concentration", "n (cm-3)",
                      ELECTRON, CONCENTRATION, true);
        }
        if (ImGui::MenuItem("Hole Concentration")) {
            open_plot("Hole Concentration", "p (cm-3)",
                      HOLE, CONCENTRATION, true);
        }
        if (ImGui::MenuItem("Electron Current")) {
            open_plot("Electron Current", "Jn (A/cm2)",
                      ELECTRON, CURRENT);
        }
        if (ImGui::MenuItem("Hole Current")) {
            open_plot("Hole Current", "Jp (A/cm2)",
                      HOLE, CURRENT);
        }
        if (ImGui::MenuItem("Total Current")) {
            open_plot("Total Current", "J (A/cm2)",
                      NODE, TOTAL_CURRENT);
        }

        ImGui::Separator();
        ImGui::TextDisabled("Fields & Potential");

        if (ImGui::MenuItem("Electric Field")) {
            open_plot("Electric Field", "E (V/cm)",
                      GRID_ELECTRICAL, FIELD);
        }
        if (ImGui::MenuItem("Potential")) {
            open_plot("Potential", "V (V)",
                      GRID_ELECTRICAL, POTENTIAL);
        }

        ImGui::Separator();
        ImGui::TextDisabled("Other");

        if (ImGui::MenuItem("Doping (Nd)")) {
            open_plot("Donor Concentration", "Nd (cm-3)",
                      ELECTRON, DOPING_CONC, true);
        }
        if (ImGui::MenuItem("Doping (Na)")) {
            open_plot("Acceptor Concentration", "Na (cm-3)",
                      HOLE, DOPING_CONC, true);
        }
        if (ImGui::MenuItem("Total Charge")) {
            open_plot("Total Charge", "rho (C/cm3)",
                      NODE, TOTAL_CHARGE);
        }
        if (ImGui::MenuItem("SHR Recombination")) {
            open_plot("SHR Recombination", "R_SHR (cm-3 s-1)",
                      NODE, SHR_RECOMB);
        }
        if (ImGui::MenuItem("B-B Recombination")) {
            open_plot("B-B Recombination", "R_BB (cm-3 s-1)",
                      NODE, B_B_RECOMB);
        }
        if (ImGui::MenuItem("Total Recombination")) {
            open_plot("Total Recombination", "R (cm-3 s-1)",
                      NODE, TOTAL_RECOMB);
        }
        if (ImGui::MenuItem("Optical Generation")) {
            open_plot("Optical Generation", "G (cm-3 s-1)",
                      NODE, OPTICAL_GENERATION);
        }
        if (ImGui::MenuItem("Lattice Temperature")) {
            open_plot("Lattice Temperature", "T (K)",
                      GRID_ELECTRICAL, TEMPERATURE);
        }

        ImGui::Separator();
        ImGui::TextDisabled("Optical");

        if (ImGui::MenuItem("External Optical Spectra")) {
            int num_spec = environment.get_number_objects(SPECTRUM);
            if (num_spec > 0) {
                PlotWindow pw;
                pw.title = "External Optical Spectra";
                pw.y_label = "Intensity (W/cm2)";
                pw.y_flag_type = SPECTRUM;
                pw.y_flag = INCIDENT_INPUT_INTENSITY;
                pw.is_spectrum_plot = true;
                open_plots.push_back(pw);
            } else {
                add_log("No optical spectrum defined. Use Environment > Optical Input first.");
            }
        }

        ImGui::EndMenu();
    }

    // --- Data ---
    if (ImGui::BeginMenu("Data", device_loaded)) {
        if (ImGui::MenuItem("Export Operating Point...")) {
            auto dest = pfd::save_file("Export Operating Point", "operating_point.txt",
                {"Text Files", "*.txt", "All Files", "*"}).result();
            if (!dest.empty()) {
                export_operating_point(dest.c_str());
            }
        }
        if (ImGui::MenuItem("Export All Spatial Data...")) {
            auto dest = pfd::save_file("Export All Spatial Data", "spatial_data.csv",
                {"CSV Files", "*.csv", "All Files", "*"}).result();
            if (!dest.empty()) {
                export_all_spatial(dest.c_str());
            }
        }
        if (ImGui::MenuItem("Export Band Data...")) {
            auto dest = pfd::save_file("Export Band Data", "band_data.csv",
                {"CSV Files", "*.csv", "All Files", "*"}).result();
            if (!dest.empty()) {
                export_band_data(dest.c_str());
            }
        }
        ImGui::EndMenu();
    }

    // --- Help ---
    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("About SimWindows...")) {
            show_about = true;
        }
        ImGui::EndMenu();
    }

    // Status indicator on right side of menu bar
    float status_width = 200.0f;
    ImGui::SameLine(ImGui::GetWindowWidth() - status_width);
    if (is_solving) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "SOLVING...");
    } else if (device_loaded) {
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Ready");
    } else if (material_loaded) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.6f, 1.0f), "No device");
    } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Load materials first");
    }

    ImGui::EndMainMenuBar();
}
