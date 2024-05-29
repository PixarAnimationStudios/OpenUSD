//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
////////////////////////////////////////////////////////////////////////

#ifndef PXR_USD_IMAGING_USD_IMAGING_EXTENTS_HINT_SCHEMA_H
#define PXR_USD_IMAGING_USD_IMAGING_EXTENTS_HINT_SCHEMA_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/extentSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

//-----------------------------------------------------------------------------

#define USDIMAGINGEXTENTSHINT_SCHEMA_TOKENS \
    (extentsHint)

TF_DECLARE_PUBLIC_TOKENS(UsdImagingExtentsHintSchemaTokens, USDIMAGING_API,
    USDIMAGINGEXTENTSHINT_SCHEMA_TOKENS);

class UsdImagingExtentsHintSchema : public HdSchema
{
public:
    UsdImagingExtentsHintSchema(HdContainerDataSourceHandle container)
    : HdSchema(container) {}

    USDIMAGING_API
    static HdContainerDataSourceHandle
    BuildRetained(
        size_t count,
        const TfToken *names,
        const HdDataSourceBaseHandle *values);

    USDIMAGING_API
    HdExtentSchema GetExtent(const TfToken &purpose);

    USDIMAGING_API
    static UsdImagingExtentsHintSchema GetFromParent(
        const HdContainerDataSourceHandle &fromParentContainer);

    /// Returns a token where the container representing this schema is found in
    /// a container by default.
    USDIMAGING_API
    static const TfToken &GetSchemaToken();

    /// Returns an HdDataSourceLocator (relative to the prim-level data source)
    /// where the container representing this schema is found by default.
    USDIMAGING_API
    static const HdDataSourceLocator &GetDefaultLocator();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
