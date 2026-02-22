/*
    SimWindows - 1D Semiconductor Device Simulator
    Test Harness — Phase 4 Validation

    Tests:
      1. P-N junction convergence (basic smoke test)
      2. State save/reload round-trip
      3. Thesis Test 1: Si pn diode depletion field
      4. Thesis Test 2: GaAs photodetector short-circuit current (stub)
      5. Voltage sweep produces monotonic IV curve
*/

#include "comincl.h"
#include "simparse.h"
#include "strtable.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <filesystem>

// NUMERIC globals
TEnvironment      environment;
TMaterialStorage  material_parameters;
TErrorHandler     error_handler;
TPreferences      preferences;
NormalizeConstants normalization;

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

static void check(bool condition, const char* name, const char* detail = nullptr)
{
    tests_run++;
    if (condition) {
        tests_passed++;
        printf("  PASS: %s\n", name);
    } else {
        tests_failed++;
        printf("  FAIL: %s", name);
        if (detail) printf(" -- %s", detail);
        printf("\n");
    }
}

static void init_executable_path(const char* argv0)
{
    namespace fs = std::filesystem;
    std::string exe_dir;
    fs::path exe_path(argv0);
    if (exe_path.has_parent_path())
        exe_dir = exe_path.parent_path().string();
    else
        exe_dir = fs::current_path().string();
    if (!exe_dir.empty() && exe_dir.back() != '/' && exe_dir.back() != '\\')
        exe_dir += '/';
    strncpy(executable_path, exe_dir.c_str(), MAXPATH - 1);
    executable_path[MAXPATH - 1] = '\0';
}

static bool load_materials(const char* path)
{
    TParseMaterial parser(path);
    parser.parse_material();
    return !error_handler.fail();
}

static bool load_and_solve(const char* device_path)
{
    environment.load_file(device_path);
    if (error_handler.fail()) return false;
    environment.solve();
    return !error_handler.fail();
}

// =========================================================================
// Test 1: Basic p-n junction convergence
// =========================================================================
static void test_pn_junction_convergence(const char* device_path)
{
    printf("\n--- Test 1: P-N Junction Convergence ---\n");

    bool ok = load_and_solve(device_path);
    check(ok, "Device loads and solves without error");

    if (!ok) return;

    int nodes = environment.get_number_objects(NODE);
    check(nodes > 0, "Device has grid nodes");

    int contacts = environment.get_number_objects(CONTACT);
    check(contacts == 2, "Device has 2 contacts");

    // Check solver errors are small
    double err_psi = environment.get_value(DEVICE, ERROR_PSI, 0);
    double err_ec  = environment.get_value(DEVICE, ERROR_ETA_C, 0);
    double err_ev  = environment.get_value(DEVICE, ERROR_ETA_V, 0);

    char buf[128];
    snprintf(buf, sizeof(buf), "err_psi=%.2e", err_psi);
    check(err_psi < 1e-3, "Potential error < 1e-3", buf);

    snprintf(buf, sizeof(buf), "err_ec=%.2e", err_ec);
    check(err_ec < 1e-3, "Electron quasi-Fermi error < 1e-3", buf);

    snprintf(buf, sizeof(buf), "err_ev=%.2e", err_ev);
    check(err_ev < 1e-3, "Hole quasi-Fermi error < 1e-3", buf);

    // At equilibrium (0V bias), current should be very small
    double total_j = environment.get_value(CONTACT, TOTAL_CURRENT, 0);
    snprintf(buf, sizeof(buf), "J=%.2e A/cm2", total_j);
    check(fabs(total_j) < 1e-6, "Equilibrium current ~0", buf);
}

