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
/// \file Sdf/fileIO.cpp


#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/fileIO_Common.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/atomicOfstreamWrapper.h"
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tracelite/trace.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/stringUtils.h"

#include <fstream>
#include <sstream>

using std::map;
using std::ostream;
using std::string;
using std::vector;

////////////////////////////////////////////////////////////////////////
// Local shorthand

#define _Write             Sdf_FileIOUtility::Write
#define _WriteQuotedString Sdf_FileIOUtility::WriteQuotedString
#define _WriteAssetPath    Sdf_FileIOUtility::WriteAssetPath
#define _WriteSdfPath      Sdf_FileIOUtility::WriteSdfPath

////////////////////////////////////////////////////////////////////////
// Layer Content Support

bool
Sdf_WriteVariant(const SdfVariantSpec &variantSpec,
    std::ostream &out, size_t indent)
{
    SdfPrimSpec primSpec = variantSpec.GetPrimSpec().GetSpec();
    _WriteQuotedString(out, indent, variantSpec.GetName());

    Sdf_WritePrimMetadata( primSpec, out, indent );

    _Write(out, 0, " {\n");

    Sdf_WritePrimBody( primSpec, out, indent );

    _Write(out, 0, "\n");
    _Write(out, indent, "}\n");

    return true;
}

static
bool
_VariantNameLess(const SdfVariantSpecHandle& a, const SdfVariantSpecHandle& b)
{
    return a->GetName() < b->GetName();
}

bool
Sdf_WriteVariantSet(const SdfVariantSetSpec &spec, ostream &out, size_t indent)
{
    SdfVariantSpecHandleVector variants = spec.GetVariantList();
    std::sort(variants.begin(), variants.end(), &::_VariantNameLess);

    if (!variants.empty()) {
        _Write(out, indent, "variantSet ");
        _WriteQuotedString(out, 0, spec.GetName());
        _Write(out, 0, " = {\n");
        TF_FOR_ALL(it, variants) {
            (*it)->WriteToStream(out, indent+1);
        }
        _Write(out, indent, "}\n");
    }
    return true;
}

bool
Sdf_WriteToStream(const SdfSpec &baseSpec, std::ostream& out, size_t indent)
{
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
