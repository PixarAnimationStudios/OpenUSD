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
#include "pxr/usd/usdLux/linkingAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usdGeom/faceSetAPI.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxLinkingAPI,
        TfType::Bases< UsdSchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (collection)
    (includes)
    (excludes)
    (includeByDefault)
);

/* virtual */
UsdLuxLinkingAPI::~UsdLuxLinkingAPI()
{
}

/* static */
UsdLuxLinkingAPI
UsdLuxLinkingAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxLinkingAPI();
    }
    return UsdLuxLinkingAPI(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdLuxLinkingAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxLinkingAPI>();
    return tfType;
}

/* static */
bool 
UsdLuxLinkingAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxLinkingAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdLuxLinkingAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdSchemaBase::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

USDLUX_API
bool
UsdLuxLinkingAPI::DoesLinkPath(const LinkMap &linkMap, const SdfPath &path)
{
    if (!path.IsAbsolutePath()) {
        TF_CODING_ERROR("Path %s must be absolute\n",
                        path.GetText());
        return false;
    }
    // Scan for closest containing opinion
    for (SdfPath p = path; p != SdfPath::EmptyPath(); p = p.GetParentPath()) {
        const LinkMap::const_iterator i = linkMap.find(p);
        if (i != linkMap.end()) {
            return i->second;
        }
    }
    // Any path not explicitly mentioned, and that does not inherit
    // its setting from a prefix path, is included.
    return true;
}

USDLUX_API
SdfPath
UsdLuxLinkingAPI::GetLinkPathForFaceSet(const UsdGeomFaceSetAPI &faceSet)
{
    return faceSet.GetFaceIndicesAttr().GetPath();
}

USDLUX_API
bool
UsdLuxLinkingAPI::DoesLinkFaceSet(const LinkMap &linkMap,
                                  const UsdGeomFaceSetAPI &faceSet )
{
    return DoesLinkPath(linkMap, GetLinkPathForFaceSet(faceSet));
}

UsdLuxLinkingAPI::LinkMap
UsdLuxLinkingAPI::ComputeLinkMap() const
{
    SdfPathVector includes, excludes;
    _GetIncludesRel().GetTargets(&includes);
    _GetExcludesRel().GetTargets(&excludes);
    bool includeByDefault = true;
    _GetIncludeByDefaultAttr().Get(&includeByDefault);

    // Note: An include of path P is stronger than an exclude of P.
    LinkMap result;
    for (const SdfPath &p: excludes) {
        result[p] = false;
    }
    for (const SdfPath &p: includes) {
        result[p] = true;
    }
    if (!includeByDefault) {
        result[SdfPath::AbsoluteRootPath()] = includeByDefault;
    }
    return result;
}

void UsdLuxLinkingAPI::SetLinkMap(const LinkMap &linkMap) const
{
    SdfPathVector includes, excludes;
    bool includeByDefault = true;
    for (const auto &entry: linkMap) {
        if (!entry.first.IsAbsolutePath()) {
            TF_CODING_ERROR("Path %s must be absolute\n",
                            entry.first.GetText());
            return;
        } else if (entry.first == SdfPath::AbsoluteRootPath()) {
            includeByDefault = entry.second;
        } else {
            (entry.second ? includes : excludes).push_back(entry.first);
        }
    }
    _GetIncludesRel().SetTargets(includes);
    _GetExcludesRel().SetTargets(excludes);
    _GetIncludeByDefaultAttr(true).Set(includeByDefault);
}

UsdAttribute
UsdLuxLinkingAPI::_GetIncludeByDefaultAttr(bool create /* = false */) const
{
    const TfToken &attrName =
        _GetCollectionPropertyName(_tokens->includeByDefault);
    if (create) {
        return UsdSchemaBase::_CreateAttr(attrName,
                                          SdfValueTypeNames->Bool,
                                          /* custom = */ false,
                                          SdfVariabilityUniform,
                                          /* default = */ VtValue(),
                                          /* writeSparsely */ false);
    } else {
        return GetPrim().GetAttribute(attrName);
    }
}

UsdRelationship 
UsdLuxLinkingAPI::_GetIncludesRel(bool create /* =false */) const
{
    const TfToken &relName = _GetCollectionPropertyName(_tokens->includes);
    return create ? GetPrim().CreateRelationship(relName, /* custom */ false) :
                    GetPrim().GetRelationship(relName);
}

UsdRelationship 
UsdLuxLinkingAPI::_GetExcludesRel(bool create /* =false */) const
{
    const TfToken &relName = _GetCollectionPropertyName(_tokens->excludes);
    return create ? GetPrim().CreateRelationship(relName, /* custom */ false) :
                    GetPrim().GetRelationship(relName);
}

// Note that we deliberately use a similar backing storage representation
// as UsdGeomCollectionAPI here, with intention to eventually converge.
TfToken 
UsdLuxLinkingAPI::_GetCollectionPropertyName(
    const TfToken &baseName /* =TfToken() */) const
{
    return TfToken(_tokens->collection.GetString() + ":" + 
                   _name.GetString() + 
                   (baseName.IsEmpty() ? "" : (":" + baseName.GetString())));
}


PXR_NAMESPACE_CLOSE_SCOPE
