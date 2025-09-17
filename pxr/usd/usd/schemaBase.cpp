//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    , _proxyPrimPath(prim._ProxyPrimPath())
{
    /* NOTHING */
}

UsdSchemaBase::UsdSchemaBase(const UsdSchemaBase& schema) 
    : _primData(schema._primData)
    , _proxyPrimPath(schema._proxyPrimPath)
{
    /* NOTHING YET */
}

/*virtual*/
UsdSchemaBase::~UsdSchemaBase()
{
    // This only exists to avoid memory leaks in derived classes which may
    // define new members.
}

const UsdPrimDefinition *
UsdSchemaBase::GetSchemaClassPrimDefinition() const
{
    const UsdSchemaRegistry &reg = UsdSchemaRegistry::GetInstance();
    const TfToken usdTypeName = reg.GetSchemaTypeName(_GetType());
    return IsAppliedAPISchema() ?
        reg.FindAppliedAPIPrimDefinition(usdTypeName) :
        reg.FindConcretePrimDefinition(usdTypeName);
}

bool
UsdSchemaBase::_IsCompatible() const
{
    // By default, schema objects are compatible with any valid prim.
    return true;
}

/* static */
const TfType &
UsdSchemaBase::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdSchemaBase>();
    return tfType;
}

const TfType &
UsdSchemaBase::_GetTfType() const
{
    return _GetStaticTfType();
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
            (!attr.HasAuthoredValue()
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

