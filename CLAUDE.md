# CLAUDE.md — SimWindows Modernization

This file guides Claude Code sessions working on the SimWindows refactoring project.

## Goal

Port SimWindows from its original Borland C++ / Windows OWL environment to compile and run on a **modern OS** (Linux and/or macOS, with Windows as a secondary target). The physics simulation core (NUMERIC) should be fully preserved — only the platform and UI layers need replacement.

---

## Project Context

- **Language:** C++ (Borland-era, pre-C++11 idioms) and C
- **Original GUI:** OWL (Object Windows Library) — Windows-only, unavailable on modern toolchains
- **Original Build:** Borland C++ IDE — no Makefile, no CMake
- **Physics engine:** `NUMERIC/` — self-contained, no external dependencies, portable C++
- **Formula parser:** `Formulc/` — plain C, already portable

---

## Current Progress

### Phase 1 — Build System: COMPLETE ✓

**Branch:** `modernize/phase1-build-system`

**Result:** Both static libraries build cleanly with GCC 15.2 / C++17 / Ninja on MSYS2 ucrt64:
- `libformulc.a` (21 KB)
- `libnumeric.a` (1.7 MB)

**Build commands:**
```bash
export PATH="/c/msys64/ucrt64/bin:$PATH"
cd /c/GitHub/simsemi && mkdir -p build && cd build
cmake -G "Ninja" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..
ninja -j$(nproc)
```

**What was done (14 files modified, 1 new file):**

1. **`CMakeLists.txt` (NEW)** — Root build file. Builds `formulc` (C11 static lib) + `numeric` (C++17 static lib) with `-Wall -Wextra`.

2. **`NUMERIC/INCLUDE/COMINCL.H`** — Central header included by all 24 NUMERIC source files.
   - Replaced 5 non-standard Borland headers: `<io.h>`, `<alloc.h>`, `<fstream.h>`, `<cstring.h>`, `<complex.h>` → `<cassert>`, `<cmath>`, `<cstdio>`, `<cstdlib>`, `<cstring>`, `<ctime>`, `<complex>`, `<fstream>`, `<string>`, `<algorithm>`, `<filesystem>`
   - Added `using std::` declarations for: `string`, `ofstream`, `ifstream`, `ios`, `streampos`, `complex`, `real`, `imag`, `norm`, `conj`, `exp`
   - Wrapped `#include "formulc.h"` in `extern "C" {}` block (C header included from C++)

3. **`NUMERIC/INCLUDE/GLOBFUNC.H`** — Renamed `round` macro to `sim_round` (conflicts with C++11 `std::round`). Removed non-portable `_matherr` declaration.

4. **`NUMERIC/INCLUDE/SIMENV.H`** — Replaced `access()` with `std::filesystem::exists()`

5. **`NUMERIC/parclass.cpp`** — Replaced 7 Borland string methods:
   - `read_to_delim()` → `std::getline()`
   - `strip(string::Leading)` → `erase(0, find_first_not_of(...))`
   - `to_upper()` → `std::transform(..., ::toupper)`
   - `get_at()` → `operator[]`
   - `remove()` → `erase()`
   - `NPOS` → `std::string::npos`
   - `is_null()` → `empty()`

6. **`NUMERIC/globfunc.cpp`** — Guarded `_matherr` behind `#if defined(__BORLANDC__)`. Replaced `.contains()` → `.find() != npos`

7. **`NUMERIC/funclass.cpp`** — Replaced `NPOS` → `npos`, `get_at()` → `[]`

8. **`NUMERIC/envclass.cpp`** — Replaced `access()` → `std::filesystem::exists()`, `is_null()` → `empty()`

9. **`Formulc/formulc.c`** — Removed 6 deprecated `register` keywords

10. **`NUMERIC/INCLUDE/COMINCL.H`** — Changed `using std::complex` → `typedef std::complex<double> complex` (Borland provided bare `complex` type; modern C++ `std::complex` is a template requiring `<double>`)

