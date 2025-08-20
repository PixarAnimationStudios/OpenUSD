//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_DEBUG_CODES_H
#define PXR_EXEC_VDF_DEBUG_CODES_H

/// \file

#include "pxr/pxr.h"

#include "pxr/base/tf/debug.h"

PXR_NAMESPACE_OPEN_SCOPE

/// TF_DEBUG codes for Vdf execution.
///
TF_DEBUG_CODES(
    VDF_MUNG_BUFFER_LOCKING,
    VDF_SPARSE_INPUT_PATH_FINDER,
    VDF_SCHEDULING
);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
