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
#include "pxr/usdImaging/usdviewq/utils.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/treeIterator.h"
#include "pxr/usd/usdGeom/imageable.h"

PXR_NAMESPACE_OPEN_SCOPE


static 
bool _IsA(UsdPrim const& prim, TfType const& schemaType)
{
    // XXX: This method was copied from UsdPrim, we should remove it
    // once UsdPrim::IsA can take TfType as an argument.
    // See http://bug/98391

    // Check Schema TfType                                                       
    if (schemaType.IsUnknown()) {                                                
        TF_CODING_ERROR("Unknown schema type (%s) is invalid for IsA query",
                        schemaType.GetTypeName().c_str());                       
        return false;                                                            
    }                                                                            

    // Get Prim TfType
    const std::string &typeName = prim.GetTypeName().GetString();

    return !typeName.empty() &&                                              
        PlugRegistry::FindDerivedTypeByName<UsdSchemaBase>(typeName).            
        IsA(schemaType); 
}

/*static*/
std::vector<UsdPrim> 
UsdviewqUtils::_GetAllPrimsOfType(UsdStagePtr const &stage, 
                                  TfType const& schemaType)
{
    std::vector<UsdPrim> result;
    for(UsdPrimRange it = stage->Traverse(); it; ++it) {
        if (_IsA(*it, schemaType))
            result.push_back(*it);
    }
    return result;
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (root)
);

/*static*/
UsdviewqUtils::PrimInfo
UsdviewqUtils::GetPrimInfo(UsdPrim prim, UsdTimeCode time)
{
    PrimInfo info;

    info.hasCompositionArcs = (prim.HasAuthoredReferences()    ||
                               prim.HasPayload()               ||
                               prim.HasAuthoredInherits()      ||
                               prim.HasAuthoredSpecializes()   ||
                               prim.HasVariantSets());
    info.isActive = prim.IsActive();
    UsdGeomImageable img(prim);
    info.isImageable = img;
    info.isDefined = prim.IsDefined();
    info.isAbstract = prim.IsAbstract();
    info.isInMaster = prim.IsInMaster();
    info.isInstance = prim.IsInstance();
    info.isVisibilityInherited = false;
    if (img){
        UsdAttributeQuery query(img.GetVisibilityAttr());
        TfToken visibility = UsdGeomTokens->inherited;
        query.Get(&visibility, time);
        info.isVisibilityInherited = (visibility == UsdGeomTokens->inherited);
        info.visVaries = query.ValueMightBeTimeVarying();
    }

    if (prim.GetParent())
        info.name = prim.GetName().GetString();
    else
        info.name = _tokens->root.GetString();
    info.typeName = prim.GetTypeName().GetString();
    
    return info;
}

PXR_NAMESPACE_CLOSE_SCOPE

