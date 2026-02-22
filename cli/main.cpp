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
 * Usage:  simcli [options] <device-file>
 *
 * Options:
 *   -m <path>    Path to material parameter file (default: material.prm
 *                in the same directory as the executable)
 *   -o <path>    Write simulation state to this file after solving
 *
 * SIMSTRCT.H declares the following globals as extern; they were previously
 * defined in OWL/simwin.cpp.  The CLI must define them here.
 */

#include "comincl.h"
#include "simparse.h"
// strtable.h defines global string tables (state_version_string, long_string_table,
// material_parameters_strings, executable_path, etc.) that NUMERIC references via
// extern declarations.  It must be included in exactly one translation unit, just
// as OWL/simwin.cpp did previously.
#include "strtable.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Global objects required by the NUMERIC library (declared extern in SIMSTRCT.H).
// TMacroStorage is OWL-side and never accessed by NUMERIC, so omitted here.
TEnvironment      environment;
TMaterialStorage  material_parameters;
TErrorHandler     error_handler;
TPreferences      preferences;
NormalizeConstants normalization;

static void print_usage(const char* prog)
{
    fprintf(stderr,
        "SimWindows CLI — 1D Semiconductor Device Simulator\n"
        "Usage: %s [options] <device-file>\n\n"
        "Options:\n"
        "  -m <path>   Material parameter file (default: <exe-dir>/material.prm)\n"
        "  -o <path>   Write state file after solving\n"
        "  -h          Show this help\n", prog);
}

// Set executable_path global to the directory containing the executable.
// NUMERIC uses this to locate material.prm and undo temp files.
static void init_executable_path(const char* argv0)
{
    namespace fs = std::filesystem;
    std::string exe_dir;

    // Try to resolve from argv[0]
    fs::path exe_path(argv0);
    if (exe_path.has_parent_path()) {
        exe_dir = exe_path.parent_path().string();
    } else {
        // argv[0] has no directory component — use current working directory
        exe_dir = fs::current_path().string();
    }

    // Ensure trailing separator
    if (!exe_dir.empty() && exe_dir.back() != '/' && exe_dir.back() != '\\')
        exe_dir += '/';

    // Copy into the fixed-size global buffer (MAXPATH = 80)
    strncpy(executable_path, exe_dir.c_str(), MAXPATH - 1);
    executable_path[MAXPATH - 1] = '\0';
}

static bool load_materials(const char* material_file)
{
    printf("Loading materials: %s\n", material_file);
    TParseMaterial parser(material_file);
    parser.parse_material();

    if (error_handler.fail()) {
        fprintf(stderr, "Material load failed: %s\n",
                error_handler.get_error_string().c_str());
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    const char* material_file = nullptr;
    const char* output_file = nullptr;
    const char* device_file = nullptr;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            material_file = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 2;
        } else {
            device_file = argv[i];
        }
    }

    if (!device_file) {
        print_usage(argv[0]);
        return 2;
    }

    // 1. Initialize random number generator (as OWL constructor did)
    rnd_init();

    // 2. Set executable_path so NUMERIC can find material.prm and temp files
    init_executable_path(argv[0]);

    // 3. Load material database (must happen before loading any device)
    if (material_file) {
        if (!load_materials(material_file))
            return 1;
    } else {
        // Default: look for material.prm in executable directory
        string default_path = string(executable_path)
                            + preferences.get_material_parameters_file();
        if (!load_materials(default_path.c_str()))
            return 1;
    }

    // 4. Load device file
    printf("Loading device: %s\n", device_file);
    environment.load_file(device_file);

    if (error_handler.fail()) {
        fprintf(stderr, "Device load failed: %s\n",
                error_handler.get_error_string().c_str());
        return 1;
    }

    // 5. Solve
    printf("Solving...\n");
    environment.solve();

    if (error_handler.fail()) {
        fprintf(stderr, "Solve failed: %s\n",
                error_handler.get_error_string().c_str());
        return 1;
    }

    // 6. Write output state file if requested
    if (output_file) {
        printf("Writing state: %s\n", output_file);
        environment.write_state_file(output_file);
        if (error_handler.fail()) {
            fprintf(stderr, "Write failed: %s\n",
                    error_handler.get_error_string().c_str());
            return 1;
        }
    }

    printf("Done.\n");
    return 0;
}