11. **`NUMERIC/INCLUDE/GLOBFUNC.H`** + **`NUMERIC/globfunc.cpp`** — Added `extern "C"` to `rnd()`/`rnd_init()` declarations and definitions to resolve C/C++ linkage conflict with formulc.h; fixed `randomize()` → `srand((unsigned)time(nullptr))`

12. **`NUMERIC/INCLUDE/SIMNODE.H`** — Added `friend T2DElectron; friend T2DHole;` (Borland allowed cross-class protected base access that modern C++ disallows)

13. **`NUMERIC/INCLUDE/SIMQW.H`** — Added `friend TBoundElectron; friend TBoundHole;` (same protected access issue)

14. **`NUMERIC/infclass.cpp`** — Fixed wrong-typed null pointer cast `(StructureInput*)0` → `nullptr` for `RegionInput*` member

15. **`NUMERIC/envclass.cpp`** — Fixed `(OpticalParam*)0` → `nullptr` for `OpticalComponent*` members (3 sites)

16. **`NUMERIC/modclass.cpp`** — Fixed Borland implicit-int extension: `static error_sign;` → `static prec error_sign;`

17. **`NUMERIC/parclass.cpp`** — Fixed `rdbuf()->seekoff()` (protected in libstdc++) → `tellg()`/`seekg()` (public stream API)

---

## Modernization Strategy

### Phase 1 — Build System
- [x] Add a `CMakeLists.txt` at the root
- [x] Fix any Borland-specific extensions (non-standard headers, string methods, `register`, `_matherr`, `access()`, implicit-int, seekoff, null pointer casts, C/C++ linkage, complex type, protected base access)
- [x] Get `NUMERIC/` and `Formulc/` compiling cleanly with a modern compiler (GCC 15.2 / C++17) ✓
- [x] Enable warnings (`-Wall -Wextra`) — remaining warnings are `-Wswitch` (unhandled enum values) and `-Wreorder`, not errors
- [x] Target: build a static library from `NUMERIC/` + `Formulc/` — done (`libnumeric.a` 1.7MB, `libformulc.a` 21KB) ✓

### Phase 2 — Headless CLI: COMPLETE ✓

**Result:** `simcli.exe` (1.38 MB) links against `libnumeric.a` + `libformulc.a` and runs end-to-end simulations.

- [x] NUMERIC core has NO Windows API calls, NO OWL dependencies — already portable
- [x] CLI defines NUMERIC globals, implements WIOFUNC.H callbacks as stdout reporters
- [x] Voltage sweep (`-sweep`), CSV export (`-csv`), spatial data export (`-data`)
- [x] End-to-end verified: load materials → load device → solve → write state → reload → re-solve

### Phase 3a — GUI (Minimal Working): COMPLETE ✓

**Result:** `simgui.exe` (10.3 MB) — Dear ImGui + ImPlot + SDL2 + OpenGL3

- [x] Toolkit chosen: Dear ImGui v1.91.8 + ImPlot v0.16 + SDL2 (vendored in `gui/vendor/`)
- [x] Entry point (`gui/main_gui.cpp`): SDL2/OpenGL init, ImGui main loop
- [x] WIOFUNC callbacks → thread-safe log buffer displayed in Status panel
- [x] Menu bar: File (Open/Save), Environment (Load Material, Preferences), Device (Generate/Simulate/Stop/Reset/Contacts/Info), Plot (16 types), Help
- [x] Background solver thread with F5 start / Esc stop
- [x] ImPlot-based plot windows: band diagram (Ec/Ev/Efn/Efp), carrier concentrations, currents, fields, doping, recombination, etc.
- [x] Device text editor panel with Generate button
- [x] Native file dialogs via portable-file-dialogs

### Phase 3b — GUI (Full Feature Parity): COMPLETE ✓

**Result:** All dialogs fully functional with NUMERIC API integration.

