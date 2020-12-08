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
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdClipsAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (ClipsAPI)
);

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


/* virtual */
UsdSchemaKind UsdClipsAPI::_GetSchemaKind() const {
    return UsdClipsAPI::schemaKind;
}

/* virtual */
UsdSchemaKind UsdClipsAPI::_GetSchemaType() const {
    return UsdClipsAPI::schemaType;
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
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

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

#include "pxr/usd/usd/clipSet.h"
#include "pxr/usd/usd/clipSetDefinition.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/sdf/layerUtils.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdClipsAPIInfoKeys, USDCLIPS_INFO_KEYS);
TF_DEFINE_PUBLIC_TOKENS(UsdClipsAPISetNames, USDCLIPS_SET_NAMES);

namespace
{

TfToken
_MakeKeyPath(const std::string& clipSet, const TfToken& clipInfoKey)
{
    return TfToken(clipSet + ":" + clipInfoKey.GetString());
}

bool
_ComputeClipSetDefinition(
    const UsdPrim& prim, const std::string& clipSet,
    Usd_ClipSetDefinition* clipSetDef)
{
    std::vector<Usd_ClipSetDefinition> clipSetDefs;
    std::vector<std::string> clipSetNames;
    Usd_ComputeClipSetDefinitionsForPrimIndex(
        prim.GetPrimIndex(), &clipSetDefs, &clipSetNames);

    auto it = std::find(clipSetNames.begin(), clipSetNames.end(), clipSet);
    if (it == clipSetNames.end()) {
        TF_CODING_ERROR("No clip set named '%s'", clipSet.c_str());
        return false;
    }

    const size_t clipDefIndex = std::distance(clipSetNames.begin(), it);
    if (!TF_VERIFY(clipDefIndex < clipSetDefs.size())) {
        return false;
    }

    *clipSetDef = clipSetDefs[clipDefIndex];
    return true;
}

}

#define USD_CLIPS_API_SETTER(FnName, InArg, MetadataKey)        \
    if (GetPath() == SdfPath::AbsoluteRootPath()) {             \
        /* Special-case to pre-empt coding errors. */           \
        return false;                                           \
    }                                                           \
    return FnName(InArg, UsdClipsAPISetNames->default_);        \

#define USD_CLIPS_API_GETTER(FnName, OutArg, MetadataKey)       \
    if (GetPath() == SdfPath::AbsoluteRootPath()) {             \
        /* Special-case to pre-empt coding errors.  */          \
        return false;                                           \
    }                                                           \
    return FnName(OutArg, UsdClipsAPISetNames->default_);       \

#define USD_CLIPS_API_CLIPSET_SETTER(FnName, InArg, ClipSetArg, InfoKey) \
    if (GetPath() == SdfPath::AbsoluteRootPath()) {                      \
        /* Special-case to pre-empt coding errors. */                    \
        return false;                                                    \
    }                                                                    \
    if (ClipSetArg.empty()) {                                            \
        TF_CODING_ERROR("Empty clip set name not allowed");              \
        return false;                                                    \
    }                                                                    \
    if (!TfIsValidIdentifier(ClipSetArg)) {                              \
        TF_CODING_ERROR("Clip set name must be a valid identifier "      \
                        "(got '%s')", ClipSetArg.c_str());               \
        return false;                                                    \
    }                                                                    \
    return GetPrim().SetMetadataByDictKey(                               \
        UsdTokens->clips, _MakeKeyPath(ClipSetArg, InfoKey), InArg);

#define USD_CLIPS_API_CLIPSET_GETTER(FnName, OutArg, ClipSetArg, InfoKey) \
    if (GetPath() == SdfPath::AbsoluteRootPath()) {                       \
        /* Special-case to pre-empt coding errors.  */                    \
        return false;                                                     \
    }                                                                     \
    if (ClipSetArg.empty()) {                                             \
        TF_CODING_ERROR("Empty clip set name not allowed");               \
        return false;                                                     \
    }                                                                     \
    if (!TfIsValidIdentifier(ClipSetArg)) {                               \
        TF_CODING_ERROR("Clip set name must be a valid identifier "       \
                        "(got '%s')", ClipSetArg.c_str());                \
        return false;                                                     \
    }                                                                     \
    return GetPrim().GetMetadataByDictKey(                                \
        UsdTokens->clips, _MakeKeyPath(ClipSetArg, InfoKey), OutArg);

bool 
UsdClipsAPI::GetClips(VtDictionary* clips) const
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }
    return GetPrim().GetMetadata(UsdTokens->clips, clips);
}