// =========================================================================
// Test 2: State save/reload round-trip
// =========================================================================
static void test_state_roundtrip(const char* device_path)
{
    printf("\n--- Test 2: State Save/Reload Round-Trip ---\n");

    bool ok = load_and_solve(device_path);
    check(ok, "Initial solve succeeds");
    if (!ok) return;

    // Read current before save
    double j_before = environment.get_value(CONTACT, TOTAL_CURRENT, 0);
    double ec_mid = environment.get_value(ELECTRON, BAND_EDGE,
                        environment.get_number_objects(NODE) / 2);

    // Save state
    namespace fs = std::filesystem;
    std::string tmp_state = fs::temp_directory_path().string() + "/simtest_roundtrip.sta";
    environment.write_state_file(tmp_state.c_str());
    check(!error_handler.fail(), "State file written");

    // Reload state
    environment.load_file(tmp_state.c_str());
    check(!error_handler.fail(), "State file reloaded");

    // Re-solve (should converge in ~1 iteration since already at solution)
    environment.solve();
    check(!error_handler.fail(), "Re-solve after reload succeeds");

    double err_psi = environment.get_value(DEVICE, ERROR_PSI, 0);
    char buf[128];
    snprintf(buf, sizeof(buf), "err_psi=%.2e", err_psi);
    check(err_psi < 1e-6, "Re-solve error very small (already converged)", buf);

    // Check values match
    double j_after = environment.get_value(CONTACT, TOTAL_CURRENT, 0);
    double ec_mid_after = environment.get_value(ELECTRON, BAND_EDGE,
                              environment.get_number_objects(NODE) / 2);

    snprintf(buf, sizeof(buf), "before=%.6e after=%.6e", j_before, j_after);
    double j_diff = (j_before != 0.0) ? fabs((j_after - j_before) / j_before) : fabs(j_after);
    check(j_diff < 0.01 || (fabs(j_before) < 1e-10 && fabs(j_after) < 1e-10),
          "Current matches after reload", buf);

    snprintf(buf, sizeof(buf), "before=%.6f after=%.6f", ec_mid, ec_mid_after);
    check(fabs(ec_mid - ec_mid_after) < 0.001, "Band edge matches after reload", buf);

    // Clean up
    std::remove(tmp_state.c_str());
}

// =========================================================================
// Test 3: Thesis Test 1 — Si pn diode depletion field
// =========================================================================
static void test_thesis_depletion_field(const char* device_path)
{
    printf("\n--- Test 3: Thesis Depletion Field (Si pn diode) ---\n");

    bool ok = load_and_solve(device_path);
    check(ok, "Thesis pn diode loads and solves");
    if (!ok) return;

    int nodes = environment.get_number_objects(NODE);
    check(nodes > 0, "Has grid nodes");

    // Find peak (most negative) electric field in the depletion region
    double peak_field = 0;
    for (int i = 0; i < nodes; i++) {
        double field = environment.get_value(GRID_ELECTRICAL, FIELD, i);
        if (field < peak_field)
            peak_field = field;
    }

    char buf[128];
    snprintf(buf, sizeof(buf), "peak=%.2e V/cm (expect ~-1.3e5)", peak_field);

    // Thesis Figure 43: peak field is approximately -1.3e5 V/cm
    // Allow 20% tolerance for numerical vs analytical comparison
    check(peak_field < -1.0e5 && peak_field > -1.6e5,
          "Peak E-field within 20% of analytical (-1.3e5 V/cm)", buf);

    // Field should be near zero at the edges (neutral regions)
    double field_left = environment.get_value(GRID_ELECTRICAL, FIELD, 0);
    double field_right = environment.get_value(GRID_ELECTRICAL, FIELD, nodes - 1);

    snprintf(buf, sizeof(buf), "left=%.2e right=%.2e", field_left, field_right);
    check(fabs(field_left) < fabs(peak_field) * 0.1 &&
          fabs(field_right) < fabs(peak_field) * 0.1,
          "Field near zero at edges (neutral regions)", buf);
}

