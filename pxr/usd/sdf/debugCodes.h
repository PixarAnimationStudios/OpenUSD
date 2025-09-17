//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_DEBUG_CODES_H
#define PXR_USD_SDF_DEBUG_CODES_H

#include "pxr/pxr.h"
#include "pxr/base/tf/debug.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEBUG_CODES(

    SDF_ASSET,
    SDF_CHANGES,
    SDF_FILE_FORMAT,
    SDF_LAYER,
    SDF_VARIABLE_EXPRESSION_PARSING,
    SDF_TEXT_FILE_FORMAT_CONTEXT
);

////////////////////////////////////////////////////////////////////////
// Debugging Symbols for PEGTL Grammer matching tracing.
// These symbols can be turned on at compile time by setting the first
// argument value to `true`.
TF_CONDITIONALLY_COMPILE_TIME_ENABLED_DEBUG_CODES(
    false,
    SDF_TEXT_FILE_FORMAT_PEGTL_TRACE
);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_DEBUG_CODES_H
