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

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <complex>
#include <fstream>
#include <string>
#include <algorithm>
#include <filesystem>

using std::string;
using std::ofstream;
using std::ifstream;
using std::ios;
using std::streampos;
typedef std::complex<double> complex;
using std::real;
using std::imag;
using std::norm;
using std::conj;
using std::exp;

#include "simconst.h"
#include "globfunc.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "formulc.h"
#ifdef __cplusplus
}
#endif

#include "simstrct.h"
#include "wiofunc.h"
#include "simsup.h"
#include "simfunc.h"
#include "simmat.h"
#include "siminf.h"
#include "simenv.h"
