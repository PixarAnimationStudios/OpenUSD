//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_IR_TYPES_H
#define PXR_EXEC_EXEC_IR_TYPES_H

/// \file

#include "pxr/pxr.h"

#include "pxr/base/tf/denseHashMap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Map used to return results from controller forward and inverse computations.
///
/// The map entries are the names of attributes for which the computation
/// produces values and the corresponding computed values.
///
using ExecIrResult = TfDenseHashMap<TfToken, VtValue, TfToken::HashFunctor>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
