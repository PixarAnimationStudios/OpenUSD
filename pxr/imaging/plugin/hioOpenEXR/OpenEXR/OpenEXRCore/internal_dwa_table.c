//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) DreamWorks Animation LLC and Contributors of the OpenEXR Project
//

#include <stdint.h>

extern uint16_t* exrcore_dwaToLinearTable;
extern uint16_t* exrcore_dwaToNonLinearTable;

static uint16_t exrcore_dwaToLinearTable_data[65536];
uint16_t* exrcore_dwaToLinearTable = exrcore_dwaToLinearTable_data;

static uint16_t exrcore_dwaToNonLinearTable_data[65536];
uint16_t* exrcore_dwaToNonLinearTable = exrcore_dwaToNonLinearTable_data;
