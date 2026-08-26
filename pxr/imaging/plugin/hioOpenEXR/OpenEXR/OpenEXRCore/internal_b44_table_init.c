//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) DreamWorks Animation LLC and Contributors of the OpenEXR Project
//

#include <stdint.h>

#include "internal_coding.h"
#include "internal_thread.h"

extern uint16_t* exrcore_expTable;
extern uint16_t* exrcore_logTable;

static once_flag b44_tables_once = ONCE_FLAG_INIT;

static inline uint16_t
b44_convertFromLinear (uint16_t x)
{
    if ((x & 0x7c00) == 0x7c00) // infinity/nan?
        return 0;
    if (x >= 0x558c && x < 0x8000) // >= 8 * log (HALF_MAX)
        return 0x7bff;             // HALF_MAX

    float f = half_to_float (x);
    f       = expf (f / 8);
    return float_to_half (f);
}

static inline uint16_t
b44_convertToLinear (uint16_t x)
{
    if ((x & 0x7c00) == 0x7c00) // infinity/nan?
        return 0;
    if (x > 0x8000) // negative? (excluding -0.0 which is accepted)
        return 0;

    float f = half_to_float (x);
    f       = 8 * logf (f);
    return float_to_half (f);
}


static void
init_b44_tables(void)
{
    for (int i = 0; i < 65536; i++)
    {
        exrcore_expTable[i] = b44_convertFromLinear (i);
        exrcore_logTable[i] = b44_convertToLinear (i);
    }
}

void
exrcore_ensure_b44_tables()
{
    call_once (&b44_tables_once, init_b44_tables);
}
