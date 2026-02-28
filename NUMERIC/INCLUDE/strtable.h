/*
    SimWindows - 1D Semiconductor Device Simulator
    Copyright (C) 2013 David W. Winston

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#define ENCODE_KEY	5

#ifdef CUSTOM
extern const char custom_string[];
#endif
extern const char about_string[];
extern const char state_version_string[];
extern const char update_string[];
extern const char state_string[];
extern const char undo_filename[];
extern const char ini_filename[];

extern int state_string_size;
extern int state_version_string_size;

#ifdef _SIMWINDOWS_32
extern const char application_string[];
extern const char executable_string[];
extern const char cap_executable_string[];
#endif

#ifdef _SIMWINDOWS_DB
extern const char application_string[];
extern const char executable_string[];
extern const char cap_executable_string[];
#endif

extern char executable_path[];

#ifndef NDEBUG
extern const char debug_string[];
#endif

/********************************* Material Parameters Strings *********************************
	All material parameters strings must be capatilized and in the same order as the
	MAT_xxx define constants
*/

extern const char *material_parameters_strings[];
extern const char *material_parameters_variables[];

//***************************** Free Electron Strings ******************************************
extern const char *free_electron_long_table[];
extern const char *free_electron_short_table[];

//********************************* Bound Electron Strings ************************************
extern const char *bound_electron_long_table[];
extern const char *bound_electron_short_table[];

//********************************** Free Hole Strings ****************************************
extern const char *free_hole_long_table[];
extern const char *free_hole_short_table[];

//******************************** Bound Hole Strings *****************************************
extern const char *bound_hole_long_table[];
extern const char *bound_hole_short_table[];

//********************************** Electron Strings *****************************************
extern const char *electron_long_table[];
extern const char *electron_short_table[];

//************************************* Hole Strings *****************************************
extern const char *hole_long_table[];
extern const char *hole_short_table[];

//***************************** Grid Electrical Strings ***************************************
extern const char *grid_electrical_long_table[];
extern const char *grid_electrical_short_table[];

//***************************** Grid Optical Strings *****************************************
extern const char *grid_optical_long_table[];
extern const char *grid_optical_short_table[];

//************************************** Node Strings *****************************************
extern const char *node_long_table[];
extern const char *node_short_table[];

//***************************** QW Electron Strings *******************************************
extern const char *qw_electron_long_table[];
extern const char *qw_electron_short_table[];

//*********************************** QW Hole Strings *****************************************
extern const char *qw_hole_long_table[];
extern const char *qw_hole_short_table[];

//******************************* QuantumWell Strings *****************************************
extern const char *quantum_well_long_table[];
extern const char *quantum_well_short_table[];

//************************************** Mode Strings *****************************************
extern const char *mode_long_table[];
extern const char *mode_short_table[];

//************************************* Mirror Strings *****************************************
extern const char *mirror_long_table[];
extern const char *mirror_short_table[];

//************************************* Cavity Strings *****************************************
extern const char *cavity_long_table[];
extern const char *cavity_short_table[];

//*********************************** Contact Strings *****************************************
extern const char *contact_long_table[];
extern const char *contact_short_table[];

//************************************ Surface Strings *****************************************
extern const char *surface_long_table[];
extern const char *surface_short_table[];

//************************************ Device Strings *****************************************
extern const char *device_long_table[];
extern const char *device_short_table[];

//******************************* Environment Strings *****************************************
extern const char *environment_long_table[];
extern const char *environment_short_table[];

//*********************************** Spectrum Strings *****************************************
extern const char *spectrum_long_table[];
extern const char *spectrum_short_table[];

//*********************************** String Tables *******************************************
extern const char **long_string_table[];
extern const char **short_string_table[];
