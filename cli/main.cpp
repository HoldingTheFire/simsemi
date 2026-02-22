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

/*
 * main.cpp — CLI entry point for SimWindows headless simulation.
 *
 * Usage:  simcli <device-file.dev>
 *
 * SIMSTRCT.H declares the following globals as extern; they were previously
 * defined in OWL/simwin.cpp.  The CLI must define them here.
 */

#include "comincl.h"
// strtable.h defines global string tables (state_version_string, long_string_table,
// material_parameters_strings, executable_path, etc.) that NUMERIC references via
// extern declarations.  It must be included in exactly one translation unit, just
// as OWL/simwin.cpp did previously.
#include "strtable.h"
#include <cstdio>
#include <cstdlib>

// Global objects required by the NUMERIC library (declared extern in SIMSTRCT.H).
// TMacroStorage is OWL-side and never accessed by NUMERIC, so omitted here.
TEnvironment      environment;
TMaterialStorage  material_parameters;
TErrorHandler     error_handler;
TPreferences      preferences;
NormalizeConstants normalization;

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <device-file.dev>\n", argv[0]);
        return 2;
    }

    const char* device_file = argv[1];

    printf("SimWindows CLI — loading: %s\n", device_file);
    environment.load_file(device_file);

    if (error_handler.fail()) {
        fprintf(stderr, "Load failed: %s\n",
                error_handler.get_error_string().c_str());
        return 1;
    }

    printf("Solving...\n");
    environment.solve();

    if (error_handler.fail()) {
        fprintf(stderr, "Solve failed: %s\n",
                error_handler.get_error_string().c_str());
        return 1;
    }

    printf("Done.\n");
    return 0;
}
