//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TS_DEBUG_CODES_H
#define PXR_BASE_TS_DEBUG_CODES_H

#include "pxr/pxr.h"
#include "pxr/base/tf/debug.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_CONDITIONALLY_COMPILE_TIME_ENABLED_DEBUG_CODES(
    false,
    TS_DEBUG_LOOPS
);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
