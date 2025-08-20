//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/cacheView.h"

#include "pxr/exec/execUsd/request.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

VtValue
ExecUsdCacheView::Get(int index) const
{
    // In the future, to support executor bypass for attribute values that do
    // not require computation, index may need to be remapped into the range of
    // _view.  Currently, the index range mapping is always one-to-one.

    return _view.Get(index);
}

PXR_NAMESPACE_CLOSE_SCOPE
