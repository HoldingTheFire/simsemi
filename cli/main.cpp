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
 *   -m <path>                     Material parameter file
 *   -o <path>                     Write state file after solving
 *   -sweep <start>,<end>,<step>   Voltage sweep on contact 0
 *   -csv <path>                   Write sweep results as CSV
 *   -data <path>                  Write spatial profile data after solve
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
#include <cmath>

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
        "  -m <path>                     Material parameter file (default: <exe-dir>/material.prm)\n"
        "  -o <path>                     Write state file after solving\n"
        "  -sweep <start>,<end>,<step>   Voltage sweep applied to contact 0\n"
        "  -csv <path>                   Write sweep results as CSV\n"
        "  -data <path>                  Write spatial profile data after solve\n"
        "  -h                            Show this help\n", prog);
}

// Parse "-sweep start,end,step" into three doubles. Returns true on success.
static bool parse_sweep(const char* arg, double& start, double& end, double& step)
{
    char* p;
    start = strtod(arg, &p);
    if (*p != ',') return false;
    end = strtod(p + 1, &p);
    if (*p != ',') return false;
    step = strtod(p + 1, &p);
    if (*p != '\0') return false;
    if (step == 0.0) return false;
    return true;
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
    const char* sweep_arg = nullptr;
    const char* csv_file = nullptr;
    const char* data_file = nullptr;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            material_file = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-sweep") == 0 && i + 1 < argc) {
            sweep_arg = argv[++i];
        } else if (strcmp(argv[i], "-csv") == 0 && i + 1 < argc) {
            csv_file = argv[++i];
        } else if (strcmp(argv[i], "-data") == 0 && i + 1 < argc) {
            data_file = argv[++i];
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

    // Parse sweep parameters if provided
    double sweep_start = 0, sweep_end = 0, sweep_step = 0;
    if (sweep_arg) {
        if (!parse_sweep(sweep_arg, sweep_start, sweep_end, sweep_step)) {
            fprintf(stderr, "Invalid -sweep format. Expected: -sweep start,end,step\n");
            return 2;
        }
        if ((sweep_end - sweep_start) / sweep_step < 0) {
            fprintf(stderr, "Sweep step sign doesn't match start→end direction.\n");
            return 2;
        }
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

    // --- Voltage sweep mode ---
    if (sweep_arg) {
        // Open CSV file if requested
        FILE* csv_fp = nullptr;
        if (csv_file) {
            csv_fp = fopen(csv_file, "w");
            if (!csv_fp) {
                fprintf(stderr, "Cannot open CSV file: %s\n", csv_file);
                return 1;
            }
            fprintf(csv_fp, "Voltage(V),Current(A/cm2)\n");
        }

        int num_steps = (int)std::round((sweep_end - sweep_start) / sweep_step) + 1;
        printf("Sweep: %.4f to %.4f V, step %.4f (%d points)\n",
               sweep_start, sweep_end, sweep_step, num_steps);

        for (int i = 0; i < num_steps; i++) {
            double V = sweep_start + i * sweep_step;

            // Apply bias to contact 0 (left contact)
            environment.put_value(CONTACT, APPLIED_BIAS, V, 0);
            environment.solve();

            if (error_handler.fail()) {
                fprintf(stderr, "Solve failed at V=%.4f: %s\n",
                        V, error_handler.get_error_string().c_str());
                if (csv_fp) fclose(csv_fp);
                return 1;
            }

            // Read total current from contact 0
            double I = environment.get_value(CONTACT, TOTAL_CURRENT, 0);
            printf("V=%+.4f  I=%.6e A/cm2\n", V, I);

            if (csv_fp)
                fprintf(csv_fp, "%.6f,%.6e\n", V, I);
        }

        if (csv_fp) {
            fclose(csv_fp);
            printf("CSV written: %s\n", csv_file);
        }
    }
    // --- Single-point solve (original behavior) ---
    else {
        printf("Solving...\n");
        environment.solve();

        if (error_handler.fail()) {
            fprintf(stderr, "Solve failed: %s\n",
                    error_handler.get_error_string().c_str());
            return 1;
        }
    }

    // Write output state file if requested
    if (output_file) {
        printf("Writing state: %s\n", output_file);
        environment.write_state_file(output_file);
        if (error_handler.fail()) {
            fprintf(stderr, "Write failed: %s\n",
                    error_handler.get_error_string().c_str());
            return 1;
        }
    }

    // Write spatial profile data if requested
    if (data_file) {
        printf("Writing data: %s\n", data_file);
        TValueFlag write_flags;
        write_flags.set_write_combo(COMBO_RETURN_ALL);
        environment.write_data_file(data_file, write_flags);
        if (error_handler.fail()) {
            fprintf(stderr, "Data write failed: %s\n",
                    error_handler.get_error_string().c_str());
            return 1;
        }
    }

    printf("Done.\n");
    return 0;
}
