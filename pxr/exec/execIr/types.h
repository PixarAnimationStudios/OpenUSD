//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_IR_TYPES_H
#define PXR_EXEC_EXEC_IR_TYPES_H

#include "pxr/pxr.h"

#include "pxr/base/tf/denseHashMap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Map used to return results from a controller inverse compute function.
///
/// Keys are the names of invertible input attributes and values are the values
/// the inputs must assume in order to produce the desired output values that
/// were specified.
///
using ExecIrInversionResult =
    TfDenseHashMap<TfToken, VtValue, TfToken::HashFunctor>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
