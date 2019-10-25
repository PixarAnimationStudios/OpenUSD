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
/// \file AttributeSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/accessorHelpers.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/tf/type.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DEFINE_SPEC(
    SdfSchema, SdfSpecTypeAttribute, SdfAttributeSpec, SdfPropertySpec);

SdfAttributeSpecHandle
SdfAttributeSpec::New(
    const SdfPrimSpecHandle& owner,
    const std::string& name,
    const SdfValueTypeName& typeName,
    SdfVariability variability,
    bool custom)
{
    TRACE_FUNCTION();

    if (!owner) {
	TF_CODING_ERROR("Cannot create an SdfAttributeSpec with a null owner");
	return TfNullPtr;
    }

    if (!Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::IsValidName(name)) {
        TF_CODING_ERROR(
            "Cannot create attribute on %s with invalid name: %s",
            owner->GetPath().GetText(), name.c_str());
        return TfNullPtr;
    }

    SdfPath attributePath = owner->GetPath().AppendProperty(TfToken(name));
    if (!attributePath.IsPropertyPath()) {
        TF_CODING_ERROR(
            "Cannot create attribute at invalid path <%s.%s>",
            owner->GetPath().GetText(), name.c_str());
        return TfNullPtr;
    }

    return _New(owner, attributePath, typeName, variability, custom);
}

SdfAttributeSpecHandle
SdfAttributeSpec::_New(
    const SdfSpecHandle &owner,
    const SdfPath& attrPath,
    const SdfValueTypeName& typeName,
    SdfVariability variability,
    bool custom)
{
    if (!owner) {
        TF_CODING_ERROR("NULL owner");
        return TfNullPtr;
    }
    if (!typeName) {
        TF_CODING_ERROR("Cannot create attribute spec <%s> with invalid type",
                        attrPath.GetText());
        return TfNullPtr;
    }

    const SdfLayerHandle layer = owner->GetLayer();
    if (layer->_ValidateAuthoring()) {
        const SdfValueTypeName typeInSchema = 
            layer->GetSchema().FindType(typeName.GetAsToken().GetString());
        if (!typeInSchema) {
            TF_CODING_ERROR(
                "Cannot create attribute spec <%s> with invalid type",
                attrPath.GetText());
            return TfNullPtr;
        }
    }

    SdfChangeBlock block;

    // AttributeSpecs are considered initially to have only required fields 
    // only if they are not custom.
    bool hasOnlyRequiredFields = (!custom);

    if (!Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::CreateSpec(
            layer, attrPath, SdfSpecTypeAttribute, hasOnlyRequiredFields)) {
        return TfNullPtr;
    }

    SdfAttributeSpecHandle spec = layer->GetAttributeAtPath(attrPath);

    // Avoid expensive dormancy checks
    SdfAttributeSpec *specPtr = get_pointer(spec);
    if (TF_VERIFY(specPtr)) {
        specPtr->SetField(SdfFieldKeys->Custom, custom);
        specPtr->SetField(SdfFieldKeys->TypeName, typeName.GetAsToken());
        specPtr->SetField(SdfFieldKeys->Variability, variability);
    }

    return spec;
}

//
// Connections
//

SdfPath
SdfAttributeSpec::_CanonicalizeConnectionPath(
    const SdfPath& connectionPath) const
{
    // Attribute connection paths are always absolute. If a relative path
    // is passed in, it is considered to be relative to the connection's
    // owning prim.
    return connectionPath.MakeAbsolutePath(GetPath().GetPrimPath());
}

SdfConnectionsProxy
SdfAttributeSpec::GetConnectionPathList() const
{
    return SdfGetPathEditorProxy(
        SdfCreateHandle(this), SdfFieldKeys->ConnectionPaths);
}

bool
SdfAttributeSpec::HasConnectionPaths() const
{
    return GetConnectionPathList().HasKeys();
}

void
SdfAttributeSpec::ClearConnectionPaths()
{
    GetConnectionPathList().ClearEdits();
}

//
// Metadata, Attribute Value API, and Spec Properties
// (methods built on generic SdfSpec accessor macros)
//

// Initialize accessor helper macros to associate with this class and optimize
// out the access predicate
#define SDF_ACCESSOR_CLASS                   SdfAttributeSpec
#define SDF_ACCESSOR_READ_PREDICATE(key_)    SDF_NO_PREDICATE
#define SDF_ACCESSOR_WRITE_PREDICATE(key_)   SDF_NO_PREDICATE

// Attribute Value API

SDF_DEFINE_GET_SET_HAS_CLEAR(AllowedTokens, SdfFieldKeys->AllowedTokens, VtTokenArray)

SDF_DEFINE_GET_SET_HAS_CLEAR(ColorSpace, SdfFieldKeys->ColorSpace, TfToken)

TfEnum
SdfAttributeSpec::GetDisplayUnit() const
{
    // The difference between this and the macro version is that the
    // macro calls _GetValueWithDefault().  That checks if the value
    // is empty and, if so, returns the default value from the schema.
    // But we want to return a default displayUnit that's based on
    // the role.
    TfEnum displayUnit;
    if (HasField(SdfFieldKeys->DisplayUnit, &displayUnit)) {
        return displayUnit;
    }

    return GetTypeName().GetDefaultUnit();
}

TfToken
SdfAttributeSpec::GetRoleName() const
{
    return GetTypeName().GetRole();
}

SDF_DEFINE_SET(DisplayUnit, SdfFieldKeys->DisplayUnit, const TfEnum&)
SDF_DEFINE_HAS(DisplayUnit, SdfFieldKeys->DisplayUnit)
SDF_DEFINE_CLEAR(DisplayUnit, SdfFieldKeys->DisplayUnit)

PXR_NAMESPACE_CLOSE_SCOPE

// Clean up macro shenanigans
#undef SDF_ACCESSOR_CLASS
#undef SDF_ACCESSOR_READ_PREDICATE
#undef SDF_ACCESSOR_WRITE_PREDICATE
