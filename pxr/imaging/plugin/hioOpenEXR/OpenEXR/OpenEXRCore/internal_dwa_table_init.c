//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) DreamWorks Animation LLC and Contributors of the OpenEXR Project
//

#include <stdint.h>

#include "internal_coding.h"
#include "internal_thread.h"

extern uint16_t* exrcore_dwaToLinearTable;
extern uint16_t* exrcore_dwaToNonLinearTable;

static once_flag dwa_tables_once = ONCE_FLAG_INIT;


// Nonlinearly encode luminance. For values below 1.0, we want
// to use a gamma 2.2 function to match what is fairly common
// for storing output referred. However, > 1, gamma functions blow up,
// and log functions are much better behaved. We could use a log
// function everywhere, but it tends to over-sample dark
// regions and undersample the brighter regions, when
// compared to the way real devices reproduce values.
//
// So, above 1, use a log function which is a smooth blend
// into the gamma function.
//
//  Nonlinear(linear) =
//
//    linear^(1./2.2)             / linear <= 1.0
//                               |
//    ln(linear)/ln(e^2.2) + 1    \ otherwise
//
//
// toNonlinear[] needs to take in XDR format half float values,
// and output NATIVE format float.
//
// toLinear[] does the opposite - takes in NATIVE half and
// outputs XDR half values.
//

static inline uint16_t
dwa_convertToLinear (uint16_t x)
{
    if (x == 0)
        return 0;
    if ((x & 0x7c00) == 0x7c00) // infinity/nan?
        return 0;
    
    float f = half_to_float(x);
    float sign = f < 0.0f ? -1.0f : 1.0f;
    f = fabsf(f);
    
    float px, py;
    if (f <= 1.0f)
    {
        px = f;
        py = 2.2f;
    }
    else
    {
        px = 9.02501329156f; // = pow(2.7182818, 2.2)
        py = f - 1.0f;
    }
    float z = sign * powf(px, py);
    return float_to_half(z);
}

static inline uint16_t
dwa_convertToNonLinear (uint16_t x)
{
    if (x == 0)
        return 0;
    if ((x & 0x7c00) == 0x7c00) // infinity/nan?
        return 0;
    
    float f = half_to_float(x);
    float sign = f < 0.0f ? -1.0f : 1.0f;
    f = fabsf(f);
    
    float z;
    if (f <= 1.0f)
    {
        z = powf(f, 1.0f / 2.2f);
    }
    else
    {
        z = logf (f) / 2.2f + 1.0f;
    }
    return float_to_half(sign * z);
}


static void
init_dwa_tables(void)
{
    for (int i = 0; i < 65536; i++)
    {
        exrcore_dwaToLinearTable[i] = dwa_convertToLinear (i);
        exrcore_dwaToNonLinearTable[i] = dwa_convertToNonLinear (i);
    }
}

void
exrcore_ensure_dwa_tables()
{
    call_once (&dwa_tables_once, init_dwa_tables);
}