bool 
UsdClipsAPI::SetClips(const VtDictionary& clips)
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }
    return GetPrim().SetMetadata(UsdTokens->clips, clips);
}

bool 
UsdClipsAPI::GetClipSets(SdfStringListOp* clipSets) const
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    const SdfPrimSpecHandle primSpec = GetPrim().GetStage()->GetEditTarget()
        .GetPrimSpecForScenePath(GetPath());
    return primSpec->HasField(UsdTokens->clipSets, clipSets);
}

bool 
UsdClipsAPI::SetClipSets(const SdfStringListOp& clipSets)
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }
    return GetPrim().SetMetadata(UsdTokens->clipSets, clipSets);
}

bool 
UsdClipsAPI::SetClipAssetPaths(const VtArray<SdfAssetPath>& assetPaths)
{
    USD_CLIPS_API_SETTER(SetClipAssetPaths, 
        assetPaths, UsdTokens->clipAssetPaths);
}

bool 
UsdClipsAPI::SetClipAssetPaths(const VtArray<SdfAssetPath>& assetPaths,
                               const std::string& clipSet)
{
    USD_CLIPS_API_CLIPSET_SETTER(SetClipAssetPaths, 
        assetPaths, clipSet, UsdClipsAPIInfoKeys->assetPaths);
}

bool 
UsdClipsAPI::GetClipAssetPaths(VtArray<SdfAssetPath>* assetPaths) const
{
    USD_CLIPS_API_GETTER(GetClipAssetPaths,
        assetPaths, UsdTokens->clipAssetPaths);
}

VtArray<SdfAssetPath>
UsdClipsAPI::ComputeClipAssetPaths(const std::string& clipSet) const
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return {};
    }

    Usd_ClipSetDefinition clipSetDef;
    if (!_ComputeClipSetDefinition(GetPrim(), clipSet, &clipSetDef)
        || !clipSetDef.clipAssetPaths) {
        return {};
    }

    // Anchor and resolve each path in the clipAssetPaths specified in
    // the definition. 
    ArResolverScopedCache resolverScopedCache;
    auto& resolver = ArGetResolver();

    const SdfLayerRefPtr& sourceLayer =
        clipSetDef.sourceLayerStack->GetLayers()[
            clipSetDef.indexOfLayerWhereAssetPathsFound];
    const ArResolverContextBinder binder(
        clipSetDef.sourceLayerStack->GetIdentifier().pathResolverContext);

    for (SdfAssetPath& p : *clipSetDef.clipAssetPaths) {
        const std::string anchoredPath = SdfComputeAssetPathRelativeToLayer(
            sourceLayer, p.GetAssetPath());
        const std::string resolvedPath = resolver.Resolve(anchoredPath);

        if (!resolvedPath.empty()) {
            p = SdfAssetPath(p.GetAssetPath(), resolvedPath);
        }
    }

    return *clipSetDef.clipAssetPaths;
}

VtArray<SdfAssetPath>
UsdClipsAPI::ComputeClipAssetPaths() const
{
    return ComputeClipAssetPaths(UsdClipsAPISetNames->default_);
}

bool 
UsdClipsAPI::GetClipAssetPaths(VtArray<SdfAssetPath>* assetPaths,
                               const std::string& clipSet) const
{
    USD_CLIPS_API_CLIPSET_GETTER(GetClipAssetPaths,
        assetPaths, clipSet, UsdClipsAPIInfoKeys->assetPaths);
}

bool 
UsdClipsAPI::SetClipManifestAssetPath(const SdfAssetPath& assetPath)
{
    USD_CLIPS_API_SETTER(SetClipManifestAssetPath,
        assetPath, UsdTokens->clipManifestAssetPath);
}

bool 
UsdClipsAPI::SetClipManifestAssetPath(const SdfAssetPath& assetPath,
                                      const std::string& clipSet)
{
    USD_CLIPS_API_CLIPSET_SETTER(SetClipManifestAssetPath,
        assetPath, clipSet, UsdClipsAPIInfoKeys->manifestAssetPath);
}

bool 
UsdClipsAPI::GetClipManifestAssetPath(SdfAssetPath* assetPath) const
{
    USD_CLIPS_API_GETTER(GetClipManifestAssetPath,
        assetPath, UsdTokens->clipManifestAssetPath);
}

