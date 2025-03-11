//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_UTILS_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_UTILS_H

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

template<typename T>
auto UsdSkelImagingGetTypedValue(
    const T &ds,
    const HdSampledDataSource::Time shutterOffset = 0.0f)
        -> decltype(ds->GetTypedValue(0.0f))
{
    if (!ds) {
        return {};
    }
    return ds->GetTypedValue(0.0f);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
