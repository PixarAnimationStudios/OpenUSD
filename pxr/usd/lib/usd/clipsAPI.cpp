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
#include "pxr/usd/usd/clipsAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdClipsAPI,
        TfType::Bases< UsdSchemaBase > >();
    
}

/* virtual */
UsdClipsAPI::~UsdClipsAPI()
{
}

/* static */
UsdClipsAPI
UsdClipsAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (not stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdClipsAPI();
    }
    return UsdClipsAPI(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdClipsAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdClipsAPI>();
    return tfType;
}

/* static */
bool 
UsdClipsAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdClipsAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdClipsAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdSchemaBase::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/usd/tokens.h"

bool 
UsdClipsAPI::SetClipAssetPaths(const VtArray<SdfAssetPath>& assetPaths)
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return GetPrim().SetMetadata(UsdTokens->clipAssetPaths, assetPaths);
}

bool 
UsdClipsAPI::GetClipAssetPaths(VtArray<SdfAssetPath>* assetPaths) const
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return GetPrim().GetMetadata(UsdTokens->clipAssetPaths, assetPaths);
}

bool 
UsdClipsAPI::SetClipManifestAssetPath(const SdfAssetPath& assetPath)
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return GetPrim().SetMetadata(
        UsdTokens->clipManifestAssetPath, assetPath);
}

bool 
UsdClipsAPI::GetClipManifestAssetPath(SdfAssetPath* assetPath) const
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return GetPrim().GetMetadata(
        UsdTokens->clipManifestAssetPath, assetPath);
}

bool 
UsdClipsAPI::SetClipPrimPath(const std::string& primPath)
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return GetPrim().SetMetadata(UsdTokens->clipPrimPath, primPath);
}

bool 
UsdClipsAPI::GetClipPrimPath(std::string* primPath) const
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return GetPrim().GetMetadata(UsdTokens->clipPrimPath, primPath);
}

bool 
UsdClipsAPI::SetClipActive(const VtVec2dArray& activeClips)
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return GetPrim().SetMetadata(UsdTokens->clipActive, activeClips);
}

bool 
UsdClipsAPI::GetClipActive(VtVec2dArray* activeClips) const
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return GetPrim().GetMetadata(UsdTokens->clipActive, activeClips);
}

bool 
UsdClipsAPI::SetClipTimes(const VtVec2dArray& clipTimes)
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return GetPrim().SetMetadata(UsdTokens->clipTimes, clipTimes);
}

bool 
UsdClipsAPI::GetClipTimes(VtVec2dArray* clipTimes) const
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return GetPrim().GetMetadata(UsdTokens->clipTimes, clipTimes);
}
