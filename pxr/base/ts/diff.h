//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_DIFF_H
#define PXR_BASE_TS_DIFF_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/types.h"

PXR_NAMESPACE_OPEN_SCOPE

struct Ts_SplineData;

/// Compare two splines and return the time interval where they differ.
///
/// The input compareInterval may be infinite. If the splines do not differ, an
/// empty GfInterval is returned.
TS_API
GfInterval
Ts_Diff(const Ts_SplineData* data1,
        const Ts_SplineData* data2,
        const GfInterval& compareInterval);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
