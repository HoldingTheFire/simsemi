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
#include <fstream>
#include <cmath>

extern TEnvironment environment;
extern TErrorHandler error_handler;

void SimWindowsApp::export_operating_point(const char* path)
{
    std::ofstream ofs(path);
    if (!ofs.good()) {
        add_log(std::string("ERROR: Could not write ") + path);
        return;
    }

    ofs << "SimWindows Operating Point\n";
    ofs << "==========================\n\n";

    int num_contacts = environment.get_number_objects(CONTACT);
    for (int c = 0; c < num_contacts; c++) {
        double bias = environment.get_value(CONTACT, APPLIED_BIAS, c);
        double current = environment.get_value(CONTACT, TOTAL_CURRENT, c);
        double e_current = environment.get_value(CONTACT, ELECTRON_CURRENT, c);
        double h_current = environment.get_value(CONTACT, HOLE_CURRENT, c);
        ofs << "Contact " << c << ":\n";
        ofs << "  Applied Bias:     " << bias << " V\n";
        ofs << "  Total Current:    " << current << " A/cm2\n";
        ofs << "  Electron Current: " << e_current << " A/cm2\n";
        ofs << "  Hole Current:     " << h_current << " A/cm2\n\n";
    }

    int num_modes = environment.get_number_objects(MODE);
    if (num_modes > 0) {
        ofs << "Laser Modes:\n";
        for (int m = 0; m < num_modes; m++) {
            double photon_energy = environment.get_value(MODE, MODE_PHOTON_ENERGY, m);
            double wavelength = environment.get_value(MODE, MODE_PHOTON_WAVELENGTH, m);
            double total_photons = environment.get_value(MODE, MODE_TOTAL_PHOTONS, m);
            ofs << "  Mode " << m << ": E=" << photon_energy << " eV, lambda="
                << wavelength << " um, photons=" << total_photons << "\n";
        }
        ofs << "\n";
    }

    int num_cavities = environment.get_number_objects(CAVITY);
    if (num_cavities > 0) {
        ofs << "Cavities: " << num_cavities << "\n";
        for (int c = 0; c < num_cavities; c++) {
            double area = environment.get_value(CAVITY, AREA, c);
            double length = environment.get_value(CAVITY, LENGTH, c);
            ofs << "  Cavity " << c << ": area=" << area << " cm2, length=" << length << " cm\n";
        }
        ofs << "\n";
    }

    int num_mirrors = environment.get_number_objects(MIRROR);
    if (num_mirrors > 0) {
        ofs << "Mirrors:\n";
        for (int m = 0; m < num_mirrors; m++) {
            double power = environment.get_value(MIRROR, POWER, m);
            ofs << "  Mirror " << m << ": power=" << power << " W/cm2\n";
        }
        ofs << "\n";
    }

    ofs << "Device file: " << device_file_path << "\n";

    add_log(std::string("Exported operating point: ") + path);
}

void SimWindowsApp::export_all_spatial(const char* path)
{
    std::ofstream ofs(path);
    if (!ofs.good()) {
        add_log(std::string("ERROR: Could not write ") + path);
        return;
    }

    int num_nodes = environment.get_number_objects(NODE);
    if (num_nodes <= 0) {
        add_log("ERROR: No nodes in device");
        return;
    }

    // Header
    ofs << "Position_um,Ec_eV,Ev_eV,Efn_eV,Efp_eV,"
        << "Electron_cm3,Hole_cm3,"
        << "Jn_A_cm2,Jp_A_cm2,Jtotal_A_cm2,"
        << "Field_V_cm,Potential_V,"
        << "Nd_cm3,Na_cm3,"
        << "SHR_Recomb_cm3s,BB_Recomb_cm3s,Total_Recomb_cm3s,"
        << "Opt_Gen_cm3s,Temperature_K\n";

    for (int i = 0; i < num_nodes; i++) {
        double pos = environment.get_value(GRID_ELECTRICAL, POSITION, i) * 1e4; // cm -> um
        double ec = environment.get_value(ELECTRON, BAND_EDGE, i);
        double ev = environment.get_value(HOLE, BAND_EDGE, i);

        // Quasi-Fermi levels: get_value returns the offset from band edge
        double qf_n = environment.get_value(ELECTRON, QUASI_FERMI, i);
        double qf_p = environment.get_value(HOLE, QUASI_FERMI, i);
        double efn = ec - qf_n;  // Electron quasi-Fermi level
        double efp = ev + qf_p;  // Hole quasi-Fermi level

        double n = environment.get_value(ELECTRON, CONCENTRATION, i);
        double p = environment.get_value(HOLE, CONCENTRATION, i);
        double jn = environment.get_value(ELECTRON, CURRENT, i);
        double jp = environment.get_value(HOLE, CURRENT, i);
        double jtot = environment.get_value(NODE, TOTAL_CURRENT, i);
        double field = environment.get_value(GRID_ELECTRICAL, FIELD, i);
        double pot = environment.get_value(GRID_ELECTRICAL, POTENTIAL, i);
        double nd = environment.get_value(ELECTRON, DOPING_CONC, i);
        double na = environment.get_value(HOLE, DOPING_CONC, i);
        double shr = environment.get_value(NODE, SHR_RECOMB, i);
        double bb = environment.get_value(NODE, B_B_RECOMB, i);
        double rtot = environment.get_value(NODE, TOTAL_RECOMB, i);
        double optgen = environment.get_value(NODE, OPTICAL_GENERATION, i);
        double temp = environment.get_value(GRID_ELECTRICAL, TEMPERATURE, i);

        ofs << pos << "," << ec << "," << ev << "," << efn << "," << efp << ","
            << n << "," << p << ","
            << jn << "," << jp << "," << jtot << ","
            << field << "," << pot << ","
            << nd << "," << na << ","
            << shr << "," << bb << "," << rtot << ","
            << optgen << "," << temp << "\n";
    }

    add_log(std::string("Exported spatial data: ") + path);
}

void SimWindowsApp::export_band_data(const char* path)
{
    std::ofstream ofs(path);
    if (!ofs.good()) {
        add_log(std::string("ERROR: Could not write ") + path);
        return;
    }

    int num_nodes = environment.get_number_objects(NODE);
    if (num_nodes <= 0) {
        add_log("ERROR: No nodes in device");
        return;
    }

    ofs << "Position_um,Ec_eV,Ev_eV,Efn_eV,Efp_eV\n";

    for (int i = 0; i < num_nodes; i++) {
        double pos = environment.get_value(GRID_ELECTRICAL, POSITION, i) * 1e4;
        double ec = environment.get_value(ELECTRON, BAND_EDGE, i);
        double ev = environment.get_value(HOLE, BAND_EDGE, i);
        double qf_n = environment.get_value(ELECTRON, QUASI_FERMI, i);
        double qf_p = environment.get_value(HOLE, QUASI_FERMI, i);
        double efn = ec - qf_n;
        double efp = ev + qf_p;

        ofs << pos << "," << ec << "," << ev << "," << efn << "," << efp << "\n";
    }

    add_log(std::string("Exported band data: ") + path);
}
