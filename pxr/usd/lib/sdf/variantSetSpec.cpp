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
/// \file VariantSetSpec.cpp

#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/variantSpec.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tracelite/trace.h"
#include <ostream>

SDF_DEFINE_SPEC(SdfVariantSetSpec, SdfSpec);

SdfVariantSetSpecHandle 
SdfVariantSetSpec::New(const SdfPrimSpecHandle& owner, const std::string& name)
{
    TRACE_FUNCTION();

    if (!owner) {
        TF_CODING_ERROR("NULL owner prim");
        return TfNullPtr;
    }

    if (!Sdf_ChildrenUtils<Sdf_VariantSetChildPolicy>::IsValidName(name)) {
        TF_CODING_ERROR("Cannot create variant set spec with invalid "
            "identifier: '%s'", name.c_str());
        return TfNullPtr;
    }

    SdfChangeBlock block;

    SdfLayerHandle layer = owner->GetLayer();
    SdfPath path = owner->GetPath().AppendVariantSelection(name, "");

    if (!path.IsPrimVariantSelectionPath()) {
        TF_CODING_ERROR("Cannot create variant set spec at invalid "
            "path <%s{%s=}>", owner->GetPath().GetText(), name.c_str());
        return TfNullPtr;
    }

    if (!Sdf_ChildrenUtils<Sdf_VariantSetChildPolicy>::CreateSpec(
            layer, path, SdfSpecTypeVariantSet))
        return TfNullPtr;
    
    return TfStatic_cast<SdfVariantSetSpecHandle>(layer->GetObjectAtPath(path));
}

SdfVariantSetSpecHandle 
SdfVariantSetSpec::New(const SdfVariantSpecHandle& owner,
                       const std::string& name)
{
    TRACE_FUNCTION();

    if (!owner) {
        TF_CODING_ERROR("NULL owner variant");
        return TfNullPtr;
    }

    if (!Sdf_ChildrenUtils<Sdf_VariantSetChildPolicy>::IsValidName(name)) {
        TF_CODING_ERROR("Cannot create variant set spec with invalid "
            "identifier: '%s'", name.c_str());
        return TfNullPtr;
    }

    SdfChangeBlock block;

    SdfLayerHandle layer = owner->GetLayer();
    SdfPath path = owner->GetPath().AppendVariantSelection(name, "");

    if (!path.IsPrimVariantSelectionPath()) {
        TF_CODING_ERROR("Cannot create variant set spec at invalid "
            "path <%s{%s=}>", owner->GetPath().GetText(), name.c_str());
        return TfNullPtr;
    }

    if (!Sdf_ChildrenUtils<Sdf_VariantSetChildPolicy>::CreateSpec(
            layer, path, SdfSpecTypeVariantSet))
        return TfNullPtr;
    
    return TfStatic_cast<SdfVariantSetSpecHandle>(layer->GetObjectAtPath(path));
}

//
// Name
//

std::string
SdfVariantSetSpec::GetName() const
{ 
    return GetPath().GetName();
}

TfToken
SdfVariantSetSpec::GetNameToken() const
{ 
    return GetPath().GetNameToken();
}

//
// Namespace hierarchy
//

SdfSpecHandle
SdfVariantSetSpec::GetOwner() const
{
    return GetLayer()->GetObjectAtPath(GetPath().GetParentPath());
}

//
// Variants
//

SdfVariantView
SdfVariantSetSpec::GetVariants() const
{
    return SdfVariantView(
        GetLayer(), GetPath(), 
        SdfChildrenKeys->VariantChildren);
}

SdfVariantSpecHandleVector
SdfVariantSetSpec::GetVariantList() const 
{
    return GetVariants().values();
}

void
SdfVariantSetSpec::RemoveVariant(const SdfVariantSpecHandle& variant)
{
    const SdfLayerHandle& layer = GetLayer();
    const SdfPath& path = GetPath();

    SdfPath parentPath = 
        Sdf_VariantChildPolicy::GetParentPath(variant->GetPath());
    if (variant->GetLayer() != layer || parentPath != path) {
        TF_CODING_ERROR("Cannot remove a variant that does not belong to "
            "this variant set.");
        return;
    }

    if (!Sdf_ChildrenUtils<Sdf_VariantChildPolicy>::RemoveChild(
            layer, path, variant->GetNameToken())) {
        TF_CODING_ERROR("Unable to remove child: %s",
            variant->GetName().c_str());
    }
}