// =========================================================================
// Test 4: Voltage sweep produces monotonic IV
// =========================================================================
static void test_voltage_sweep(const char* device_path)
{
    printf("\n--- Test 4: Voltage Sweep (forward bias IV) ---\n");

    environment.load_file(device_path);
    check(!error_handler.fail(), "Device loads for sweep");
    if (error_handler.fail()) return;

    // Solve at equilibrium first
    environment.solve();
    check(!error_handler.fail(), "Equilibrium solve");
    if (error_handler.fail()) return;

    // Forward bias sweep: 0V to 0.4V in 0.1V steps
    // Current should increase monotonically
    double voltages[] = {0.0, 0.1, 0.2, 0.3, 0.4};
    double currents[5] = {};
    bool all_solved = true;

    for (int i = 0; i < 5; i++) {
        environment.put_value(CONTACT, APPLIED_BIAS, voltages[i], 0);
        environment.solve();
        if (error_handler.fail()) {
            all_solved = false;
            break;
        }
        currents[i] = environment.get_value(CONTACT, TOTAL_CURRENT, 0);
    }

    check(all_solved, "All sweep points solved");
    if (!all_solved) return;

    char buf[256];
    for (int i = 0; i < 5; i++) {
        snprintf(buf, sizeof(buf), "V=%.1f: J=%.4e A/cm2", voltages[i], currents[i]);
        printf("    %s\n", buf);
    }

    // Check monotonic increase in forward bias current
    bool monotonic = true;
    for (int i = 1; i < 5; i++) {
        if (currents[i] <= currents[i-1]) {
            monotonic = false;
            break;
        }
    }
    check(monotonic, "Current increases monotonically with forward bias");

    // Check exponential-like growth: I(0.4V) >> I(0.0V)
    snprintf(buf, sizeof(buf), "I(0.4V)/I(0V)=%.1e",
             (currents[0] != 0) ? currents[4]/currents[0] : currents[4]);
    check(currents[4] > currents[0] * 100,
          "Current at 0.4V >> current at 0V (diode behavior)", buf);
}

// =========================================================================
// Main
// =========================================================================
int main(int argc, char* argv[])
{
    printf("SimWindows Test Harness\n");
    printf("=======================\n");

    rnd_init();
    init_executable_path(argv[0]);

    // Determine paths
    namespace fs = std::filesystem;
    std::string exe_dir(executable_path);

    // Find material file
    std::string mat_path = exe_dir + "material.prm";
    if (!fs::exists(mat_path)) {
        // Try relative to source tree
        mat_path = exe_dir + "../PROJECT/MATERIAL.PRM";
    }
    if (!fs::exists(mat_path)) {
        fprintf(stderr, "Cannot find material.prm. Copy PROJECT/MATERIAL.PRM to build dir.\n");
        return 1;
    }

    printf("Material file: %s\n", mat_path.c_str());
    if (!load_materials(mat_path.c_str())) {
        fprintf(stderr, "Failed to load materials: %s\n",
                error_handler.get_error_string().c_str());
        return 1;
    }
    printf("Materials loaded.\n");

    // Find test device files
    std::string test_dir;
    if (fs::exists(exe_dir + "../test/pn_junction.dev"))
        test_dir = exe_dir + "../test/";
    else if (fs::exists("../test/pn_junction.dev"))
        test_dir = "../test/";
    else if (fs::exists("test/pn_junction.dev"))
        test_dir = "test/";
    else {
        fprintf(stderr, "Cannot find test/ directory.\n");
        return 1;
    }

    printf("Test directory: %s\n", test_dir.c_str());

    // Run tests
    std::string pn_path = test_dir + "pn_junction.dev";
    std::string thesis_pn_path = test_dir + "thesis_pn_diode.dev";

    test_pn_junction_convergence(pn_path.c_str());
    test_state_roundtrip(pn_path.c_str());

    if (fs::exists(thesis_pn_path))
        test_thesis_depletion_field(thesis_pn_path.c_str());

    test_voltage_sweep(pn_path.c_str());

    // Summary
    printf("\n=======================\n");
    printf("Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0)
        printf(" (%d FAILED)", tests_failed);
    printf("\n");

    return tests_failed > 0 ? 1 : 0;
}
