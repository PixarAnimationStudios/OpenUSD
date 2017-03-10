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
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define< UsdSchemaBase >();
}

UsdSchemaBase::UsdSchemaBase(const UsdPrim& prim) 
    : _primData(prim._Prim())
    , _primPath(_primData ? _primData->GetPath() : SdfPath())
{
    /* NOTHING */
}

UsdSchemaBase::UsdSchemaBase(const UsdSchemaBase& schema) 
    : _primData(schema._primData)
    , _primPath(schema._primPath)
{
    /* NOTHING YET */
}

/*virtual*/
UsdSchemaBase::~UsdSchemaBase()
{
    // This only exists to avoid memory leaks in derived classes which may
    // define new members.
}

// Forward decl helper in SchemaRegistry.cpp
SdfPrimSpecHandle
Usd_SchemaRegistryGetPrimDefinitionAtPath(SdfPath const &path);
SdfPrimSpecHandle
UsdSchemaBase::GetSchemaClassPrimDefinition() const
{
    return UsdSchemaRegistry::GetPrimDefinition(_GetType());
}

bool
UsdSchemaBase::_IsCompatible(const UsdPrim &prim) const
{
    // By default, schema objects are compatible.
    return true;
}

TF_MAKE_STATIC_DATA(TfType, _tfType) {
    *_tfType = TfType::Find<UsdSchemaBase>();
}
const TfType &
UsdSchemaBase::_GetTfType() const
{
    return *_tfType;
}

UsdAttribute
UsdSchemaBase::_CreateAttr(TfToken const &attrName,
                           SdfValueTypeName const & typeName,
                           bool custom, SdfVariability variability,
                           VtValue const &defaultValue, 
                           bool writeSparsely) const
{
    UsdPrim prim(GetPrim());
    
    if (writeSparsely && !custom){
        // We are a builtin, and we're trying to be parsimonious.
        // We only need to even CREATE a propertySpec if we are
        // authoring a non-fallback default value
        UsdAttribute attr = prim.GetAttribute(attrName);
        VtValue  fallback;
        if (defaultValue.IsEmpty() ||
            (!attr.HasAuthoredValueOpinion()
             && attr.Get(&fallback)
             && fallback == defaultValue)){
            return attr;
        }
    }
    
    UsdAttribute attr(prim.CreateAttribute(attrName, typeName,
                                           custom, variability));
    if (attr && !defaultValue.IsEmpty()) {
        attr.Set(defaultValue);
    }

    return attr;
}

PXR_NAMESPACE_CLOSE_SCOPE

