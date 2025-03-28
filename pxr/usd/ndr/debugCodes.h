//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_NDR_DEBUG_CODES_H
#define PXR_USD_NDR_DEBUG_CODES_H

#include "pxr/pxr.h"
#include "pxr/base/tf/debug.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \note
/// NDR debug codes will be moved to corresponding SDR debug codes

TF_DEBUG_CODES(
    NDR_DISCOVERY,
    NDR_PARSING,
    NDR_INFO,
    NDR_STATS,
    NDR_DEBUG
);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_NDR_DEBUG_CODES_H
