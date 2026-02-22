/*
    SimWindows - 1D Semiconductor Device Simulator
    Copyright (C) 2013 David W. Winston

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

/*
 * wiofunc_gui.cpp -- GUI implementation of the WIOFUNC.H callbacks.
 * Writes to the app log buffer instead of stdout.
 */

#include "comincl.h"
#include "app.h"
#include <cstdio>

extern SimWindowsApp app;

void out_message(string message)
{
    app.add_log(message);
}

void out_error_message(logical clear_error)
{
    if (!clear_error && error_handler.fail())
        app.add_log("ERROR: " + error_handler.get_error_string());
}

void out_elect_convergence(short iterations, FundamentalParam error)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "  elec iter %3d  psi=%.3e  eta_c=%.3e  eta_v=%.3e",
             (int)iterations, error.psi, error.eta_c, error.eta_v);
    app.add_log(buf);
}

void out_optic_convergence(short iterations, prec error)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "  optic iter %3d  err=%.3e", (int)iterations, error);
    app.add_log(buf);
}

void out_therm_convergence(short iterations, prec error)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "  therm iter %3d  err=%.3e", (int)iterations, error);
    app.add_log(buf);
}

void out_coarse_mode_convergence(short iterations, prec error)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "  coarse-mode iter %3d  err=%.3e", (int)iterations, error);
    app.add_log(buf);
}

void out_fine_mode_convergence(short iterations, prec error)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "  fine-mode iter %3d  err=%.3e", (int)iterations, error);
    app.add_log(buf);
}

void out_operating_condition(void)
{
    app.add_log("Operating condition updated.");
}

void out_simulation_result(void)
{
    app.add_log("Simulation complete.");
}