bool 
UsdClipsAPI::GetClipManifestAssetPath(SdfAssetPath* assetPath,
                                      const std::string& clipSet) const
{
    USD_CLIPS_API_CLIPSET_GETTER(GetClipManifestAssetPath,
        assetPath, clipSet, UsdClipsAPIInfoKeys->manifestAssetPath);
}

SdfLayerRefPtr
UsdClipsAPI::GenerateClipManifest(
    const std::string& clipSetName,
    bool writeBlocksForClipsWithMissingValues) const
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return SdfLayerRefPtr();
    }

    Usd_ClipSetDefinition clipSetDef;
    if (!_ComputeClipSetDefinition(GetPrim(), clipSetName, &clipSetDef)) {
        return SdfLayerRefPtr();
    }

    std::string err;
    Usd_ClipSetRefPtr clipSet = Usd_ClipSet::New(clipSetName, clipSetDef, &err);
    if (!clipSet) {
        if (!err.empty()) {
            TF_CODING_ERROR(
                "Invalid clips in clip set '%s': %s", 
                clipSetName.c_str(), err.c_str());
        }
        return SdfLayerRefPtr();
    }

    return Usd_GenerateClipManifest(
        clipSet->valueClips, clipSet->clipPrimPath,
        /* tag = */ std::string(),
        writeBlocksForClipsWithMissingValues);
}

SdfLayerRefPtr
UsdClipsAPI::GenerateClipManifest(
    bool writeBlocksForClipsWithMissingValues) const
{
    return GenerateClipManifest(
        UsdClipsAPISetNames->default_, writeBlocksForClipsWithMissingValues);
}

SdfLayerRefPtr
UsdClipsAPI::GenerateClipManifestFromLayers(
    const SdfLayerHandleVector& clipLayers, 
    const SdfPath& clipPrimPath)
{
    return Usd_GenerateClipManifest(clipLayers, clipPrimPath);
}

bool
UsdClipsAPI::SetInterpolateMissingClipValues(bool interpolate)
{
    USD_CLIPS_API_SETTER(SetInterpolateMissingClipValues,
        interpolate, UsdClipsAPIInfoKeys->interpolateMissingClipValues);
}

bool
UsdClipsAPI::SetInterpolateMissingClipValues(bool interpolate,
                                              const std::string& clipSet)
{
    USD_CLIPS_API_CLIPSET_SETTER(SetInterpolateMissingClipValues,
        interpolate, clipSet, UsdClipsAPIInfoKeys->interpolateMissingClipValues);
}

bool
UsdClipsAPI::GetInterpolateMissingClipValues(bool* interpolate) const
{
    USD_CLIPS_API_GETTER(GetInterpolateMissingClipValues,
        interpolate, UsdClipsAPIInfoKeys->interpolateMissingClipValues);
}

bool
UsdClipsAPI::GetInterpolateMissingClipValues(bool* interpolate,
                                              const std::string& clipSet) const
{
    USD_CLIPS_API_CLIPSET_GETTER(GetInterpolateMissingClipValues,
        interpolate, clipSet, UsdClipsAPIInfoKeys->interpolateMissingClipValues);
}

bool 
UsdClipsAPI::SetClipPrimPath(const std::string& primPath)
{
    USD_CLIPS_API_SETTER(SetClipPrimPath,
        primPath, UsdTokens->clipPrimPath);
}

bool 
UsdClipsAPI::SetClipPrimPath(const std::string& primPath, 
                             const std::string& clipSet)
{
    USD_CLIPS_API_CLIPSET_SETTER(SetClipPrimPath,
        primPath, clipSet, UsdClipsAPIInfoKeys->primPath);
}

bool 
UsdClipsAPI::GetClipPrimPath(std::string* primPath) const
{
    USD_CLIPS_API_GETTER(GetClipPrimPath,
        primPath, UsdTokens->clipPrimPath);
}

bool 
UsdClipsAPI::GetClipPrimPath(std::string* primPath, 
                             const std::string& clipSet) const
{
    USD_CLIPS_API_CLIPSET_GETTER(GetClipPrimPath,
        primPath, clipSet, UsdClipsAPIInfoKeys->primPath);
}

bool 
UsdClipsAPI::SetClipActive(const VtVec2dArray& activeClips)
{
    USD_CLIPS_API_SETTER(SetClipActive,
        activeClips, UsdTokens->clipActive);
}

