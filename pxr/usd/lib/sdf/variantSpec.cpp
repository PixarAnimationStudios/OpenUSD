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
/// \file VariantSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/variantSpec.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/trace/trace.h"

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

SDF_DEFINE_SPEC(SdfSchema, SdfSpecTypeVariant, SdfVariantSpec, SdfSpec);

SdfVariantSpecHandle
SdfVariantSpec::New(
    const SdfVariantSetSpecHandle& owner, const std::string& name)
{
    TRACE_FUNCTION();

    if (!owner) {
        TF_CODING_ERROR("NULL owner variant set");
        return TfNullPtr;
    }

    if (!SdfSchema::IsValidVariantIdentifier(name)) {
        TF_CODING_ERROR("Invalid variant name: %s", name.c_str());
        return TfNullPtr;
    }

    SdfPath childPath = 
        Sdf_VariantChildPolicy::GetChildPath(owner->GetPath(), TfToken(name));

    SdfLayerHandle layer = owner->GetLayer();
    if (!Sdf_ChildrenUtils<Sdf_VariantChildPolicy>::CreateSpec(
            layer, childPath, SdfSpecTypeVariant)) {
        return TfNullPtr;
    }

    layer->SetField(childPath, SdfFieldKeys->Specifier, SdfSpecifierOver);

    return TfStatic_cast<SdfVariantSpecHandle>(
        layer->GetObjectAtPath(childPath));
}


//
// Name
//

std::string
SdfVariantSpec::GetName() const
{
    return GetPath().GetVariantSelection().second;
}

TfToken
SdfVariantSpec::GetNameToken() const
{
    return TfToken(GetPath().GetVariantSelection().second);
}

//
// Namespace hierarchy
//

SdfVariantSetSpecHandle
SdfVariantSpec::GetOwner() const
{
    SdfPath path = Sdf_VariantChildPolicy::GetParentPath(GetPath());
    return TfStatic_cast<SdfVariantSetSpecHandle>(
        GetLayer()->GetObjectAtPath(path));
}

SdfPrimSpecHandle
SdfVariantSpec::GetPrimSpec() const
{
    return GetLayer()->GetPrimAtPath(GetPath());
}

SdfVariantSetsProxy
SdfVariantSpec::GetVariantSets() const
{
    return SdfVariantSetsProxy(SdfVariantSetView(GetLayer(),
            GetPath(), SdfChildrenKeys->VariantSetChildren),
            "variant sets", SdfVariantSetsProxy::CanErase);
}

std::vector<std::string> 
SdfVariantSpec::GetVariantNames(const std::string& name) const
{
    std::vector<std::string> variantNames;

    SdfPath variantSetPath = GetPath().AppendVariantSelection(name, "");
    std::vector<TfToken> variantNameTokens =
        GetLayer()->GetFieldAs<std::vector<TfToken> >(variantSetPath,
            SdfChildrenKeys->VariantChildren);

    variantNames.reserve(variantNameTokens.size());
    TF_FOR_ALL(i, variantNameTokens) {
        variantNames.push_back(i->GetString());
    }

    return variantNames;
}

SdfVariantSpecHandle
SdfCreateVariantInLayer(
    const SdfLayerHandle &layer,
    const SdfPath &primPath,
    const string &variantSetName,
    const string &variantName )
{
    SdfPath variantPath =
        primPath.AppendVariantSelection(variantSetName, variantName);

    // Create the ancestor prim and variant specs.
    SdfCreatePrimInLayer(layer, variantPath);

    // Look up the resulting variant spec.
    return TfStatic_cast<SdfVariantSpecHandle>(
        layer->GetObjectAtPath(variantPath));
}

PXR_NAMESPACE_CLOSE_SCOPE
