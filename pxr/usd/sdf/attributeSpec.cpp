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

    SdfAttributeSpecHandle result;

    SdfPrimSpec *ownerPtr = get_pointer(owner);

    if (!ownerPtr) {
	TF_CODING_ERROR("Cannot create an SdfAttributeSpec with a null owner");
	return result;
    }

    const SdfPath attrPath = ownerPtr->GetPath().AppendProperty(TfToken(name));
    if (ARCH_UNLIKELY(attrPath.IsEmpty())) {
        // This can happen if the owner is the pseudo-root '/', or if the passed
        // name was not a valid property name.  Give specific error messages in
        // these cases.
        if (!Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::IsValidName(name)) {
            TF_CODING_ERROR(
                "Cannot create attribute spec on <%s> with invalid name '%s'",
                ownerPtr->GetPath().GetText(), name.c_str());
        }
        else if (ownerPtr->GetPath() == SdfPath::AbsoluteRootPath()) {
            TF_CODING_ERROR(
                "Cannot create attribute spec '%s' on the pseudo-root '/'",
                name.c_str());
        }
        else {
            TF_CODING_ERROR(
                "Cannot create attribute spec '%s' on <%s>",
                name.c_str(), ownerPtr->GetPath().GetText());
        }
        return result;
    }

    if (!typeName) {
        TF_CODING_ERROR("Cannot create attribute spec <%s> with invalid type",
                        attrPath.GetText());
        return result;
    }

    const SdfLayerHandle layer = ownerPtr->GetLayer();
    if (layer->_ValidateAuthoring()) {
        const SdfValueTypeName typeInSchema = 
            layer->GetSchema().FindType(typeName.GetAsToken().GetString());
        if (!typeInSchema) {
            TF_CODING_ERROR(
                "Cannot create attribute spec <%s> with invalid type",
                attrPath.GetText());
            return result;
        }
    }

    SdfChangeBlock block;

    // AttributeSpecs are considered initially to have only required fields 
    // only if they are not custom.
    const bool hasOnlyRequiredFields = (!custom);

    if (!Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::CreateSpec(
            layer, attrPath, SdfSpecTypeAttribute, hasOnlyRequiredFields)) {
        return result;
    }

    result = layer->GetAttributeAtPath(attrPath);

    // Avoid expensive dormancy checks
    SdfAttributeSpec *resultPtr = get_pointer(result);
    if (TF_VERIFY(resultPtr)) {
        resultPtr->SetField(SdfFieldKeys->Custom, custom);
        resultPtr->SetField(SdfFieldKeys->TypeName, typeName.GetAsToken());
        resultPtr->SetField(SdfFieldKeys->Variability, variability);
    }

    return result;
}

//
// Connections
//

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

// Defined in primSpec.cpp.
bool
Sdf_UncheckedCreatePrimInLayer(SdfLayer *layer, SdfPath const &primPath);

bool
SdfJustCreatePrimAttributeInLayer(
    const SdfLayerHandle &layer,
    const SdfPath &attrPath,
    const SdfValueTypeName &typeName,
    SdfVariability variability,
    bool isCustom)
{
    if (!attrPath.IsPrimPropertyPath()) {
        TF_CODING_ERROR("Cannot create prim attribute at path '%s' because "
                        "it is not a prim property path",
                        attrPath.GetText());
        return false;
    }

    SdfLayer *layerPtr = get_pointer(layer);

    SdfChangeBlock block;

    if (!Sdf_UncheckedCreatePrimInLayer(layerPtr, attrPath.GetParentPath())) {
        return false;
    }

    if (!Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::CreateSpec(
            layer, attrPath, SdfSpecTypeAttribute,
            /*hasOnlyRequiredFields=*/!isCustom)) {
        TF_RUNTIME_ERROR("Failed to create attribute at path '%s' in "
                         "layer @%s@", attrPath.GetText(),
                         layerPtr->GetIdentifier().c_str());
        return false;
    }
    
    layerPtr->SetField(attrPath, SdfFieldKeys->Custom, isCustom);
    layerPtr->SetField(attrPath, SdfFieldKeys->TypeName, typeName.GetAsToken());
    layerPtr->SetField(attrPath, SdfFieldKeys->Variability, variability);
    
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
