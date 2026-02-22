/*
    SimWindows - 1D Semiconductor Device Simulator
    Copyright (C) 2013 David W. Winston

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "comincl.h"
#undef UCHAR

#include "imgui.h"
#include "implot.h"
#include "app.h"
#include "portable-file-dialogs.h"
#include <vector>
#include <fstream>

extern TEnvironment environment;

void SimWindowsApp::render_plots()
{
    // Remove closed plot windows
    open_plots.erase(
        std::remove_if(open_plots.begin(), open_plots.end(),
                        [](const PlotWindow& pw) { return !pw.open; }),
        open_plots.end());

    if (!device_loaded) return;

    int num_nodes = environment.get_number_objects(NODE);
    if (num_nodes <= 0) return;

    // Build x-axis data (position in microns)
    std::vector<double> x(num_nodes);
    for (int i = 0; i < num_nodes; i++) {
        x[i] = environment.get_value(GRID_ELECTRICAL, POSITION, i) * 1e4; // cm -> um
    }

    for (auto& pw : open_plots) {
        if (!pw.open) continue;

        // Use unique window ID to allow multiple plots of same type
        std::string win_id = pw.title + "##" + std::to_string((uintptr_t)&pw);
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin(win_id.c_str(), &pw.open)) {
            ImGui::End();
            continue;
        }

        // Get primary data
        std::vector<double> y(num_nodes);
        for (int i = 0; i < num_nodes; i++) {
            y[i] = environment.get_value(pw.y_flag_type, pw.y_flag, i);
        }

        // Export CSV button
        if (ImGui::Button("Export CSV")) {
            std::string default_name = pw.title + ".csv";
            // Replace spaces with underscores for filename
            for (auto& c : default_name) {
                if (c == ' ') c = '_';
            }
            auto dest = pfd::save_file("Export Plot Data", default_name,
                {"CSV Files", "*.csv",
                 "All Files", "*"}).result();
            if (!dest.empty()) {
                std::ofstream ofs(dest);
                if (ofs.good()) {
                    // Header
                    ofs << "Position_um";
                    if (pw.extra_lines.empty()) {
                        ofs << "," << pw.title;
                    } else {
                        ofs << ",Ec";
                        for (auto& el : pw.extra_lines)
                            ofs << "," << el.label;
                    }
                    ofs << "\n";

                    // Data
                    for (int i = 0; i < num_nodes; i++) {
                        ofs << x[i];
                        ofs << "," << y[i];
                        for (auto& el : pw.extra_lines) {
                            double val = environment.get_value(el.flag_type, el.flag_value, i);
                            ofs << "," << val;
                        }
                        ofs << "\n";
                    }

                    add_log("Exported: " + dest);
                } else {
                    add_log("ERROR: Could not write " + dest);
                }
            }
        }

        if (ImPlot::BeginPlot(pw.title.c_str(), ImVec2(-1, -1))) {
            ImPlot::SetupAxes("Position (um)", pw.y_label.c_str());

            if (pw.log_y)
                ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);

            // Primary line
            if (pw.extra_lines.empty()) {
                ImPlot::PlotLine(pw.title.c_str(), x.data(), y.data(), num_nodes);
            } else {
                // Multi-line plot (e.g., band diagram)
                ImPlot::PlotLine("Ec", x.data(), y.data(), num_nodes);

                for (auto& el : pw.extra_lines) {
                    std::vector<double> ey(num_nodes);
                    for (int i = 0; i < num_nodes; i++) {
                        ey[i] = environment.get_value(el.flag_type, el.flag_value, i);
                    }
                    ImPlot::PlotLine(el.label.c_str(), x.data(), ey.data(), num_nodes);
                }
            }

            ImPlot::EndPlot();
        }

        ImGui::End();
    }
}
