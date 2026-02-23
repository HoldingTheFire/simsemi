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
#include <cmath>
#include <algorithm>
#include <string>

extern TEnvironment environment;

// Apply the selected y-axis operation to a data vector in-place.
static void apply_y_operation(std::vector<double>& y, int op)
{
    if (op == 1) {
        for (auto& v : y) v = -v;
    } else if (op == 2) {
        for (auto& v : y) v = std::fabs(v);
    }
}

// Render the Scale dialog for a single PlotWindow as an ImGui popup.
// The popup must be opened with ImGui::OpenPopup() from the same ID stack context.
static void render_scale_dialog(PlotWindow& pw)
{
    // Use a unique popup ID derived from the plot window address
    char popup_id[64];
    snprintf(popup_id, sizeof(popup_id), "ScaleSettings##%p", (void*)&pw);

    if (pw.show_scale_dialog) {
        ImGui::OpenPopup(popup_id);
        pw.show_scale_dialog = false;
    }

    ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_Always);
    if (ImGui::BeginPopupModal(popup_id, nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::TextDisabled("Axis Scale for: %s", pw.title.c_str());
        ImGui::Separator();

        // --- Log Y toggle ---
        ImGui::Checkbox("Log Y-axis", &pw.log_y);

        ImGui::Separator();

        // --- Y operation ---
        ImGui::TextUnformatted("Y-axis transform:");
        ImGui::RadioButton("None",         &pw.y_operation, 0); ImGui::SameLine();
        ImGui::RadioButton("Negate (-y)",  &pw.y_operation, 1); ImGui::SameLine();
        ImGui::RadioButton("Abs |y|",      &pw.y_operation, 2);

        ImGui::Separator();

        // --- Custom axis limits ---
        ImGui::Checkbox("Use custom axis limits", &pw.use_custom_scale);
        ImGui::BeginDisabled(!pw.use_custom_scale);

        ImGui::TextUnformatted("X axis:");
        ImGui::SetNextItemWidth(120);
        ImGui::InputDouble("X min##sc", &pw.edit_x_min, 0.0, 0.0, "%.4g");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        ImGui::InputDouble("X max##sc", &pw.edit_x_max, 0.0, 0.0, "%.4g");

        ImGui::TextUnformatted("Y axis:");
        ImGui::SetNextItemWidth(120);
        ImGui::InputDouble("Y min##sc", &pw.edit_y_min, 0.0, 0.0, "%.4g");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        ImGui::InputDouble("Y max##sc", &pw.edit_y_max, 0.0, 0.0, "%.4g");

        ImGui::EndDisabled();

        ImGui::Separator();

        // --- Buttons ---
        if (ImGui::Button("Apply", ImVec2(90, 0))) {
            if (pw.use_custom_scale) {
                pw.x_min = pw.edit_x_min;
                pw.x_max = pw.edit_x_max;
                pw.y_min = pw.edit_y_min;
                pw.y_max = pw.edit_y_max;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Auto-fit", ImVec2(90, 0))) {
            pw.use_custom_scale = false;
            pw.auto_fit_next_frame = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(90, 0))) {
            // Revert edit buffers
            pw.edit_x_min = pw.x_min;
            pw.edit_x_max = pw.x_max;
            pw.edit_y_min = pw.y_min;
            pw.edit_y_max = pw.y_max;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

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
        ImGui::SetNextWindowSize(ImVec2(600, 420), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin(win_id.c_str(), &pw.open)) {
            ImGui::End();
            continue;
        }

        // --- Special handling for spectrum plots ---
        if (pw.is_spectrum_plot) {
            int num_spec = environment.get_number_objects(SPECTRUM);
            if (num_spec <= 0) {
                ImGui::Text("No spectrum data available.");
                ImGui::End();
                continue;
            }

            std::vector<double> energies(num_spec), intensities(num_spec);
            for (int i = 0; i < num_spec; i++) {
                energies[i] = environment.get_value(SPECTRUM, INCIDENT_PHOTON_ENERGY, i);
                intensities[i] = environment.get_value(SPECTRUM, INCIDENT_INPUT_INTENSITY, i);
            }

            // Toolbar
            if (ImGui::Button("Export CSV##spec")) {
                auto dest = pfd::save_file("Export Spectrum", "spectrum.csv",
                    {"CSV Files", "*.csv", "All Files", "*"}).result();
                if (!dest.empty()) {
                    std::ofstream ofs(dest);
                    if (ofs.good()) {
                        ofs << "PhotonEnergy_eV,Intensity_W_cm2\n";
                        for (int i = 0; i < num_spec; i++)
                            ofs << energies[i] << "," << intensities[i] << "\n";
                        add_log("Exported spectrum: " + dest);
                    }
                }
            }

            if (ImPlot::BeginPlot(pw.title.c_str(), ImVec2(-1, -1))) {
                ImPlot::SetupAxes("Photon Energy (eV)", "Intensity (W/cm2)");
                if (num_spec == 1) {
                    // Single point -- draw as scatter
                    ImPlot::PlotScatter("Spectrum", energies.data(), intensities.data(), num_spec);
                } else {
                    ImPlot::PlotLine("Spectrum", energies.data(), intensities.data(), num_spec);
                    ImPlot::PlotScatter("##dots", energies.data(), intensities.data(), num_spec);
                }
                ImPlot::EndPlot();
            }

            ImGui::End();
            continue;  // Skip the normal plot rendering
        }

        // Get primary data
        std::vector<double> y(num_nodes);
        for (int i = 0; i < num_nodes; i++) {
            y[i] = environment.get_value(pw.y_flag_type, pw.y_flag, i);
        }
        apply_y_operation(y, pw.y_operation);

        // Helper: get extra line value at node i
        auto get_extra_value = [&](const PlotWindow::ExtraLine& el, int i) -> double {
            double val;
            if (el.flag_value == QUASI_FERMI) {
                double qf = environment.get_value(el.flag_type, QUASI_FERMI, i);
                if (el.flag_type == ELECTRON) {
                    double ec = environment.get_value(ELECTRON, BAND_EDGE, i);
                    val = ec - qf;
                } else if (el.flag_type == HOLE) {
                    double ev = environment.get_value(HOLE, BAND_EDGE, i);
                    val = ev + qf;
                } else {
                    val = qf;
                }
            } else {
                val = environment.get_value(el.flag_type, el.flag_value, i);
            }
            // Extra lines share the same y-operation
            if (pw.y_operation == 1) val = -val;
            else if (pw.y_operation == 2) val = std::fabs(val);
            return val;
        };

        // ----------------------------------------------------------------
        // Toolbar: Export CSV | Scale... | Auto-fit | Trace toggle
        // ----------------------------------------------------------------
        if (ImGui::Button("Export CSV")) {
            std::string default_name = pw.title + ".csv";
            for (auto& c : default_name)
                if (c == ' ') c = '_';
            auto dest = pfd::save_file("Export Plot Data", default_name,
                {"CSV Files", "*.csv", "All Files", "*"}).result();
            if (!dest.empty()) {
                std::ofstream ofs(dest);
                if (ofs.good()) {
                    ofs << "Position_um";
                    if (pw.extra_lines.empty()) {
                        ofs << "," << pw.title;
                    } else {
                        ofs << ",Ec";
                        for (auto& el : pw.extra_lines)
                            ofs << "," << el.label;
                    }
                    ofs << "\n";
                    for (int i = 0; i < num_nodes; i++) {
                        ofs << x[i] << "," << y[i];
                        for (auto& el : pw.extra_lines)
                            ofs << "," << get_extra_value(el, i);
                        ofs << "\n";
                    }
                    add_log("Exported: " + dest);
                } else {
                    add_log("ERROR: Could not write " + dest);
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Scale...")) {
            // Seed edit buffers with current committed values
            pw.edit_x_min = pw.x_min;
            pw.edit_x_max = pw.x_max;
            pw.edit_y_min = pw.y_min;
            pw.edit_y_max = pw.y_max;
            pw.show_scale_dialog = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Set axis limits, log scale, and y-axis transform");

        ImGui::SameLine();
        if (ImGui::Button("Auto-fit")) {
            pw.auto_fit_next_frame = true;
            pw.use_custom_scale = false;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Reset axes to auto-fit all data");

        ImGui::SameLine();
        // Trace toggle button — highlight when active
        if (pw.show_trace)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button(pw.show_trace ? "Trace ON" : "Trace")) {
            pw.show_trace = !pw.show_trace;
        }
        if (pw.show_trace)
            ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Toggle mouse-position trace overlay");

        ImGui::SameLine();
        if (!pw.frozen) {
            if (ImGui::Button("Freeze")) {
                pw.frozen = true;
                pw.frozen_x = x;
                pw.frozen_y = y;
                pw.frozen_extra_y.clear();
                for (auto& el : pw.extra_lines) {
                    std::vector<double> ey(num_nodes);
                    for (int i = 0; i < num_nodes; i++)
                        ey[i] = get_extra_value(el, i);
                    pw.frozen_extra_y.push_back(ey);
                }
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Freeze current data for comparison overlay");
        } else {
            if (ImGui::Button("Melt")) {
                pw.frozen = false;
                pw.frozen_x.clear();
                pw.frozen_y.clear();
                pw.frozen_extra_y.clear();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Remove frozen overlay and resume live-only display");
        }

        // Show current y-operation label if non-default
        if (pw.y_operation != 0) {
            ImGui::SameLine();
            const char* op_labels[] = { "", "[-y]", "[|y|]" };
            ImGui::TextDisabled("%s", op_labels[pw.y_operation]);
        }

        // ----------------------------------------------------------------
        // Render the Scale dialog popup (must be called in same window ctx)
        // ----------------------------------------------------------------
        render_scale_dialog(pw);

        // ----------------------------------------------------------------
        // ImPlot
        // ----------------------------------------------------------------

        // Auto-fit: call before BeginPlot
        if (pw.auto_fit_next_frame) {
            ImPlot::SetNextAxesToFit();
            pw.auto_fit_next_frame = false;
        }

        // Custom axis limits
        if (pw.use_custom_scale) {
            ImPlot::SetNextAxisLimits(ImAxis_X1, pw.x_min, pw.x_max, ImGuiCond_Always);
            ImPlot::SetNextAxisLimits(ImAxis_Y1, pw.y_min, pw.y_max, ImGuiCond_Always);
        }

        // Reserve space for the trace readout line below the plot
        float trace_height = pw.show_trace ? ImGui::GetFrameHeightWithSpacing() : 0.0f;
        ImVec2 plot_size(-1.0f, -trace_height - ImGui::GetStyle().ItemSpacing.y);

        bool trace_hovered = false;
        ImPlotPoint trace_mouse = {0, 0};
        double trace_nearest_y = 0;

        if (ImPlot::BeginPlot(pw.title.c_str(), plot_size)) {
            ImPlot::SetupAxes("Position (um)", pw.y_label.c_str());

            if (pw.log_y)
                ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);

            // Primary line
            if (pw.extra_lines.empty()) {
                ImPlot::PlotLine(pw.title.c_str(), x.data(), y.data(), num_nodes);
            } else {
                // Multi-line plot (e.g. band diagram)
                ImPlot::PlotLine("Ec", x.data(), y.data(), num_nodes);
                for (auto& el : pw.extra_lines) {
                    std::vector<double> ey(num_nodes);
                    for (int i = 0; i < num_nodes; i++)
                        ey[i] = get_extra_value(el, i);
                    ImPlot::PlotLine(el.label.c_str(), x.data(), ey.data(), num_nodes);
                }
            }

            // ---- Frozen data overlay ----
            if (pw.frozen && !pw.frozen_x.empty()) {
                ImPlot::SetNextLineStyle(ImVec4(0.7f, 0.7f, 0.7f, 0.8f), 1.5f);
                if (pw.extra_lines.empty()) {
                    ImPlot::PlotLine("Frozen", pw.frozen_x.data(), pw.frozen_y.data(), (int)pw.frozen_x.size());
                } else {
                    ImPlot::PlotLine("Ec (frozen)", pw.frozen_x.data(), pw.frozen_y.data(), (int)pw.frozen_x.size());
                    for (size_t ei = 0; ei < pw.extra_lines.size() && ei < pw.frozen_extra_y.size(); ei++) {
                        std::string label = pw.extra_lines[ei].label + " (frozen)";
                        ImPlot::SetNextLineStyle(ImVec4(0.7f, 0.7f, 0.7f, 0.8f), 1.5f);
                        ImPlot::PlotLine(label.c_str(), pw.frozen_x.data(), pw.frozen_extra_y[ei].data(), (int)pw.frozen_x.size());
                    }
                }
            }

            // ---- Trace overlay ----
            trace_hovered = pw.show_trace && ImPlot::IsPlotHovered();
            if (trace_hovered) {
                ImPlotPoint mouse = ImPlot::GetPlotMousePos();
                trace_mouse = mouse;

                // Find nearest node for exact y-value interpolation on primary curve
                double nearest_y = mouse.y;
                if (num_nodes > 0) {
                    // x[] is sorted ascending; binary-search for closest
                    double target_x = mouse.x;
                    int lo = 0, hi = num_nodes - 1;
                    while (lo < hi) {
                        int mid = (lo + hi) / 2;
                        if (x[mid] < target_x) lo = mid + 1;
                        else hi = mid;
                    }
                    // lo is now the first index >= target_x
                    int best = lo;
                    if (lo > 0 && std::fabs(x[lo-1] - target_x) < std::fabs(x[lo] - target_x))
                        best = lo - 1;
                    best = std::max(0, std::min(best, num_nodes - 1));
                    nearest_y = y[best];
                }

                // Draw crosshair lines
                ImPlot::SetNextLineStyle(ImVec4(1.0f, 1.0f, 0.2f, 0.6f), 1.0f);
                double vline_x[2] = { mouse.x, mouse.x };
                ImPlotRect lims = ImPlot::GetPlotLimits();
                double vline_y[2] = { lims.Y.Min, lims.Y.Max };
                ImPlot::PlotLine("##vline", vline_x, vline_y, 2);

                double hline_y[2] = { nearest_y, nearest_y };
                double hline_x[2] = { lims.X.Min, lims.X.Max };
                ImPlot::PlotLine("##hline", hline_x, hline_y, 2);

                // Draw labelled dot at (mouse.x, nearest_y)
                ImPlot::PlotScatter("##dot", &mouse.x, &nearest_y, 1);

                // Annotation near the cursor
                char ann[128];
                snprintf(ann, sizeof(ann), "x=%.4g  y=%.4g", mouse.x, nearest_y);
                ImPlot::Annotation(mouse.x, nearest_y, ImVec4(0,0,0,0.7f), ImVec2(8,8), true, "%s", ann);

                trace_nearest_y = nearest_y;
            }

            ImPlot::EndPlot();
        }

        // ---- Trace status bar below the plot ----
        if (pw.show_trace) {
            if (trace_hovered) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f),
                    "Trace:  x = %.6g um    y = %.6g %s",
                    trace_mouse.x, trace_nearest_y, pw.y_label.c_str());
            } else {
                ImGui::TextDisabled("Trace: move mouse over plot");
            }
        }

        ImGui::End();
    }
}
