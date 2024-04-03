//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