bool 
UsdClipsAPI::SetClipActive(const VtVec2dArray& activeClips, 
                           const std::string& clipSet)
{
    USD_CLIPS_API_CLIPSET_SETTER(SetClipActive,
        activeClips, clipSet, UsdClipsAPIInfoKeys->active);
}

bool 
UsdClipsAPI::GetClipActive(VtVec2dArray* activeClips) const
{
    USD_CLIPS_API_GETTER(GetClipActive,
        activeClips, UsdTokens->clipActive);
}

bool 
UsdClipsAPI::GetClipActive(VtVec2dArray* activeClips, 
                           const std::string& clipSet) const
{
    USD_CLIPS_API_CLIPSET_GETTER(GetClipActive,
        activeClips, clipSet, UsdClipsAPIInfoKeys->active);
}

bool 
UsdClipsAPI::SetClipTimes(const VtVec2dArray& clipTimes)
{
    USD_CLIPS_API_SETTER(SetClipTimes,
        clipTimes, UsdTokens->clipTimes);
}

bool 
UsdClipsAPI::SetClipTimes(const VtVec2dArray& clipTimes, 
                          const std::string& clipSet)
{
    USD_CLIPS_API_CLIPSET_SETTER(SetClipTimes,
        clipTimes, clipSet, UsdClipsAPIInfoKeys->times);
}

bool 
UsdClipsAPI::GetClipTimes(VtVec2dArray* clipTimes) const
{
    USD_CLIPS_API_GETTER(GetClipTimes,
        clipTimes, UsdTokens->clipTimes);
}

bool 
UsdClipsAPI::GetClipTimes(VtVec2dArray* clipTimes, 
                          const std::string& clipSet) const
{
    USD_CLIPS_API_CLIPSET_GETTER(GetClipTimes,
        clipTimes, clipSet, UsdClipsAPIInfoKeys->times);
}

bool 
UsdClipsAPI::GetClipTemplateAssetPath(std::string* clipTemplateAssetPath) const
{
    USD_CLIPS_API_GETTER(GetClipTemplateAssetPath,
        clipTemplateAssetPath, UsdTokens->clipTemplateAssetPath);
}

bool 
UsdClipsAPI::GetClipTemplateAssetPath(std::string* clipTemplateAssetPath, 
                                      const std::string& clipSet) const
{
    USD_CLIPS_API_CLIPSET_GETTER(GetClipTemplateAssetPath,
        clipTemplateAssetPath, clipSet, UsdClipsAPIInfoKeys->templateAssetPath);
}

bool 
UsdClipsAPI::SetClipTemplateAssetPath(const std::string& clipTemplateAssetPath)
{
    USD_CLIPS_API_SETTER(SetClipTemplateAssetPath,
        clipTemplateAssetPath, UsdTokens->clipTemplateAssetPath);
}

bool 
UsdClipsAPI::SetClipTemplateAssetPath(const std::string& clipTemplateAssetPath, 
                                      const std::string& clipSet)
{
    USD_CLIPS_API_CLIPSET_SETTER(SetClipTemplateAssetPath,
        clipTemplateAssetPath, clipSet, UsdClipsAPIInfoKeys->templateAssetPath);
}

bool 
UsdClipsAPI::GetClipTemplateStride(double* clipTemplateStride) const
{
    USD_CLIPS_API_GETTER(GetClipTemplateStride,
        clipTemplateStride, UsdTokens->clipTemplateStride);
}

bool 
UsdClipsAPI::GetClipTemplateStride(double* clipTemplateStride, 
                                   const std::string& clipSet) const
{
    USD_CLIPS_API_CLIPSET_GETTER(GetClipTemplateStride,
        clipTemplateStride, clipSet, UsdClipsAPIInfoKeys->templateStride);
}

bool 
UsdClipsAPI::SetClipTemplateStride(const double clipTemplateStride)
{
    if (clipTemplateStride <= 0) {
        TF_CODING_ERROR("Invalid clipTemplateStride %f for prim <%s>. "
                        "clipTemplateStride must be greater than 0.",
                        clipTemplateStride, 
                        GetPrim().GetPath().GetText());
        return false;
    }

    USD_CLIPS_API_SETTER(SetClipTemplateStride,
        clipTemplateStride, UsdTokens->clipTemplateStride);
}

