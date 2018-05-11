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
#include "pxr/pxr.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_MAKE_STATIC_DATA(TfType, _schemaBaseTfType) {
    *_schemaBaseTfType = TfType::Find<UsdSchemaBase>();
    TF_VERIFY(!_schemaBaseTfType->IsUnknown());
}

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdTyped, TfType::Bases<UsdSchemaBase> >();
    TfType::AddAlias<UsdSchemaBase, UsdTyped>("Typed");
}

UsdTyped::~UsdTyped()
{
}

/* static */
UsdTyped
UsdTyped::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTyped();
    }
    return UsdTyped(stage->GetPrimAtPath(path));
}


bool
UsdTyped::_IsCompatible() const
{
    if (!UsdSchemaBase::_IsCompatible())
        return false;

    // A typed prim must have a non-empty typeName for it to be compatible.
    const std::string &typeName = GetPrim().GetTypeName().GetString();
    if (typeName.empty()) {
        return false;
    }

    // Typed schemas are compatible if the prim's type is a subtype of this
    // schema object's type.
    if (!_schemaBaseTfType->FindDerivedByName(typeName).IsA(_GetType())) {
        return false;
    }

    return true;
}

TF_MAKE_STATIC_DATA(TfType, _tfType) {
    *_tfType = TfType::Find<UsdTyped>();
}
const TfType &
UsdTyped::_GetTfType() const
{
    return *_tfType;
}

PXR_NAMESPACE_CLOSE_SCOPE

