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
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdAPISchemaBase,
        TfType::Bases< UsdSchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (APISchemaBase)
);

/* virtual */
UsdAPISchemaBase::~UsdAPISchemaBase()
{
}

/* static */
UsdAPISchemaBase
UsdAPISchemaBase::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdAPISchemaBase();
    }
    return UsdAPISchemaBase(stage->GetPrimAtPath(path));
}


/* static */
UsdAPISchemaBase
UsdAPISchemaBase::_Apply(const UsdPrim &prim)
{
    return UsdAPISchemaBase::_ApplyAPISchema<UsdAPISchemaBase>(
            prim, _schemaTokens->APISchemaBase);
}

/* static */
const TfType &
UsdAPISchemaBase::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdAPISchemaBase>();
    return tfType;
}

/* static */
bool 
UsdAPISchemaBase::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdAPISchemaBase::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdAPISchemaBase::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdSchemaBase::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

PXR_NAMESPACE_OPEN_SCOPE

/* static */
UsdPrim 
UsdAPISchemaBase::_ApplyAPISchemaImpl(const UsdPrim &prim, 
                                      const TfToken &apiName)
{
    if (!prim) {
        TF_CODING_ERROR("Invalid prim.");
        return prim;
    }

    if (prim.IsInstanceProxy() || prim.IsInMaster()) {
        TF_CODING_ERROR("Prim at <%s> is an instance proxy or is inside an "
            "instance master.", prim.GetPath().GetText());
        return UsdPrim();
    }

    // Get the listop at the current edit target.
    UsdStagePtr stage = prim.GetStage();
    UsdEditTarget editTarget = stage->GetEditTarget();
    SdfPrimSpecHandle primSpec = editTarget.GetPrimSpecForScenePath(
        prim.GetPath());
    if (!primSpec) {
        // This should create the primSpec in the current edit target. 
        // It will also issue an error if it's unable to.
        primSpec = stage->_CreatePrimSpecForEditing(prim);

        // _CreatePrimSpecForEditing would have already issued a runtime error 
        // in case of a failure. This can happen when an ancestor path is 
        // inactive on the stage or when trying to author directly to a proxy 
        // or master prim within an instance, or if the given path is not 
        // reachable within the current edit target.
        if (!primSpec) {
            TF_WARN("Unable to create primSpec at path <%s> in edit target "
                "'%s'. Failed to apply API schema '%s' on the prim.",
                prim.GetPath().GetText(), 
                editTarget.GetLayer()->GetIdentifier().c_str(),
                apiName.GetText());
            return prim;
        }
    }

    SdfTokenListOp listOp = 
        primSpec->GetInfo(UsdTokens->apiSchemas).UncheckedGet<SdfTokenListOp>();

    // Append our name to the prepend list, if it doesnt exist locally.
    TfTokenVector existingApiSchemas = listOp.IsExplicit() ? 
            listOp.GetExplicitItems() : listOp.GetPrependedItems();

    if (std::find(existingApiSchemas.begin(), existingApiSchemas.end(), apiName) 
            != existingApiSchemas.end()) { 
        return prim;
    }

    existingApiSchemas.push_back(apiName);

    SdfTokenListOp prependListOp;    
    prependListOp.SetPrependedItems(existingApiSchemas);
    
    if (auto result = listOp.ApplyOperations(prependListOp)) {
        // Set the listop on the primSpec at the current edit target and return 
        // the prim
        primSpec->SetInfo(UsdTokens->apiSchemas, VtValue(*result));
        return prim; 
    } else {
        TF_CODING_ERROR("Failed to prepend api name %s to 'apiSchemas' listOp "
            "at path <%s>", apiName.GetText(), prim.GetPath().GetText());
        return UsdPrim();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
