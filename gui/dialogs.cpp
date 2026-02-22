/*
    SimWindows - 1D Semiconductor Device Simulator
    Copyright (C) 2013 David W. Winston

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "comincl.h"
#include "imgui.h"
#include "app.h"

extern TEnvironment      environment;
extern TErrorHandler     error_handler;
extern TPreferences      preferences;

void SimWindowsApp::render_dialogs()
{
    // --- About ---
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

    // --- Contacts ---
    if (show_contacts) {
        ImGui::OpenPopup("Contacts");
        show_contacts = false;
    }
    if (ImGui::BeginPopupModal("Contacts", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        int num_contacts = environment.get_number_objects(CONTACT);

        for (int c = 0; c < num_contacts; c++) {
            ImGui::PushID(c);
            ImGui::Text("Contact %d (%s)", c, c == 0 ? "Left" : "Right");

            float bias = (float)environment.get_value(CONTACT, APPLIED_BIAS, c);
            if (ImGui::InputFloat("Applied Bias (V)", &bias, 0.01f, 0.1f, "%.4f")) {
                environment.put_value(CONTACT, APPLIED_BIAS, (prec)bias, c, c);
            }

            float barrier = (float)environment.get_value(CONTACT, BARRIER_HEIGHT, c);
            ImGui::InputFloat("Barrier Height (eV)", &barrier, 0.01f, 0.1f, "%.4f");

            ImGui::Separator();
            ImGui::PopID();
        }

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            environment.process_recompute_flags();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    // --- Surfaces ---
    if (show_surfaces) {
        ImGui::OpenPopup("Surfaces");
        show_surfaces = false;
    }
    if (ImGui::BeginPopupModal("Surfaces", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        int num_surfaces = environment.get_number_objects(SURFACE);

        for (int s = 0; s < num_surfaces; s++) {
            ImGui::PushID(s);
            ImGui::Text("Surface %d", s);

            float temp = (float)environment.get_value(SURFACE, TEMPERATURE, s);
            ImGui::InputFloat("Temperature (K)", &temp, 1.0f, 10.0f, "%.1f");

            ImGui::Separator();
            ImGui::PopID();
        }

        if (ImGui::Button("OK", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    // --- Electrical Models ---
    if (show_electrical_models) {
        ImGui::OpenPopup("Electrical Models");
        show_electrical_models = false;
    }
    if (ImGui::BeginPopupModal("Electrical Models", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Recombination Models:");
        // These are read-only display for now; full editing in Phase 3b
        flag effects = (flag)environment.get_value(DEVICE, EFFECTS, 0);
        ImGui::Text("  Device effects flags: 0x%08lX", effects);

        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    // --- Preferences ---
    if (show_preferences) {
        ImGui::OpenPopup("Preferences");
        show_preferences = false;
    }
    if (ImGui::BeginPopupModal("Preferences", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        float max_elec_err = (float)environment.get_value(ENVIRONMENT, MAX_ELECTRICAL_ERROR);
        float max_therm_err = (float)environment.get_value(ENVIRONMENT, MAX_THERMAL_ERROR);
        float max_optic_err = (float)environment.get_value(ENVIRONMENT, MAX_OPTIC_ERROR);
        float clamp = (float)environment.get_value(ENVIRONMENT, POT_CLAMP_VALUE);
        int max_elec_iter = (int)environment.get_value(ENVIRONMENT, MAX_INNER_ELECT_ITER);
        int max_therm_iter = (int)environment.get_value(ENVIRONMENT, MAX_INNER_THERM_ITER);

        ImGui::Text("Error Tolerances:");
        ImGui::InputFloat("Max Electrical Error", &max_elec_err, 0.0f, 0.0f, "%.2e");
        ImGui::InputFloat("Max Thermal Error", &max_therm_err, 0.0f, 0.0f, "%.2e");
        ImGui::InputFloat("Max Optical Error", &max_optic_err, 0.0f, 0.0f, "%.2e");

        ImGui::Separator();
        ImGui::Text("Iteration Limits:");
        ImGui::InputInt("Max Electrical Iterations", &max_elec_iter);
        ImGui::InputInt("Max Thermal Iterations", &max_therm_iter);

        ImGui::Separator();
        ImGui::Text("Clamp Values:");
        ImGui::InputFloat("Potential Clamp (V)", &clamp, 0.01f, 0.1f, "%.3f");

        ImGui::Separator();
        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            environment.put_value(ENVIRONMENT, MAX_ELECTRICAL_ERROR, (prec)max_elec_err);
            environment.put_value(ENVIRONMENT, MAX_THERMAL_ERROR, (prec)max_therm_err);
            environment.put_value(ENVIRONMENT, MAX_OPTIC_ERROR, (prec)max_optic_err);
            environment.put_value(ENVIRONMENT, POT_CLAMP_VALUE, (prec)clamp);
            environment.put_value(ENVIRONMENT, MAX_INNER_ELECT_ITER, (prec)max_elec_iter);
            environment.put_value(ENVIRONMENT, MAX_INNER_THERM_ITER, (prec)max_therm_iter);
            environment.process_recompute_flags();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    // --- Device Info ---
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

        ImGui::Text("Grid Nodes: %d", nodes);
        ImGui::Text("Contacts: %d", contacts);
        ImGui::Text("Surfaces: %d", surfaces);
        ImGui::Text("Quantum Wells: %d", qws);
        ImGui::Text("Mirrors: %d", mirrors);

        if (nodes > 0) {
            ImGui::Separator();
            float temp = (float)environment.get_value(ENVIRONMENT, TEMPERATURE);
            ImGui::Text("Temperature: %.1f K", temp);
        }

        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}
