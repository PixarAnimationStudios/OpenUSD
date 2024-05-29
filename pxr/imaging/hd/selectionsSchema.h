//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HD_SELECTIONS_SCHEMA_H
#define PXR_IMAGING_HD_SELECTIONS_SCHEMA_H

#include "pxr/imaging/hd/api.h"

#include "pxr/imaging/hd/vectorSchema.h" 
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSelectionSchema;

//-----------------------------------------------------------------------------

#define HDSELECTIONS_SCHEMA_TOKENS \
    (selections) \

TF_DECLARE_PUBLIC_TOKENS(HdSelectionsSchemaTokens, HD_API,
    HDSELECTIONS_SCHEMA_TOKENS);

//-----------------------------------------------------------------------------

class HdSelectionsSchema : public HdSchemaBasedVectorSchema<HdSelectionSchema>
{
public:
    HdSelectionsSchema(HdVectorDataSourceHandle const &vector)
      : HdSchemaBasedVectorSchema(vector) {}

    /// Retrieves a vector data source with the schema's default name token
    /// "selections" from the parent vector and constructs a
    /// HdSelectionsSchema instance.
    /// Because the requested vector data source may not exist, the result
    /// should be checked with IsDefined() or a bool comparison before use.
    HD_API
    static HdSelectionsSchema GetFromParent(
        const HdContainerDataSourceHandle &fromParentContainer);

    /// Returns a token where the container representing this schema is found in
    /// a container by default.
    HD_API
    static const TfToken &GetSchemaToken();

    /// Returns an HdDataSourceLocator (relative to the prim-level data source)
    /// where the vector data source representing this schema is found by
    /// default.
    HD_API
    static const HdDataSourceLocator &GetDefaultLocator();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
