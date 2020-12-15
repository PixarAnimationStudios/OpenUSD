//
// Copyright 2016 Pixar
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
///
/// \file sdf/fileIO.cpp

#include "pxr/pxr.h"

#include "pxr/usd/sdf/fileIO.h"

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/fileIO_Common.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"

#include "pxr/base/tf/stringUtils.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

#if AR_VERSION != 1

Sdf_StreamWritableAsset::~Sdf_StreamWritableAsset() = default;

#endif

bool
Sdf_WriteToStream(const SdfSpec &baseSpec, std::ostream& o, size_t indent)
{
    Sdf_TextOutput out(o);

    const SdfSpecType type = baseSpec.GetSpecType();

    switch (type) {
    case SdfSpecTypePrim:
        {
            SdfPrimSpec spec =
                Sdf_CastAccess::CastSpec<SdfPrimSpec, SdfSpec>(baseSpec);
            return Sdf_WritePrim(spec, out, indent);
        }
    case SdfSpecTypeAttribute:
        {
            SdfAttributeSpec spec =
                Sdf_CastAccess::CastSpec<SdfAttributeSpec, SdfSpec>(baseSpec);
            return Sdf_WriteAttribute(spec, out, indent);
        }
    case SdfSpecTypeRelationship:
        {
            SdfRelationshipSpec spec =
                Sdf_CastAccess::CastSpec<SdfRelationshipSpec, SdfSpec>(baseSpec);
            return Sdf_WriteRelationship(spec, out, indent);
        }
    case SdfSpecTypeVariantSet:
        {
            SdfVariantSetSpec spec =
                Sdf_CastAccess::CastSpec<SdfVariantSetSpec, SdfSpec>(baseSpec);
            return Sdf_WriteVariantSet(spec, out, indent);
        }
    case SdfSpecTypeVariant:
        {
            SdfVariantSpec spec =
                Sdf_CastAccess::CastSpec<SdfVariantSpec, SdfSpec>(baseSpec);
            return Sdf_WriteVariant(spec, out, indent);
        }
    default:
        break;
    }

    TF_CODING_ERROR("Cannot write spec of type %s to stream", 
                    TfStringify(type).c_str());
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
