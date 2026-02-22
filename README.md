# SimWindows

**SimWindows** is a 1D semiconductor device simulator. It models the coupled optical, electrical, and thermal behavior of semiconductor structures, enabling engineers and researchers to define device configurations, run physics simulations, and analyze results.

Originally developed by David W. Winston (Copyright 2013), released under the **GPL-3.0** license. Modernized from Borland C++ / OWL to portable C++17 with CMake, a headless CLI, and a Dear ImGui GUI.

---

## Features

- Define layered semiconductor device structures with configurable materials
- Solve coupled electrical, thermal, and carrier transport equations in 1D
- Model optical cavity modes and photon dynamics
- Analyze device performance (I-V curves, band diagrams, carrier distributions, etc.)
- Voltage sweeps and CSV data export
- Interactive GUI with 16 plot types, or headless CLI for scripting
- State file save/reload for interrupted simulations
- Portable binary packaging (Windows ZIP, Linux TGZ)

---

## Quick Start

### Prerequisites

- **Windows (MSYS2):** Install [MSYS2](https://www.msys2.org/), then in the UCRT64 shell:
  ```bash
  pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-SDL2
  ```
- **Linux:** GCC 10+, CMake 3.16+, Ninja, SDL2 dev libraries
- **macOS:** Clang, CMake, Ninja, SDL2 (`brew install sdl2`)

### Build

```bash
# On MSYS2, ensure ucrt64 is on PATH:
export PATH="/c/msys64/ucrt64/bin:$PATH"

cd simsemi
mkdir -p build && cd build
cmake -G "Ninja" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..
ninja -j$(nproc)
```

This produces:
| Artifact | Size | Description |
|---|---|---|
| `libformulc.a` | 21 KB | Formula parser (static library) |
| `libnumeric.a` | 1.7 MB | Physics engine (static library) |
| `simcli.exe` | 1.4 MB | Headless CLI |
| `simgui.exe` | 10.3 MB | GUI (Dear ImGui + SDL2 + OpenGL) |
| `simtest.exe` | 1.4 MB | Test harness |

### Run a simulation

```bash
# CLI: solve a p-n junction
./simcli -m material.prm ../test/pn_junction.dev -o result.state

# CLI: voltage sweep with CSV output
./simcli -m material.prm ../test/pn_junction.dev -sweep 0 0.5 0.1 -csv iv_curve.csv

# GUI: launch interactive interface
./simgui
```

### Run tests

```bash
./simtest    # 23/23 tests pass
```

### Package for distribution

```bash
cpack    # Creates SimWindows-1.0.0-win64.zip (or .tar.gz on Linux)
```

---

## Project Structure

```
simsemi/
├── CMakeLists.txt          Root build file (C11 + C++17, Ninja)
├── CLAUDE.md               Modernization guide and architecture notes
│
├── NUMERIC/                Physics simulation core (~23,000 LOC)
│   ├── INCLUDE/            30 header files (comincl.h chains all others)
│   ├── devclass.cpp        TDevice — top-level simulator orchestrator
│   ├── envclass.cpp        TEnvironment — environment + material setup
│   ├── elcclass.cpp        TElectrical — electrical solver
│   ├── thrclass.cpp        TThermal — thermal solver
│   ├── carclass.cpp        TCarrier — carrier transport
│   ├── solclass.cpp        TSolution — solution storage
│   ├── parclass.cpp        TParseMaterial — file I/O and parsing
│   └── ...                 16 more physics modules
│
├── Formulc/                Formula parser library (portable C)
│   ├── formulc.c           Parser/evaluator
│   └── formulc.h           C API
│
├── cli/                    Headless CLI application
│   ├── main.cpp            Entry point, argument parsing
│   └── wiofunc_cli.cpp     UI callbacks → stdout
│
├── gui/                    Dear ImGui GUI application
│   ├── main_gui.cpp        SDL2/OpenGL init, main loop
│   ├── app.cpp/h           Application state, solver thread
│   ├── menu.cpp            Menu bar (File, Environment, Device, Plot)
│   ├── dialogs.cpp         All dialogs (Contacts, Models, Optical, etc.)
│   ├── plots.cpp           ImPlot windows with CSV export
│   ├── device_editor.cpp   Text editor for device files
│   ├── wiofunc_gui.cpp     Thread-safe UI callbacks
│   └── vendor/             Vendored: ImGui v1.91.8, ImPlot v0.16, SDL2 backends
│
├── test/                   Test harness and device files
│   ├── test_harness.cpp    23 tests: convergence, round-trip, thesis validation
│   ├── pn_junction.dev     Simple p-n junction
│   ├── thesis_pn_diode.dev Si p-n diode (thesis reference)
│   └── ...                 More test devices
│
├── PROJECT/
│   └── MATERIAL.PRM        Material parameter database (GaAs, Si, InP, etc.)
│
├── Thesis/                 Original thesis PDFs (physics model reference)
│
└── old/                    Deprecated Borland/OWL code (not compiled)
    ├── OWL/                Original Windows GUI (19 files)
    ├── RESOURCE/           OWL bitmaps and .rc resources
    └── Formulc/            Legacy Formulc documentation
```

---

## Device File Format

Device structures are defined in plain text `.dev` files:

```
# Simple GaAs p-n junction
GRID     LENGTH=5e-5  POINTS=50
GRID     LENGTH=5e-5  POINTS=50
DOPING   LENGTH=5e-5  NA=1e17  ND=0
DOPING   LENGTH=5e-5  NA=0     ND=1e17
STRUCTURE LENGTH=1e-4 MATERIAL=GaAs
REGION   LENGTH=1e-4  BULK
```

Keywords: `GRID`, `DOPING`, `STRUCTURE`, `REGION`, `CAVITY`, `MIRROR`. Parameters are `NAME=VALUE` pairs, parsed case-insensitively.

---

## Architecture

The codebase has a clean three-layer architecture:

```
┌─────────────────────────────────────────────┐
│  UI Layer (cli/ or gui/)                    │
│  Implements WIOFUNC.H callbacks             │
├─────────────────────────────────────────────┤
│  NUMERIC — Physics Engine                   │
│  TEnvironment → TDevice → TSolution         │
│  Electrical + Thermal + Optical solvers     │
├─────────────────────────────────────────────┤
│  Formulc — Formula Parser (C)               │
│  Expression evaluation for material models  │
└─────────────────────────────────────────────┘
```

**Key design points:**
- Every NUMERIC `.cpp` includes only `"comincl.h"`, which chains all headers
- The physics engine has **no** UI or platform dependencies
- UI layers implement 8 callback stubs defined in `WIOFUNC.H`
- Custom types: `prec` = `double`, `logical` = `int`, `flag` = `unsigned long`

---

## Tech Stack

| Layer | Technology |
|---|---|
| Language | C++17 (physics + UI), C11 (formula parser) |
| Build | CMake 3.16+ / Ninja |
| Compiler | GCC 10+ (tested with GCC 15.2) |
| GUI | Dear ImGui v1.91.8 + ImPlot v0.16 + SDL2 + OpenGL3 |
| Formula Parser | FORMULC 2.2 (Harald Helfgott) |
| Testing | Custom harness (23 tests: convergence, state round-trip, thesis validation) |
| Packaging | CPack (ZIP / TGZ) |

---

## License

- **SimWindows:** GNU General Public License v3.0 — David W. Winston (2013)
- **Formulc:** Copyright Harald Helfgott (1995)
- **Dear ImGui:** MIT License — Omar Cornut
- **ImPlot:** MIT License — Evan Pezent
