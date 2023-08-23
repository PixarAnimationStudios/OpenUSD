//
// Copyright 2022 Pixar
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
