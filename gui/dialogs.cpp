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
#include <fstream>
#include <cmath>

extern TEnvironment      environment;
extern TErrorHandler     error_handler;
extern TPreferences      preferences;

// Helper: checkbox bound to a flag bit within an effects word
static bool flag_checkbox(const char* label, flag& effects, flag bit)
{
    bool checked = (effects & bit) != 0;
    if (ImGui::Checkbox(label, &checked)) {
        if (checked) effects |= bit;
        else         effects &= ~bit;
        return true;
    }
    return false;
}

void SimWindowsApp::render_dialogs()
{
    // =====================================================================
    // About
    // =====================================================================
    if (show_about) {
        ImGui::OpenPopup("About SimWindows");
        show_about = false;
    }
    if (ImGui::BeginPopupModal("About SimWindows", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("SimWindows");
        ImGui::Text("1D Semiconductor Device Simulator");
        ImGui::Separator();
        ImGui::Text("Originally by David W. Winston (1996)");
        ImGui::Text("Modernized with Dear ImGui + ImPlot");
        ImGui::Text("Physics engine: NUMERIC library");
        ImGui::Separator();
        ImGui::Text("Licensed under GPL v3");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // =====================================================================
    // Contacts (full)
    // =====================================================================
    if (show_contacts) {
        ImGui::OpenPopup("Contacts");
        show_contacts = false;
    }
    if (ImGui::BeginPopupModal("Contacts", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        int num_contacts = environment.get_number_objects(CONTACT);
        bool changed = false;

        for (int c = 0; c < num_contacts; c++) {
            ImGui::PushID(c);

            const char* side = (c == 0) ? "Left" : "Right";
            if (ImGui::CollapsingHeader(side, ImGuiTreeNodeFlags_DefaultOpen)) {

                // --- Contact type (radio buttons) ---
                flag effects = (flag)environment.get_value(CONTACT, EFFECTS, c);
                flag old_effects = effects;

                bool is_ohmic = (effects & CONTACT_IDEALOHMIC) != 0;
                bool is_finite = (effects & CONTACT_FINITERECOMB) != 0;
                bool is_schottky = (effects & CONTACT_SCHOTTKY) != 0;

                if (ImGui::RadioButton("Ideal Ohmic", is_ohmic)) {
                    effects = CONTACT_IDEALOHMIC;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Finite Recomb", is_finite)) {
                    effects = CONTACT_FINITERECOMB;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Schottky", is_schottky)) {
                    effects = CONTACT_SCHOTTKY;
                }

                if (effects != old_effects) {
                    environment.put_value(CONTACT, EFFECTS, (prec)effects, c, c);
                    changed = true;
                }

                // --- Applied bias ---
                float bias = (float)environment.get_value(CONTACT, APPLIED_BIAS, c);
                if (ImGui::InputFloat("Applied Bias (V)", &bias, 0.01f, 0.1f, "%.4f")) {
                    environment.put_value(CONTACT, APPLIED_BIAS, (prec)bias, c, c);
                    changed = true;
                }

                // --- Finite recombination velocities (only when relevant) ---
                if (effects & CONTACT_FINITERECOMB) {
                    float e_vel = (float)environment.get_value(CONTACT, ELECTRON_RECOMB_VEL, c);
                    if (ImGui::InputFloat("Electron Recomb Vel (cm/s)", &e_vel, 0.0f, 0.0f, "%.2e")) {
                        environment.put_value(CONTACT, ELECTRON_RECOMB_VEL, (prec)e_vel, c, c);
                        changed = true;
                    }

                    float h_vel = (float)environment.get_value(CONTACT, HOLE_RECOMB_VEL, c);
                    if (ImGui::InputFloat("Hole Recomb Vel (cm/s)", &h_vel, 0.0f, 0.0f, "%.2e")) {
                        environment.put_value(CONTACT, HOLE_RECOMB_VEL, (prec)h_vel, c, c);
                        changed = true;
                    }
                }

                // --- Schottky barrier height ---
                if (effects & CONTACT_SCHOTTKY) {
                    float barrier = (float)environment.get_value(CONTACT, BARRIER_HEIGHT, c);
                    if (ImGui::InputFloat("Barrier Height (eV)", &barrier, 0.01f, 0.1f, "%.4f")) {
                        environment.put_value(CONTACT, BARRIER_HEIGHT, (prec)barrier, c, c);
                        changed = true;
                    }
                }

                // --- Read-only info ---
                ImGui::Spacing();
                ImGui::TextDisabled("Computed values:");
                float total_j = (float)environment.get_value(CONTACT, TOTAL_CURRENT, c);
                float e_j = (float)environment.get_value(CONTACT, ELECTRON_CURRENT, c);
                float h_j = (float)environment.get_value(CONTACT, HOLE_CURRENT, c);
                float field = (float)environment.get_value(CONTACT, FIELD, c);
                float built_in = (float)environment.get_value(CONTACT, BUILT_IN_POT, c);

                ImGui::Text("  Total Current:    %.4e A/cm2", total_j);
                ImGui::Text("  Electron Current: %.4e A/cm2", e_j);
                ImGui::Text("  Hole Current:     %.4e A/cm2", h_j);
                ImGui::Text("  Electric Field:   %.4e V/cm", field);
                ImGui::Text("  Built-in Pot:     %.4f V", built_in);
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            if (changed)
                environment.process_recompute_flags();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    // =====================================================================
    // Surfaces (full)
    // =====================================================================
    if (show_surfaces) {
        ImGui::OpenPopup("Surfaces");
        show_surfaces = false;
    }
    if (ImGui::BeginPopupModal("Surfaces", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        int num_surfaces = environment.get_number_objects(SURFACE);
        bool changed = false;

        for (int s = 0; s < num_surfaces; s++) {
            ImGui::PushID(s);

            const char* side = (s == 0) ? "Left Surface" : "Right Surface";
            if (ImGui::CollapsingHeader(side, ImGuiTreeNodeFlags_DefaultOpen)) {

                // --- Effect flags ---
                flag effects = (flag)environment.get_value(SURFACE, EFFECTS, s);
                flag old_effects = effects;

                if (flag_checkbox("Heat Sink", effects, SURFACE_HEAT_SINK))
                    changed = true;

                if (effects != old_effects) {
                    environment.put_value(SURFACE, EFFECTS, (prec)effects, s, s);
                }

                // --- Temperatures ---
                float temp = (float)environment.get_value(SURFACE, TEMPERATURE, s);
                if (ImGui::InputFloat("Lattice Temperature (K)", &temp, 1.0f, 10.0f, "%.1f")) {
                    environment.put_value(SURFACE, TEMPERATURE, (prec)temp, s, s);
                    changed = true;
                }

                float e_temp = (float)environment.get_value(SURFACE, ELECTRON_TEMPERATURE, s);
                if (ImGui::InputFloat("Electron Temperature (K)", &e_temp, 1.0f, 10.0f, "%.1f")) {
                    environment.put_value(SURFACE, ELECTRON_TEMPERATURE, (prec)e_temp, s, s);
                    changed = true;
                }

                float h_temp = (float)environment.get_value(SURFACE, HOLE_TEMPERATURE, s);
                if (ImGui::InputFloat("Hole Temperature (K)", &h_temp, 1.0f, 10.0f, "%.1f")) {
                    environment.put_value(SURFACE, HOLE_TEMPERATURE, (prec)h_temp, s, s);
                    changed = true;
                }

                // --- Thermal conductivity ---
                float th_cond = (float)environment.get_value(SURFACE, THERMAL_CONDUCT, s);
                if (ImGui::InputFloat("Thermal Conductivity (W/cm-K)", &th_cond, 0.0f, 0.0f, "%.4e")) {
                    environment.put_value(SURFACE, THERMAL_CONDUCT, (prec)th_cond, s, s);
                    changed = true;
                }

                // --- Refractive indices ---
                float inc_n = (float)environment.get_value(SURFACE, INCIDENT_REFRACTIVE_INDEX, s);
                if (ImGui::InputFloat("Incident Refractive Index", &inc_n, 0.01f, 0.1f, "%.4f")) {
                    environment.put_value(SURFACE, INCIDENT_REFRACTIVE_INDEX, (prec)inc_n, s, s);
                    changed = true;
                }

                float mode_n = (float)environment.get_value(SURFACE, MODE_REFRACTIVE_INDEX, s);
                if (ImGui::InputFloat("Mode Refractive Index", &mode_n, 0.01f, 0.1f, "%.4f")) {
                    environment.put_value(SURFACE, MODE_REFRACTIVE_INDEX, (prec)mode_n, s, s);
                    changed = true;
                }

                // --- Read-only optical info ---
                ImGui::Spacing();
                ImGui::TextDisabled("Optical (read-only):");
                float poynting = (float)environment.get_value(SURFACE, INCIDENT_TOTAL_POYNTING, s);
                ImGui::Text("  Incident Total Poynting: %.4e W/cm2", poynting);
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            if (changed)
                environment.process_recompute_flags();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    // =====================================================================
    // Electrical Models (full) -- regular window, opened/closed via show flag
    // =====================================================================
    if (show_electrical_models) {
        ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Electrical Models", &show_electrical_models,
                          ImGuiWindowFlags_AlwaysAutoResize)) {
            // Grid effects apply to all grid elements
            flag effects = (flag)environment.get_value(GRID_ELECTRICAL, EFFECTS, 0);
            flag old_effects = effects;

            ImGui::Text("Recombination:");
            flag_checkbox("SHR Recombination", effects, GRID_RECOMB_SHR);
            flag_checkbox("Band-to-Band Recombination", effects, GRID_RECOMB_B_B);
            flag_checkbox("Auger Recombination", effects, GRID_RECOMB_AUGER);
            flag_checkbox("Stimulated Recombination", effects, GRID_RECOMB_STIM);

            ImGui::Separator();
            ImGui::Text("Statistics & Quantum:");
            flag_checkbox("Fermi-Dirac Statistics", effects, GRID_FERMI_DIRAC);
            flag_checkbox("QW Free Carriers", effects, GRID_QW_FREE_CARR);
            flag_checkbox("Bandgap Narrowing", effects, GRID_BAND_NARROWING);
            flag_checkbox("Incomplete Ionization", effects, GRID_INCOMPLETE_IONIZATION);

            ImGui::Separator();
            ImGui::Text("Transport:");
            flag_checkbox("Thermionic Emission", effects, GRID_THERMIONIC);
            flag_checkbox("Tunneling", effects, GRID_TUNNELING);
            flag_checkbox("Temperature-Dependent Mobility", effects, GRID_TEMP_MOBILITY);
            flag_checkbox("Doping-Dependent Mobility", effects, GRID_DOPING_MOBILITY);

            ImGui::Separator();
            ImGui::Text("Optical:");
            flag_checkbox("Optical Generation", effects, GRID_OPTICAL_GEN);
            flag_checkbox("Incident Reflection", effects, GRID_INCIDENT_REFLECTION);

            ImGui::Separator();
            ImGui::Text("Material:");
            flag_checkbox("Temp-Dependent Electron Affinity", effects, GRID_TEMP_ELECTRON_AFFINITY);
            flag_checkbox("Abrupt Material Boundaries", effects, GRID_ABRUPT_MATERIALS);
            flag_checkbox("Carrier Relaxation", effects, GRID_RELAX);
            flag_checkbox("Temp-Dependent Mode Permittivity", effects, GRID_TEMP_MODE_PERM);
            flag_checkbox("Temp-Dependent Incident Permittivity", effects, GRID_TEMP_INC_PERM);

            ImGui::Separator();

            if (ImGui::Button("Apply", ImVec2(120, 0))) {
                if (effects != old_effects) {
                    environment.put_value(GRID_ELECTRICAL, EFFECTS, (prec)effects);
                    environment.process_recompute_flags();
                }
                show_electrical_models = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
                show_electrical_models = false;
        }
        ImGui::End();
    }

    // =====================================================================
    // Thermal Models -- regular window, opened/closed via show flag
    // =====================================================================
    if (show_thermal_models) {
        ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Thermal Models", &show_thermal_models,
                          ImGuiWindowFlags_AlwaysAutoResize)) {
            // Device-level thermal flags
            flag dev_effects = (flag)environment.get_value(DEVICE, EFFECTS, 0);
            flag old_dev = dev_effects;

            ImGui::Text("Device Temperature Model:");
            flag_checkbox("Non-Isothermal", dev_effects, DEVICE_NON_ISOTHERMAL);
            flag_checkbox("Single Temperature", dev_effects, DEVICE_SINGLE_TEMP);
            flag_checkbox("Vary Lattice Temperature", dev_effects, DEVICE_VARY_LATTICE_TEMP);
            flag_checkbox("Vary Electron Temperature", dev_effects, DEVICE_VARY_ELECTRON_TEMP);
            flag_checkbox("Vary Hole Temperature", dev_effects, DEVICE_VARY_HOLE_TEMP);

            ImGui::Separator();

            // Grid-level thermal flags
            flag grid_effects = (flag)environment.get_value(GRID_ELECTRICAL, EFFECTS, 0);
            flag old_grid = grid_effects;

            ImGui::Text("Thermal Effects:");
            flag_checkbox("Temp-Dependent Thermal Conductivity", grid_effects, GRID_TEMP_THERMAL_COND);
            flag_checkbox("Lateral Heat Flow", grid_effects, GRID_LATERAL_HEAT);
            flag_checkbox("Joule Heating", grid_effects, GRID_JOULE_HEAT);
            flag_checkbox("Thermoelectric Heating", grid_effects, GRID_THERMOELECTRIC_HEAT);

            ImGui::Separator();

            // Environment temperature clamping
            flag env_effects = (flag)environment.get_value(ENVIRONMENT, EFFECTS, 0);
            flag old_env = env_effects;
            flag_checkbox("Clamp Temperature", env_effects, ENV_CLAMP_TEMPERATURE);

            if (env_effects & ENV_CLAMP_TEMPERATURE) {
                float temp_clamp = (float)environment.get_value(ENVIRONMENT, TEMP_CLAMP_VALUE);
                if (ImGui::InputFloat("Temperature Clamp (K)", &temp_clamp, 1.0f, 10.0f, "%.1f")) {
                    environment.put_value(ENVIRONMENT, TEMP_CLAMP_VALUE, (prec)temp_clamp);
                }

                float temp_relax = (float)environment.get_value(ENVIRONMENT, TEMP_RELAX_VALUE);
                if (ImGui::InputFloat("Temperature Relaxation", &temp_relax, 0.01f, 0.1f, "%.3f")) {
                    environment.put_value(ENVIRONMENT, TEMP_RELAX_VALUE, (prec)temp_relax);
                }
            }

            ImGui::Separator();

            if (ImGui::Button("Apply", ImVec2(120, 0))) {
                bool any_changed = false;
                if (dev_effects != old_dev) {
                    environment.put_value(DEVICE, EFFECTS, (prec)dev_effects);
                    any_changed = true;
                }
                if (grid_effects != old_grid) {
                    environment.put_value(GRID_ELECTRICAL, EFFECTS, (prec)grid_effects);
                    any_changed = true;
                }
                if (env_effects != old_env) {
                    environment.put_value(ENVIRONMENT, EFFECTS, (prec)env_effects);
                    any_changed = true;
                }
                if (any_changed)
                    environment.process_recompute_flags();
                show_thermal_models = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
                show_thermal_models = false;
        }
        ImGui::End();
    }

    // =====================================================================
    // Optical Input
    // =====================================================================
    if (show_optical_input) {
        ImGui::OpenPopup("Optical Input");
        show_optical_input = false;
    }
    if (ImGui::BeginPopupModal("Optical Input", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        // Environment optical flags
        flag env_effects = (flag)environment.get_value(ENVIRONMENT, EFFECTS, 0);
        flag old_env = env_effects;

        flag_checkbox("Enable Optical Generation", env_effects, ENV_OPTICAL_GEN);
        flag_checkbox("Enable Incident Reflection", env_effects, ENV_INCIDENT_REFLECTION);
        flag_checkbox("Spectrum Over Entire Device", env_effects, ENV_SPEC_ENTIRE_DEVICE);
        flag_checkbox("Left-side Incident Light", env_effects, ENV_SPEC_LEFT_INCIDENT);

        ImGui::Separator();

        // Spectrum parameters
        float spec_mult = (float)environment.get_value(ENVIRONMENT, SPECTRUM_MULTIPLIER);
        if (ImGui::InputFloat("Spectrum Multiplier", &spec_mult, 0.1f, 1.0f, "%.3f")) {
            environment.put_value(ENVIRONMENT, SPECTRUM_MULTIPLIER, (prec)spec_mult);
        }

        float spec_start = (float)environment.get_value(ENVIRONMENT, SPEC_START_POSITION);
        if (ImGui::InputFloat("Start Position (cm)", &spec_start, 0.0f, 0.0f, "%.4e")) {
            environment.put_value(ENVIRONMENT, SPEC_START_POSITION, (prec)spec_start);
        }

        float spec_end = (float)environment.get_value(ENVIRONMENT, SPEC_END_POSITION);
        if (ImGui::InputFloat("End Position (cm)", &spec_end, 0.0f, 0.0f, "%.4e")) {
            environment.put_value(ENVIRONMENT, SPEC_END_POSITION, (prec)spec_end);
        }

        ImGui::Separator();
        ImGui::Text("Spectral Components:");

        int num_spec = environment.get_number_objects(SPECTRUM);

        if (ImGui::Button("Add Component")) {
            environment.add_spectral_comp();
            num_spec = environment.get_number_objects(SPECTRUM);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Spectrum...")) {
            auto sel = pfd::open_file("Load Spectrum", ".",
                {"Spectrum Files", "*.spc *.txt",
                 "All Files", "*"}).result();
            if (!sel.empty()) {
                environment.load_spectrum(sel[0].c_str());
                add_log(std::string("Loaded spectrum: ") + sel[0]);
            }
        }

        if (num_spec > 0) {
            ImGui::BeginChild("SpecList", ImVec2(0, 200), ImGuiChildFlags_Border);

            for (int i = 0; i < num_spec; i++) {
                ImGui::PushID(i);

                float energy = (float)environment.get_value(SPECTRUM, INCIDENT_PHOTON_ENERGY, i);
                float wavelength = (float)environment.get_value(SPECTRUM, INCIDENT_PHOTON_WAVELENGTH, i);
                float intensity = (float)environment.get_value(SPECTRUM, INCIDENT_INPUT_INTENSITY, i);

                ImGui::Text("Component %d:", i);
                ImGui::SameLine();
                ImGui::TextDisabled("%.4f eV (%.1f nm)", energy, wavelength * 1e7);

                if (ImGui::InputFloat("Photon Energy (eV)", &energy, 0.01f, 0.1f, "%.4f")) {
                    environment.put_value(SPECTRUM, INCIDENT_PHOTON_ENERGY, (prec)energy, i, i);
                }
                if (ImGui::InputFloat("Intensity (W/cm2)", &intensity, 0.0f, 0.0f, "%.4e")) {
                    environment.put_value(SPECTRUM, INCIDENT_INPUT_INTENSITY, (prec)intensity, i, i);
                }

                ImGui::Separator();
                ImGui::PopID();
            }

            ImGui::EndChild();
        } else {
            ImGui::TextDisabled("No spectral components defined.");
        }

        ImGui::Separator();

        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            bool effects_changed = (env_effects != old_env);
            if (effects_changed) {
                environment.put_value(ENVIRONMENT, EFFECTS, (prec)env_effects);
            }
            environment.process_recompute_flags();
            // Inform user about optical generation status
            int num_spec_now = environment.get_number_objects(SPECTRUM);
            if (env_effects & ENV_OPTICAL_GEN) {
                if (num_spec_now == 0) {
                    app.add_log("WARNING: Optical generation enabled but no spectral components defined.");
                    app.add_log("  Add a spectral component (photon energy + intensity), then re-simulate.");
                } else {
                    app.add_log("Optical input applied. Re-run simulation (F5) to compute photocurrent.");
                }
            } else if (effects_changed) {
                app.add_log("Optical generation disabled. Re-run simulation (F5) to update.");
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    // =====================================================================
    // Preferences (full)
    // =====================================================================
    if (show_preferences) {
        ImGui::OpenPopup("Preferences");
        show_preferences = false;
    }
    if (ImGui::BeginPopupModal("Preferences", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        // --- Environment ---
        ImGui::Text("Environment:");

        float temperature = (float)environment.get_value(ENVIRONMENT, TEMPERATURE);
        ImGui::InputFloat("Temperature (K)", &temperature, 1.0f, 10.0f, "%.1f");

        float radius = (float)environment.get_value(ENVIRONMENT, RADIUS);
        ImGui::InputFloat("Device Radius (cm)", &radius, 0.0f, 0.0f, "%.4e");

        ImGui::Separator();
        ImGui::Text("Error Tolerances:");

        float max_elec_err = (float)environment.get_value(ENVIRONMENT, MAX_ELECTRICAL_ERROR);
        ImGui::InputFloat("Max Electrical Error", &max_elec_err, 0.0f, 0.0f, "%.2e");

        float max_therm_err = (float)environment.get_value(ENVIRONMENT, MAX_THERMAL_ERROR);
        ImGui::InputFloat("Max Thermal Error", &max_therm_err, 0.0f, 0.0f, "%.2e");

        float max_optic_err = (float)environment.get_value(ENVIRONMENT, MAX_OPTIC_ERROR);
        ImGui::InputFloat("Max Optical Error", &max_optic_err, 0.0f, 0.0f, "%.2e");

        float coarse_mode_err = (float)environment.get_value(ENVIRONMENT, COARSE_MODE_ERROR);
        ImGui::InputFloat("Coarse Mode Error", &coarse_mode_err, 0.0f, 0.0f, "%.2e");

        float fine_mode_err = (float)environment.get_value(ENVIRONMENT, FINE_MODE_ERROR);
        ImGui::InputFloat("Fine Mode Error", &fine_mode_err, 0.0f, 0.0f, "%.2e");

        ImGui::Separator();
        ImGui::Text("Clamp Values:");

        float pot_clamp = (float)environment.get_value(ENVIRONMENT, POT_CLAMP_VALUE);
        ImGui::InputFloat("Potential Clamp (V)", &pot_clamp, 0.01f, 0.1f, "%.3f");

        float temp_clamp = (float)environment.get_value(ENVIRONMENT, TEMP_CLAMP_VALUE);
        ImGui::InputFloat("Temperature Clamp (K)", &temp_clamp, 1.0f, 10.0f, "%.1f");

        float temp_relax = (float)environment.get_value(ENVIRONMENT, TEMP_RELAX_VALUE);
        ImGui::InputFloat("Temperature Relaxation", &temp_relax, 0.01f, 0.1f, "%.3f");

        ImGui::Separator();
        ImGui::Text("Iteration Limits:");

        int max_elec_iter = (int)environment.get_value(ENVIRONMENT, MAX_INNER_ELECT_ITER);
        ImGui::InputInt("Max Inner Electrical Iter", &max_elec_iter);

        int max_therm_iter = (int)environment.get_value(ENVIRONMENT, MAX_INNER_THERM_ITER);
        ImGui::InputInt("Max Inner Thermal Iter", &max_therm_iter);

        int max_outer_optic = (int)environment.get_value(ENVIRONMENT, MAX_OUTER_OPTIC_ITER);
        ImGui::InputInt("Max Outer Optical Iter", &max_outer_optic);

        int max_outer_therm = (int)environment.get_value(ENVIRONMENT, MAX_OUTER_THERM_ITER);
        ImGui::InputInt("Max Outer Thermal Iter", &max_outer_therm);

        int max_mode_iter = (int)environment.get_value(ENVIRONMENT, MAX_INNER_MODE_ITER);
        ImGui::InputInt("Max Inner Mode Iter", &max_mode_iter);

        ImGui::Separator();
        ImGui::Text("Environment Options:");

        flag env_effects = (flag)environment.get_value(ENVIRONMENT, EFFECTS, 0);
        flag old_env = env_effects;
        flag_checkbox("Clamp Potential", env_effects, ENV_CLAMP_POTENTIAL);
        flag_checkbox("Clamp Temperature", env_effects, ENV_CLAMP_TEMPERATURE);

        ImGui::Separator();
        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            environment.put_value(ENVIRONMENT, TEMPERATURE, (prec)temperature);
            environment.put_value(ENVIRONMENT, RADIUS, (prec)radius);
            environment.put_value(ENVIRONMENT, MAX_ELECTRICAL_ERROR, (prec)max_elec_err);
            environment.put_value(ENVIRONMENT, MAX_THERMAL_ERROR, (prec)max_therm_err);
            environment.put_value(ENVIRONMENT, MAX_OPTIC_ERROR, (prec)max_optic_err);
            environment.put_value(ENVIRONMENT, COARSE_MODE_ERROR, (prec)coarse_mode_err);
            environment.put_value(ENVIRONMENT, FINE_MODE_ERROR, (prec)fine_mode_err);
            environment.put_value(ENVIRONMENT, POT_CLAMP_VALUE, (prec)pot_clamp);
            environment.put_value(ENVIRONMENT, TEMP_CLAMP_VALUE, (prec)temp_clamp);
            environment.put_value(ENVIRONMENT, TEMP_RELAX_VALUE, (prec)temp_relax);
            environment.put_value(ENVIRONMENT, MAX_INNER_ELECT_ITER, (prec)max_elec_iter);
            environment.put_value(ENVIRONMENT, MAX_INNER_THERM_ITER, (prec)max_therm_iter);
            environment.put_value(ENVIRONMENT, MAX_OUTER_OPTIC_ITER, (prec)max_outer_optic);
            environment.put_value(ENVIRONMENT, MAX_OUTER_THERM_ITER, (prec)max_outer_therm);
            environment.put_value(ENVIRONMENT, MAX_INNER_MODE_ITER, (prec)max_mode_iter);
            if (env_effects != old_env)
                environment.put_value(ENVIRONMENT, EFFECTS, (prec)env_effects);
            environment.process_recompute_flags();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    // =====================================================================
    // Device Info
    // =====================================================================
    if (show_device_info) {
        ImGui::OpenPopup("Device Info");
        show_device_info = false;
    }
    if (ImGui::BeginPopupModal("Device Info", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Device File: %s", device_file_path.c_str());
        ImGui::Text("Material File: %s", material_file_path.c_str());
        ImGui::Separator();

        int nodes = environment.get_number_objects(NODE);
        int contacts = environment.get_number_objects(CONTACT);
        int surfaces = environment.get_number_objects(SURFACE);
        int qws = environment.get_number_objects(QUANTUM_WELL);
        int mirrors = environment.get_number_objects(MIRROR);
        int modes = environment.get_number_objects(MODE);
        int cavities = environment.get_number_objects(CAVITY);
        int spectra = environment.get_number_objects(SPECTRUM);

        ImGui::Text("Grid Nodes: %d", nodes);
        ImGui::Text("Contacts: %d", contacts);
        ImGui::Text("Surfaces: %d", surfaces);
        ImGui::Text("Quantum Wells: %d", qws);
        ImGui::Text("Mirrors: %d", mirrors);
        ImGui::Text("Cavities: %d", cavities);
        ImGui::Text("Modes: %d", modes);
        ImGui::Text("Spectral Components: %d", spectra);

        if (nodes > 0) {
            ImGui::Separator();
            ImGui::Text("Environment:");
            float temp = (float)environment.get_value(ENVIRONMENT, TEMPERATURE);
            ImGui::Text("  Temperature: %.1f K", temp);
            float radius = (float)environment.get_value(ENVIRONMENT, RADIUS);
            if (radius > 0)
                ImGui::Text("  Device Radius: %.4e cm", radius);

            ImGui::Separator();
            ImGui::Text("Solver Status:");
            float err_psi = (float)environment.get_value(DEVICE, ERROR_PSI, 0);
            float err_ec = (float)environment.get_value(DEVICE, ERROR_ETA_C, 0);
            float err_ev = (float)environment.get_value(DEVICE, ERROR_ETA_V, 0);
            ImGui::Text("  Error Psi:   %.4e", err_psi);
            ImGui::Text("  Error Eta_c: %.4e", err_ec);
            ImGui::Text("  Error Eta_v: %.4e", err_ev);
        }

        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    // =====================================================================
    // Laser Info (regular window, read-only)
    // =====================================================================
    if (show_laser_info) {
        ImGui::SetNextWindowSize(ImVec2(420, 350), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Laser Info", &show_laser_info)) {
            int num_modes = environment.get_number_objects(MODE);
            int num_cavities = environment.get_number_objects(CAVITY);
            int num_mirrors = environment.get_number_objects(MIRROR);

            if (num_modes == 0 && num_cavities == 0) {
                ImGui::TextWrapped("No laser modes or cavities in current device. "
                    "Laser parameters are only available for devices with cavity/mirror definitions.");
            }

            if (num_cavities > 0 && ImGui::CollapsingHeader("Cavities", ImGuiTreeNodeFlags_DefaultOpen)) {
                for (int c = 0; c < num_cavities; c++) {
                    double area = environment.get_value(CAVITY, AREA, c);
                    int type = (int)environment.get_value(CAVITY, TYPE, c);
                    double length = environment.get_value(CAVITY, LENGTH, c);
                    const char* type_str = (type == 1) ? "Edge" : (type == 2) ? "Surface" : "Unknown";
                    ImGui::Text("Cavity %d: %s, Area=%.4g cm2, Length=%.4g cm", c, type_str, area, length);
                }
            }

            if (num_modes > 0 && ImGui::CollapsingHeader("Lasing Modes", ImGuiTreeNodeFlags_DefaultOpen)) {
                for (int m = 0; m < num_modes; m++) {
                    ImGui::PushID(m);
                    if (ImGui::TreeNode("Mode", "Mode %d", m)) {
                        double pe = environment.get_value(MODE, MODE_PHOTON_ENERGY, m);
                        double wl = environment.get_value(MODE, MODE_PHOTON_WAVELENGTH, m);
                        double tp = environment.get_value(MODE, MODE_TOTAL_PHOTONS, m);
                        double pl = environment.get_value(MODE, PHOTON_LIFETIME, m);
                        double ml = environment.get_value(MODE, MIRROR_LOSS, m);
                        double wg = environment.get_value(MODE, WAVEGUIDE_LOSS, m);
                        double mg = environment.get_value(MODE, MODE_GAIN, m);

                        ImGui::Text("Photon Energy:    %.6g eV", pe);
                        ImGui::Text("Wavelength:       %.6g um", wl);
                        ImGui::Text("Total Photons:    %.6e", tp);
                        ImGui::Text("Photon Lifetime:  %.6e s", pl);
                        ImGui::Text("Mirror Loss:      %.6g cm-1", ml);
                        ImGui::Text("Waveguide Loss:   %.6g cm-1", wg);
                        ImGui::Text("Mode Gain:        %.6g cm-1", mg);
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }

            if (num_mirrors > 0 && ImGui::CollapsingHeader("Mirrors", ImGuiTreeNodeFlags_DefaultOpen)) {
                for (int m = 0; m < num_mirrors; m++) {
                    double power = environment.get_value(MIRROR, POWER, m);
                    ImGui::Text("Mirror %d: Power=%.6e W/cm2", m, power);
                }
            }
        }
        ImGui::End();
    }

    // =====================================================================
    // Voltage Sweep (rendered as a regular window, not a popup)
    // =====================================================================
    render_voltage_sweep();
}

void SimWindowsApp::render_voltage_sweep()
{
    if (!show_voltage_sweep) return;

    ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Voltage Sweep", &show_voltage_sweep)) {
        ImGui::End();
        return;
    }

    // Input fields (disabled when sweep is running)
    ImGui::BeginDisabled(sweep_running);
    ImGui::InputFloat("Start (V)", &sweep_start, 0.1f, 1.0f, "%.4f");
    ImGui::InputFloat("End (V)", &sweep_end, 0.1f, 1.0f, "%.4f");
    ImGui::InputFloat("Step (V)", &sweep_step, 0.01f, 0.1f, "%.4f");
    ImGui::InputInt("Contact", &sweep_contact);
    sweep_contact = std::max(0, std::min(sweep_contact, 1));
    ImGui::EndDisabled();

    // Run/Stop buttons
    if (!sweep_running) {
        if (ImGui::Button("Run Sweep")) {
            start_voltage_sweep();
        }
    } else {
        if (ImGui::Button("Stop Sweep")) {
            stop_voltage_sweep();
        }
        ImGui::SameLine();
        ImGui::Text("Step %d / %d  V=%.4f", sweep_current_step, sweep_total_steps, sweep_current_voltage);
    }

    // IV curve plot
    {
        std::lock_guard<std::mutex> lock(sweep_mutex);
        if (!sweep_iv_data.empty()) {
            // Build separate V and I vectors for ImPlot
            std::vector<double> vv, ii;
            vv.reserve(sweep_iv_data.size());
            ii.reserve(sweep_iv_data.size());
            for (auto& [v, i] : sweep_iv_data) {
                vv.push_back(v);
                ii.push_back(i);
            }

            if (ImPlot::BeginPlot("I-V Curve", ImVec2(-1, 250))) {
                ImPlot::SetupAxes("Voltage (V)", "Current (A/cm2)");
                ImPlot::PlotLine("I-V", vv.data(), ii.data(), (int)vv.size());
                ImPlot::EndPlot();
            }
        }
    }

    // Export CSV button
    if (!sweep_iv_data.empty() && !sweep_running) {
        if (ImGui::Button("Export CSV")) {
            auto dest = pfd::save_file("Export Sweep Data", "iv_sweep.csv",
                {"CSV Files", "*.csv", "All Files", "*"}).result();
            if (!dest.empty()) {
                std::ofstream ofs(dest);
                if (ofs.good()) {
                    ofs << "Voltage(V),Current(A/cm2)\n";
                    std::lock_guard<std::mutex> lock(sweep_mutex);
                    for (auto& [v, i] : sweep_iv_data)
                        ofs << v << "," << i << "\n";
                    add_log("Exported sweep: " + dest);
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Results")) {
            std::lock_guard<std::mutex> lock(sweep_mutex);
            sweep_iv_data.clear();
        }
    }

    ImGui::End();
}
