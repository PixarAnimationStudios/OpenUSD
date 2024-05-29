//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_STAGE_GLOBALS_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_STAGE_GLOBALS_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/imaging/hd/dataSourceLocator.h"

#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingDataSourceStageGlobals
///
/// This class is used as a context object with global stage information,
/// that gets passed down to datasources to help them answer scene queries.
///
/// It's a pure virtual interface to allow for different use cases to override
/// certain behaviors (like getting the time coordinate, or whether we support
/// time-varying tracking).
/// 
class UsdImagingDataSourceStageGlobals
{
public:
    USDIMAGING_API
    virtual ~UsdImagingDataSourceStageGlobals();

    // Datasource API

    /// Returns the current time represented in this instance.
    virtual UsdTimeCode GetTime() const = 0;

    /// Flags the given \p hydraPath as time varying at the given locator.
    virtual void FlagAsTimeVarying(
        const SdfPath &hydraPath, 
        const HdDataSourceLocator &locator) const = 0;

    /// Flags the object at \p usdPath as dependent on an asset path.
    /// \p usdPath may point to a prim (e.g., if the prim has asset path
    /// metadata) or an attribute (e.g., if the attribute has an
    /// asset path value).
    virtual void FlagAsAssetPathDependent(
        const SdfPath &usdPath) const = 0;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_STAGE_GLOBALS_H
