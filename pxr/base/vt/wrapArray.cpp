//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/vt/wrapArray.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace Vt_WrapArray {

// The following bit of preprocessor code produces specializations of
// GetVtArrayName (declared above) for each VtArray type.  The function bodies
// simply return the "common name" for the VtArray.  For instance,
// GetVtArrayName<VtArray<int> >() -> "VtIntArray".
#define MAKE_NAME_FUNC(unused, elem) \
template <> \
VT_API string GetVtArrayName< VT_TYPE(elem) >() { \
    return TF_PP_STRINGIZE(VT_TYPE_NAME(elem)); \
}
TF_PP_SEQ_FOR_EACH(MAKE_NAME_FUNC, ~, VT_ARRAY_VALUE_TYPES)
#undef MAKE_NAME_FUNC


unsigned int
Vt_ComputeEffectiveRankAndLastDimSize(
    Vt_ShapeData const *sd, size_t *lastDimSize)
{
    unsigned int rank = sd->GetRank();
    if (rank == 1)
        return rank;

    size_t divisor = std::accumulate(
        sd->otherDims, sd->otherDims + rank-1,
        1, [](size_t x, size_t y) { return x * y; });

    size_t remainder = divisor ? sd->totalSize % divisor : 0;
    *lastDimSize = divisor ? sd->totalSize / divisor : 0;
    
    if (remainder)
        rank = 1;

    return rank;
}

} // namespace Vt_WrapArray

PXR_NAMESPACE_CLOSE_SCOPE