bool 
UsdClipsAPI::SetClipTemplateStride(const double clipTemplateStride, 
                                   const std::string& clipSet)
{
    if (clipTemplateStride <= 0) {
        TF_CODING_ERROR("Invalid clipTemplateStride %f for prim <%s>. "
                        "clipTemplateStride must be greater than 0.",
                        clipTemplateStride, 
                        GetPrim().GetPath().GetText());
        return false;
    }

    USD_CLIPS_API_CLIPSET_SETTER(SetClipTemplateStride,
        clipTemplateStride, clipSet, UsdClipsAPIInfoKeys->templateStride);
}

bool 
UsdClipsAPI::GetClipTemplateActiveOffset(double* clipTemplateActiveOffset) const
{
    return GetClipTemplateActiveOffset(clipTemplateActiveOffset,
                                       UsdClipsAPISetNames->default_.GetString());
}

bool 
UsdClipsAPI::GetClipTemplateActiveOffset(double* clipTemplateActiveOffset,
                                         const std::string& clipSet) const
{
    USD_CLIPS_API_CLIPSET_GETTER(GetClipTemplateActiveOffset,
        clipTemplateActiveOffset, clipSet, 
        UsdClipsAPIInfoKeys->templateActiveOffset);
}

bool
UsdClipsAPI::SetClipTemplateActiveOffset(const double clipTemplateActiveOffset)
{
    return SetClipTemplateActiveOffset(clipTemplateActiveOffset,
                                       UsdClipsAPISetNames->default_.GetString());
}

bool
UsdClipsAPI::SetClipTemplateActiveOffset(const double clipTemplateActiveOffset,
                                         const std::string& clipSet)
{
    USD_CLIPS_API_CLIPSET_SETTER(SetClipTemplateActiveOffset,
        clipTemplateActiveOffset, clipSet, 
        UsdClipsAPIInfoKeys->templateActiveOffset);
}

bool 
UsdClipsAPI::GetClipTemplateStartTime(double* clipTemplateStartTime) const
{
    USD_CLIPS_API_GETTER(GetClipTemplateStartTime,
        clipTemplateStartTime, UsdTokens->clipTemplateStartTime);
}

bool 
UsdClipsAPI::GetClipTemplateStartTime(double* clipTemplateStartTime, 
                                      const std::string& clipSet) const
{
    USD_CLIPS_API_CLIPSET_GETTER(GetClipTemplateStartTime,
        clipTemplateStartTime, clipSet, UsdClipsAPIInfoKeys->templateStartTime);
}

bool 
UsdClipsAPI::SetClipTemplateStartTime(const double clipTemplateStartTime)
{
    USD_CLIPS_API_SETTER(SetClipTemplateStartTime,
        clipTemplateStartTime, UsdTokens->clipTemplateStartTime);
}

bool 
UsdClipsAPI::SetClipTemplateStartTime(const double clipTemplateStartTime, 
                                      const std::string& clipSet)
{
    USD_CLIPS_API_CLIPSET_SETTER(SetClipTemplateStartTime,
        clipTemplateStartTime, clipSet, UsdClipsAPIInfoKeys->templateStartTime);
}

bool 
UsdClipsAPI::GetClipTemplateEndTime(double* clipTemplateEndTime) const
{
    USD_CLIPS_API_GETTER(GetClipTemplateEndTime,
        clipTemplateEndTime, UsdTokens->clipTemplateEndTime);
}

bool 
UsdClipsAPI::GetClipTemplateEndTime(double* clipTemplateEndTime, 
                                    const std::string& clipSet) const
{
    USD_CLIPS_API_CLIPSET_GETTER(GetClipTemplateEndTime,
        clipTemplateEndTime, clipSet, UsdClipsAPIInfoKeys->templateEndTime);
}

bool 
UsdClipsAPI::SetClipTemplateEndTime(const double clipTemplateEndTime)
{
    USD_CLIPS_API_SETTER(SetClipTemplateEndTime,
        clipTemplateEndTime, UsdTokens->clipTemplateEndTime);
}

bool 
UsdClipsAPI::SetClipTemplateEndTime(const double clipTemplateEndTime, 
                                    const std::string& clipSet)
{
    USD_CLIPS_API_CLIPSET_SETTER(SetClipTemplateEndTime,
        clipTemplateEndTime, clipSet, UsdClipsAPIInfoKeys->templateEndTime);
}

PXR_NAMESPACE_CLOSE_SCOPE
