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
 * wiofunc_cli.cpp — Headless (CLI) implementation of the WIOFUNC.H callbacks.
 *
 * WIOFUNC.H declares 8 functions that the NUMERIC physics engine calls during
 * simulation to report progress and results.  This file implements them for a
 * plain-text terminal; a future GUI layer will supply its own version instead.
 */

#include "comincl.h"
#include <cstdio>

// Report a general status message.
void out_message(string message)
{
    printf("%s\n", message.c_str());
}

// Report (or clear) an error from error_handler.
// Called with clear_error=TRUE to dismiss a previous error, FALSE to display.
void out_error_message(logical clear_error)
{
    if (!clear_error && error_handler.fail())
        printf("ERROR: %s\n", error_handler.get_error_string().c_str());
}

// Electrical solver convergence per iteration.
void out_elect_convergence(short iterations, FundamentalParam error)
{
    printf("  elec iter %3d  psi=%.3e  eta_c=%.3e  eta_v=%.3e\n",
           (int)iterations, error.psi, error.eta_c, error.eta_v);
}

// Optical solver convergence per iteration.
void out_optic_convergence(short iterations, prec error)
{
    printf("  optic iter %3d  err=%.3e\n", (int)iterations, error);
}

// Thermal solver convergence per iteration.
void out_therm_convergence(short iterations, prec error)
{
    printf("  therm iter %3d  err=%.3e\n", (int)iterations, error);
}

// Coarse optical-mode solver convergence.
void out_coarse_mode_convergence(short iterations, prec error)
{
    printf("  coarse-mode iter %3d  err=%.3e\n", (int)iterations, error);
}

// Fine optical-mode solver convergence.
void out_fine_mode_convergence(short iterations, prec error)
{
    printf("  fine-mode iter %3d  err=%.3e\n", (int)iterations, error);
}

// Print current bias/temperature operating condition.
void out_operating_condition(void)
{
    printf("Operating condition updated.\n");
}

// Simulation finished successfully.
void out_simulation_result(void)
{
    printf("Simulation complete.\n");
}
