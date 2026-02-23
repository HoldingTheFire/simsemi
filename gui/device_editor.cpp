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
#include "app.h"
#include "portable-file-dialogs.h"
#include <fstream>
#include <cstdio>
#include <filesystem>

extern TEnvironment environment;
extern TErrorHandler error_handler;

void SimWindowsApp::render_device_editor()
{
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Device Editor", &show_device_editor)) {
        ImGui::End();
        return;
    }

    // Toolbar
    bool can_generate = material_loaded && !solving;

    if (ImGui::Button("Generate") && can_generate) {
        // Write text to a temp file and load it
        namespace fs = std::filesystem;
        std::string tmp_path = fs::temp_directory_path().string() + "/simgui_temp.dev";

        {
            std::ofstream ofs(tmp_path);
            ofs << device_text;
        }

        environment.load_file(tmp_path.c_str());

        if (error_handler.fail()) {
            add_log("ERROR: " + error_handler.get_error_string());
            device_loaded = false;
        } else {
            device_loaded = true;
            add_log("Device generated from editor.");
        }

        // Clean up temp file
        std::remove(tmp_path.c_str());
    }

    ImGui::SameLine();
    if (ImGui::Button("Save...")) {
        std::string default_name = device_file_path.empty() ? "device.dev" : device_file_path;
        auto dest = pfd::save_file("Save Device File", default_name,
            {"Device Files", "*.dev", "All Files", "*"}).result();
        if (!dest.empty()) {
            std::ofstream ofs(dest);
            if (ofs.good()) {
                ofs << device_text;
                add_log("Saved device file: " + dest);
            } else {
                add_log("ERROR: Could not write " + dest);
            }
        }
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(%d chars)", (int)device_text.size());

    ImGui::Separator();

    // Text editor - use a large buffer
    static char text_buf[64 * 1024] = {};
    if (device_text.size() < sizeof(text_buf) - 1) {
        // Sync from device_text to buffer (only if changed externally)
        static std::string last_synced;
        if (device_text != last_synced) {
            strncpy(text_buf, device_text.c_str(), sizeof(text_buf) - 1);
            text_buf[sizeof(text_buf) - 1] = '\0';
            last_synced = device_text;
        }
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (ImGui::InputTextMultiline("##devtext", text_buf, sizeof(text_buf),
                                   avail,
                                   ImGuiInputTextFlags_AllowTabInput)) {
        device_text = text_buf;
    }

    ImGui::End();
}
