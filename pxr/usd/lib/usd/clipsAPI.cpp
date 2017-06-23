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

PXR_NAMESPACE_OPEN_SCOPE

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
    if (!stage) {
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

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/usd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_CLIPS_API_SETTER(FnName, InArg, MetadataKey)        \
    if (GetPath() == SdfPath::AbsoluteRootPath()) {             \
        /* Special-case to pre-empt coding errors. */           \
        return false;                                           \
    }                                                           \
    return GetPrim().SetMetadata(MetadataKey, InArg);           \

#define USD_CLIPS_API_GETTER(FnName, OutArg, MetadataKey)       \
    if (GetPath() == SdfPath::AbsoluteRootPath()) {             \
        /* Special-case to pre-empt coding errors.  */          \
        return false;                                           \
    }                                                           \
    return GetPrim().GetMetadata(MetadataKey, OutArg);          \

bool 
UsdClipsAPI::SetClipAssetPaths(const VtArray<SdfAssetPath>& assetPaths)
{
    USD_CLIPS_API_SETTER(SetClipAssetPaths, 
        assetPaths, UsdTokens->clipAssetPaths);
}

bool 
UsdClipsAPI::GetClipAssetPaths(VtArray<SdfAssetPath>* assetPaths) const
{
    USD_CLIPS_API_GETTER(GetClipAssetPaths,
        assetPaths, UsdTokens->clipAssetPaths);
}

bool 
UsdClipsAPI::SetClipManifestAssetPath(const SdfAssetPath& assetPath)
{
    USD_CLIPS_API_SETTER(SetClipManifestAssetPath,
        assetPath, UsdTokens->clipManifestAssetPath);
}

bool 
UsdClipsAPI::GetClipManifestAssetPath(SdfAssetPath* assetPath) const
{
    USD_CLIPS_API_GETTER(GetClipManifestAssetPath,
        assetPath, UsdTokens->clipManifestAssetPath);
}

bool 
UsdClipsAPI::SetClipPrimPath(const std::string& primPath)
{
    USD_CLIPS_API_SETTER(SetClipPrimPath,
        primPath, UsdTokens->clipPrimPath);
}

bool 
UsdClipsAPI::GetClipPrimPath(std::string* primPath) const
{
    USD_CLIPS_API_GETTER(GetClipPrimPath,
        primPath, UsdTokens->clipPrimPath);
}

bool 
UsdClipsAPI::SetClipActive(const VtVec2dArray& activeClips)
{
    USD_CLIPS_API_SETTER(SetClipActive,
        activeClips, UsdTokens->clipActive);
}

bool 
UsdClipsAPI::GetClipActive(VtVec2dArray* activeClips) const
{
    USD_CLIPS_API_GETTER(GetClipActive,
        activeClips, UsdTokens->clipActive);
}

bool 
UsdClipsAPI::SetClipTimes(const VtVec2dArray& clipTimes)
{
    USD_CLIPS_API_SETTER(SetClipTimes,
        clipTimes, UsdTokens->clipTimes);
}

bool 
UsdClipsAPI::GetClipTimes(VtVec2dArray* clipTimes) const
{
    USD_CLIPS_API_GETTER(GetClipTimes,
        clipTimes, UsdTokens->clipTimes);
}

bool 
UsdClipsAPI::GetClipTemplateAssetPath(std::string* clipTemplateAssetPath) const
{
    USD_CLIPS_API_GETTER(GetClipTemplateAssetPath,
        clipTemplateAssetPath, UsdTokens->clipTemplateAssetPath);
}

bool 
UsdClipsAPI::SetClipTemplateAssetPath(const std::string& clipTemplateAssetPath)
{
    USD_CLIPS_API_SETTER(SetClipTemplateAssetPath,
        clipTemplateAssetPath, UsdTokens->clipTemplateAssetPath);
}

bool 
UsdClipsAPI::GetClipTemplateStride(double* clipTemplateStride) const
{
    USD_CLIPS_API_GETTER(GetClipTemplateStride,
        clipTemplateStride, UsdTokens->clipTemplateStride);
}

bool 
UsdClipsAPI::SetClipTemplateStride(const double clipTemplateStride)
{
    if (clipTemplateStride == 0) {
        TF_CODING_ERROR("clipTemplateStride can not be set to 0.");
        return false;
    }

    USD_CLIPS_API_SETTER(SetClipTemplateStride,
        clipTemplateStride, UsdTokens->clipTemplateStride);
}

bool 
UsdClipsAPI::GetClipTemplateStartTime(double* clipTemplateStartTime) const
{
    USD_CLIPS_API_GETTER(GetClipTemplateStartTime,
        clipTemplateStartTime, UsdTokens->clipTemplateStartTime);
}

bool 
UsdClipsAPI::SetClipTemplateStartTime(const double clipTemplateStartTime)
{
    USD_CLIPS_API_SETTER(SetClipTemplateStartTime,
        clipTemplateStartTime, UsdTokens->clipTemplateStartTime);
}

bool 
UsdClipsAPI::GetClipTemplateEndTime(double* clipTemplateEndTime) const
{
    USD_CLIPS_API_GETTER(GetClipTemplateEndTime,
        clipTemplateEndTime, UsdTokens->clipTemplateEndTime);
}

bool 
UsdClipsAPI::SetClipTemplateEndTime(const double clipTemplateEndTime)
{
    USD_CLIPS_API_SETTER(SetClipTemplateEndTime,
        clipTemplateEndTime, UsdTokens->clipTemplateEndTime);
}

bool
UsdClipsAPI::ClearTemplateClipMetadata()
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    auto prim = GetPrim();
    prim.ClearMetadata(UsdTokens->clipTemplateAssetPath);
    prim.ClearMetadata(UsdTokens->clipTemplateStride);
    prim.ClearMetadata(UsdTokens->clipTemplateEndTime);
    prim.ClearMetadata(UsdTokens->clipTemplateStartTime);

    return true;
}

bool
UsdClipsAPI::ClearNonTemplateClipMetadata()
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    auto prim = GetPrim();
    prim.ClearMetadata(UsdTokens->clipAssetPaths);
    prim.ClearMetadata(UsdTokens->clipTimes);
    prim.ClearMetadata(UsdTokens->clipActive);

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
