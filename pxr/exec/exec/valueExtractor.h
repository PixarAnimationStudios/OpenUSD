//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_VALUE_EXTRACTOR_H
#define PXR_EXEC_EXEC_VALUE_EXTRACTOR_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"
#include "pxr/exec/exec/valueExtractorFunction.h"

#include "pxr/exec/vdf/mask.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfVector;

/// Converts a VdfVector to a VtValue.
///
/// Value extraction is the process of returning computed values stored in
/// VdfVector to clients that consume VtValue.
///
class Exec_ValueExtractor
{
public:

    /// Constructs an invalid extractor.
    Exec_ValueExtractor() = default;

    /// Construct an extractor that invokes \p func.
    explicit Exec_ValueExtractor(const Exec_ValueExtractorFunction &func)
        : _func(func)
    {
    }

    /// Returns a VtValue holding the elements of \p v corresponding to the set
    /// bits of \p mask.
    VtValue operator()(const VdfVector &v, const VdfMask &mask) const {
        return _func(v, mask.GetBits());
    }

    /// Returns true if this extractor can extract values.
    explicit operator bool() const {
        return _func != &_ExtractInvalid;
    }

private:

    // Posts an error and returns an empty VtValue.
    EXEC_API
    static VtValue _ExtractInvalid(const VdfVector &, const VdfMask::Bits &);

private:
    const Exec_ValueExtractorFunction *_func = &_ExtractInvalid;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