- [x] Full contacts dialog: radio buttons (Ideal Ohmic/Finite Recomb/Schottky), recomb velocities, barrier height, computed current display
- [x] Electrical models dialog: 23 checkboxes for all GRID_* effect flags, grouped by category
- [x] Thermal models dialog: device-level temp flags + grid thermal effects + environment temp clamping
- [x] Optical input dialog: enable/disable, spectrum editor with per-component photon energy/intensity, load spectrum file
- [x] Surfaces dialog: temperature editing (lattice/electron/hole), heat sink toggle, thermal conductivity, refractive indices
- [x] Full preferences dialog: all 14 environment parameters (error tolerances, clamp values, relaxation, iteration limits)
- [x] Environment menu: optical input entry; Device menu: thermal models entry
- [x] Export plot data to CSV from plot windows (save dialog per plot)
- [x] Device Info expanded: modes/cavities/spectra counts, solver error display
- [ ] Dockable window layout (ImGui docking branch or manual layout) — deferred

### Phase 4 — Validation & Polish
- [x] Reproduce known-good simulation (p-n junction converges in 4 iterations) ✓
- [x] CLI/headless mode for scripting ✓
- [ ] Compare IV curves against reference data from thesis
- [ ] Write a basic test harness around `TDevice` solve cycles
- [ ] Linux cross-compilation (rename UPPERCASE.H headers, test GCC/Clang on Linux)
- [ ] Package as portable binary (bundle SDL2 DLL, material.prm)

---

## Key Architecture Details

- **Single header chain:** Every NUMERIC `.cpp` file includes only `"comincl.h"`, which chains all other headers
- **Include path:** `NUMERIC/INCLUDE/` and `Formulc/` are the two include directories
- **Case sensitivity:** Header files are UPPERCASE.H on disk but included as lowercase in `#include` directives — works on Windows, will need renaming for Linux cross-compilation
- **Custom types:** `prec` = `double`, `logical` = `int`, `flag` = `unsigned long`, with `TRUE`/`FALSE` macros
- **Complex numbers:** Used as bare `complex` (via `using std::complex`) throughout optical field calculations in `grdclass.cpp`, `surclass.cpp`, `funclass.cpp`, `nodclass.cpp`
- **WIOFUNC.H:** Declares UI callback stubs (`out_error_message`, `out_elect_convergence`, etc.) that must be implemented by any UI layer

## Key Files to Understand First

| File | Why |
|---|---|
| `NUMERIC/INCLUDE/COMINCL.H` | Master include — every source file includes this |
| `NUMERIC/INCLUDE/SIMDEV.H` | Top-level device API — public interface to preserve |
| `NUMERIC/devclass.cpp` | Device orchestration logic |
| `NUMERIC/envclass.cpp` | Largest file; environment + material setup |
| `NUMERIC/INCLUDE/SIMCONST.H` | All constants, typedefs, flag definitions |
| `NUMERIC/INCLUDE/WIOFUNC.H` | UI callback interface that must be implemented |
| `OWL/simwin.cpp` | Entry point; shows how the UI drives `TDevice` |
| `PROJECT/MATERIAL.PRM` | Material parameter file format — must remain compatible |

---

## Coding Conventions (for new code)

- C++17 minimum
- `snake_case` for new free functions and variables; preserve existing class/member names where possible to minimize diff noise
- No platform-specific headers (`<windows.h>`, `<owl/...>`) in `NUMERIC/`
- Prefer `std::` containers and algorithms over manual memory management
- Keep new GUI code strictly separated from physics code (no NUMERIC headers in UI files except through a defined API)

---

## Branch

Active work branch: `modernize/phase1-build-system`

---

## Notes / Gotchas

- The `round` macro was renamed to `sim_round` — it is not actually called anywhere in the codebase, but kept for safety
- `_matherr` is guarded behind `__BORLANDC__` — modern compilers don't support it
- `formulc.h` is wrapped in `extern "C"` in COMINCL.H since it's a C library included from C++
- `FORMULC` uses `setjmp`/`longjmp` for error handling — leave as-is unless it causes issues
- The `.PRM` material file parser lives in `NUMERIC/` and uses custom tokenization — treat as a black box initially
- `stdc++fs` is linked for older GCC versions that need it for `<filesystem>` — may need adjustment for MSYS2
- The audit found NO OWL dependencies, NO Windows API calls, NO Borland typedefs (int16/uint32), and NO inline assembly in the NUMERIC core — it was remarkably portable
