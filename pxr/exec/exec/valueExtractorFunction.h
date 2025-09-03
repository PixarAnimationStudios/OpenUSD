//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_VALUE_EXTRACTOR_FUNCTION_H
#define PXR_EXEC_EXEC_VALUE_EXTRACTOR_FUNCTION_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/mask.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfVector;
class VtValue;

// Signature of function used to extract values held by execution in VdfVector
// into VtValue.
//
// This is an implementation detail used by ExecTypeRegistry and
// Exec_ValueExtractor as an interface to type-specific extraction code.
//
using Exec_ValueExtractorFunction =
    VtValue (const VdfVector &, const VdfMask::Bits &);

PXR_NAMESPACE_CLOSE_SCOPE

#endif

