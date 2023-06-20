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
///
/// \file usdUtils/dependencies.cpp
#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/usd/usdUtils/debugCodes.h"

#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"
#include "pxr/usd/usd/clipsAPI.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/usd/usdFileFormat.h"
#include "pxr/usd/usd/usdcFileFormat.h"
#include "pxr/usd/usd/zipFile.h"
#include "pxr/usd/usdShade/udimUtils.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/trace/trace.h"

#include <stack>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using std::vector;
using std::string;

namespace {

// Enum class representing the type of dependency.
enum class _DepType {
    Reference,
    Sublayer,
    Payload
};

// Enum class representing the external reference types that must be included 
// in the search for external dependencies.
enum class _ReferenceTypesToInclude {
    // Include only references that affect composition.
    CompositionOnly, 

    // Include all external references including asset-valued attributes
    // and non-composition metadata containing SdfAssetPath values.
    All              
};

class _FileAnalyzer {
public:
    // The asset remapping function's signature. 
    // It takes a given asset path and the layer it was found in and returns
    // the corresponding remapped path.
    // 
    // The layer is used to resolve the asset path in cases where the given 
    // asset path is a search path or a relative path. 
    using RemapAssetPathFunc = std::function<std::string 
            (const std::string &assetPath, 
             const SdfLayerRefPtr &layer)>;

    // Takes the asset path and the type of dependency it is and does some 
    // arbitrary processing (like enumerating dependencies).
    using ProcessAssetPathFunc = std::function<void 
            (const std::string &assetPath, 
             const SdfLayerRefPtr &layer,
             const _DepType &depType)>;

    // Attempts to open the file at \p referencePath and analyzes its external 
    // dependencies.  Opening the layer using this non-resolved path ensures
    // that layer metadata is correctly set.  If the file cannot be opened then
    // the analyzer simply retains the \p resolvedPath for later use.
    // 
    // For each dependency that is detected, the provided (optional) callback 
    // functions are invoked. 
    // 
    // \p processPathFunc is invoked first with the raw (un-remapped) path. 
    // Then \p remapPathFunc is invoked. 
    _FileAnalyzer(const std::string &referencePath,
                  const std::string &resolvedPath,
                  _ReferenceTypesToInclude refTypesToInclude=
                        _ReferenceTypesToInclude::All,
                  bool enableMetadataFiltering = false,
                  const RemapAssetPathFunc &remapPathFunc={},
                  const ProcessAssetPathFunc &processPathFunc={}) : 
        _filePath(resolvedPath),
        _refTypesToInclude(refTypesToInclude),
        _metadataFilteringEnabled(enableMetadataFiltering),
        _remapPathFunc(remapPathFunc),
        _processPathFunc(processPathFunc)
    {
        // If this file can be opened on a USD stage or referenced into a USD 
        // stage via composition, then analyze the file, collect & update all 
        // references. If not, return early.
        if (!UsdStage::IsSupportedFile(referencePath)) {
            return;
        }

        TRACE_FUNCTION();

        _layer = SdfLayer::FindOrOpen(referencePath);
        if (!_layer) {
            TF_WARN("Unable to open layer at path @%s@.", 
                referencePath.c_str());
            return;
        }

        // If the newly opened layer is a package, it we do not need to traverse
        // into it as the entire package will be included as a dependency.
        if (_layer->GetFileFormat()->IsPackage()) {
            return;
        }

        _AnalyzeDependencies();
    }

    // overload version of the above constructor that takes a \c layer instead
    // of a filePath.
    _FileAnalyzer(const SdfLayerHandle& layer,
                  _ReferenceTypesToInclude refTypesToInclude=
                        _ReferenceTypesToInclude::All,
                  bool enableMetadataFiltering = false,
                  const RemapAssetPathFunc &remapPathFunc={},
                  const ProcessAssetPathFunc &processPathFunc={}) : 
        _layer(layer),
        _refTypesToInclude(refTypesToInclude),
        _metadataFilteringEnabled(enableMetadataFiltering),
        _remapPathFunc(remapPathFunc),
        _processPathFunc(processPathFunc)
    {
        // In the case we have come across a package layer, traversal can be
        // halted as the entire package will be included as a dependency.
        if (!_layer || _layer->GetFileFormat()->IsPackage()) {
            return;
        }

        _filePath = _layer->GetRealPath();

        _AnalyzeDependencies();
    }

    // Returns the path to the file on disk that is being analyzed.
    const std::string &GetFilePath() const {
        return _filePath;
    }

    // Returns the SdfLayer associated with the file being analyzed.
    const SdfLayerRefPtr &GetLayer() const {
        return _layer;
    }

private:
    // This function returns a boolean value indicating whether an asset path
    // should be processed.  Currently this is used when processing metadata
    // to filter out specific keys
    using ShouldProcessAssetValueFunc = std::function<bool
            (const std::string &key)>;

    // Open the layer, updates references to point to relative or search paths
    // and accumulates all references.
    void _AnalyzeDependencies();

    // This adds the given raw reference path to the list of all referenced 
    // paths. It also returns the remapped reference path, so client code
    // can update the source reference to point to the remapped path.
    std::string _ProcessDependency(const std::string &rawRefPath,
                                   const _DepType &depType);

    // Invokes the path remapping function if one has been supplied, otherwise
    // returns the passed in raw reference path
    std::string _RemapDependency(const std::string &rawRefPath);

    // Processes any sublayers in the SdfLayer associated with the file.
    void _ProcessSublayers();

    // Processes all payloads on the given primSpec.
    void _ProcessPayloads(const SdfPrimSpecHandle &primSpec);

    // Processes prim metadata.
    void _ProcessMetadata(const SdfPrimSpecHandle &primSpec);

    // Processes metadata on properties.
    void _ProcessProperties(const SdfPrimSpecHandle &primSpec);

    // Processes all references on the given primSpec.
    void _ProcessReferences(const SdfPrimSpecHandle &primSpec);

    // Returns the given VtValue with any asset paths remapped to point to 
    // destination-relative path.
    VtValue _UpdateAssetValue(const VtValue &val);

    // Returns the given VtValue with any asset paths remapped to point to 
    // destination-relative path.
    // This overload supports filtering asset path processing based on their
    // key. The processing callback is not invoked for filtered keys, however
    // the remapping callback is invoked in all cases.
    VtValue _UpdateAssetValue(const std::string &key, 
                              const VtValue &val, 
                              const ShouldProcessAssetValueFunc shouldProcessFunc);

    // Callback function that's passed into SdfPayloadsProxy::ModifyItemEdits()
    // or SdfReferencesProxy::ModifyItemEdits() to update all payloads or 
    // references.
    template <class RefOrPayloadType, _DepType DEP_TYPE>
    boost::optional<RefOrPayloadType> _RemapRefOrPayload(
        const RefOrPayloadType &refOrPayload);

    // Resolved path to the file.
    std::string _filePath;

    // SdfLayer corresponding to the file. This will be null for non-layer 
    // files.
    SdfLayerRefPtr _layer;

    // The types of references to include in the processing. 
    // If set to _ReferenceTypesToInclude::CompositionOnly, 
    // non-composition related asset references (eg. property values, property
    // metadata and non-composition prim metadata) are ignored.
    _ReferenceTypesToInclude _refTypesToInclude;

    // if true, will filter ignore path processing for specified metadata keys.
    bool _metadataFilteringEnabled;

    // Remap and process path callback functions.
    RemapAssetPathFunc _remapPathFunc;
    ProcessAssetPathFunc _processPathFunc;
};

std::string 
_FileAnalyzer::_ProcessDependency(const std::string &rawRefPath,
                                  const _DepType &depType)
{
    if (_processPathFunc) {
        _processPathFunc(rawRefPath, GetLayer(), depType);
    }

    return _RemapDependency(rawRefPath);
}

std::string 
_FileAnalyzer::_RemapDependency(const std::string &rawRefPath) {
    if (_remapPathFunc) {
        return _remapPathFunc(rawRefPath, GetLayer());
    }

    // Return the raw reference path if there's no asset path remapping 
    // function.
    return rawRefPath;
}

VtValue 
_FileAnalyzer::_UpdateAssetValue(const VtValue &val)
{
    return _UpdateAssetValue(
        std::string(),
        val,
        [](const std::string &) { return true; }
    );
}

VtValue 
_FileAnalyzer::_UpdateAssetValue(const std::string &key, 
                            const VtValue &val, 
                            const ShouldProcessAssetValueFunc shouldProcessFunc)
{
    if (val.IsHolding<SdfAssetPath>()) {
        auto assetPath = val.UncheckedGet<SdfAssetPath>();
        const std::string& rawAssetPath = assetPath.GetAssetPath();
        if (!rawAssetPath.empty()) {
            const std::string remappedPath = shouldProcessFunc(key)
                ? _ProcessDependency(rawAssetPath, _DepType::Reference)
                : _RemapDependency(rawAssetPath);

            return remappedPath.empty() ? 
                VtValue() : VtValue(SdfAssetPath(remappedPath));
        }
    } else if (val.IsHolding<VtArray<SdfAssetPath>>()) {
        VtArray<SdfAssetPath> updatedArray;
        const VtArray<SdfAssetPath>& originalArray = 
            val.UncheckedGet< VtArray<SdfAssetPath> >();
        
        // ensure explicit empty array value is preserved
        if (originalArray.empty()) {
            return VtValue::Take(updatedArray);
        }
        
        for (const SdfAssetPath& assetPath : originalArray) {                
            const std::string& rawAssetPath = assetPath.GetAssetPath();
            if (!rawAssetPath.empty()) {
                const std::string remappedPath = shouldProcessFunc(key)
                    ? _ProcessDependency(rawAssetPath, _DepType::Reference)
                    : _RemapDependency(rawAssetPath);

                if (!remappedPath.empty()) {
                    updatedArray.push_back(SdfAssetPath(remappedPath));
                }
            }
        }
        return updatedArray.empty() ? VtValue() : VtValue::Take(updatedArray);
    }
    else if (val.IsHolding<VtDictionary>()) {
        VtDictionary updatedDict;
        const VtDictionary& originalDict = val.UncheckedGet<VtDictionary>();

        // ensure explicit empty dict value is preserved
        if (originalDict.empty()) {
            return VtValue::Take(updatedDict);
        }

        for (const auto& p : originalDict) {
            const std::string dictKey = key.empty() ? key : key + ':' + p.first;
            VtValue updatedVal = 
                _UpdateAssetValue(dictKey, p.second, shouldProcessFunc);
            if (!updatedVal.IsEmpty()) {
                updatedDict[p.first] = std::move(updatedVal);
            }
        }
        return updatedDict.empty() ? VtValue() : VtValue::Take(updatedDict);
    }

    return val;
}

void
_FileAnalyzer::_ProcessSublayers()
{
    if (_remapPathFunc) {
        _layer->GetSubLayerPaths().ModifyItemEdits(
            [this](const std::string& path) {
                std::string remappedPath =
                    _ProcessDependency(path, _DepType::Sublayer); 
                return remappedPath.empty() ? 
                    boost::optional<std::string>() : 
                    boost::optional<std::string>(std::move(remappedPath));
            });
    } else {
        for (const auto &subLayer: _layer->GetSubLayerPaths()) {
            _ProcessDependency(subLayer, _DepType::Sublayer);
        }
    }
}

template <class RefOrPayloadType, _DepType DEP_TYPE>
boost::optional<RefOrPayloadType> 
_FileAnalyzer::_RemapRefOrPayload(const RefOrPayloadType &refOrPayload)
{
    // If this is a local (or self) reference or payload, there's no asset path 
    // to update.
    if (refOrPayload.GetAssetPath().empty()) {
        return refOrPayload;
    }

    std::string remappedPath = 
        _ProcessDependency(refOrPayload.GetAssetPath(), DEP_TYPE);

    // If the remapped path was empty, return none to indicate this reference
    // or payload should be removed.
    if (remappedPath.empty()) {
        return boost::none;
    }

    // If the path was not remapped to a different path, then return the 
    // incoming payload unmodified.
    if (remappedPath == refOrPayload.GetAssetPath()) {
        return refOrPayload;
    }

    // The payload or reference path was remapped, hence construct a new 
    // SdfPayload or SdfReference object with the remapped path.
    RefOrPayloadType remappedRefOrPayload = refOrPayload;
    remappedRefOrPayload.SetAssetPath(remappedPath);
    return remappedRefOrPayload;
}

void
_FileAnalyzer::_ProcessPayloads(const SdfPrimSpecHandle &primSpec)
{
    if (_remapPathFunc) {
        primSpec->GetPayloadList().ModifyItemEdits(
            [this](const SdfPayload& payload) {
                return _RemapRefOrPayload<SdfPayload, _DepType::Payload>(
                    payload);
            });
    } else {
        for (SdfPayload const& payload:
             primSpec->GetPayloadList().GetAddedOrExplicitItems()) {

            // If the asset path is empty this is a local payload. We can ignore
            // these since they refer to the same layer where the payload was
            // authored.
            if (!payload.GetAssetPath().empty()) {
                _ProcessDependency(payload.GetAssetPath(), _DepType::Payload);
            }
        }
    }
}

void
_FileAnalyzer::_ProcessProperties(const SdfPrimSpecHandle &primSpec)
{
    // Include external references in property values and metadata only if 
    // the client is interested in all reference types. i.e. return early if 
    // _refTypesToInclude is CompositionOnly.
    if (_refTypesToInclude == _ReferenceTypesToInclude::CompositionOnly)
        return;

    // XXX:2016-04-14 Note that we use the field access API
    // here rather than calling GetAttributes, as creating specs for
    // large numbers of attributes, most of which are *not* asset
    // path-valued and therefore not useful here, is expensive.
    //
    const VtValue propertyNames =
        primSpec->GetField(SdfChildrenKeys->PropertyChildren);

    if (!propertyNames.IsHolding<vector<TfToken>>()) {
        return;
    }

    for (const auto& name : propertyNames.UncheckedGet<vector<TfToken>>()) {
        // For every property
        // Build an SdfPath to the property
        const SdfPath path = primSpec->GetPath().AppendProperty(name);

        // Check property metadata
        for (const TfToken& infoKey : _layer->ListFields(path)) {
            if (infoKey != SdfFieldKeys->Default &&
                infoKey != SdfFieldKeys->TimeSamples) {
                        
                VtValue value = _layer->GetField(path, infoKey);
                VtValue updatedValue = _UpdateAssetValue(value);
                if (_remapPathFunc && value != updatedValue) {
                    _layer->SetField(path, infoKey, updatedValue);
                }
            }
        }

        // Check property existence
        const VtValue vtTypeName =
            _layer->GetField(path, SdfFieldKeys->TypeName);
        if (!vtTypeName.IsHolding<TfToken>()) {
            continue;
        }

        const TfToken typeName = vtTypeName.UncheckedGet<TfToken>();
        if (typeName == SdfValueTypeNames->Asset ||
            typeName == SdfValueTypeNames->AssetArray) {

            // Check default value
            VtValue defValue = _layer->GetField(path, SdfFieldKeys->Default);
            VtValue updatedDefValue = _UpdateAssetValue(defValue);
            if (_remapPathFunc && defValue != updatedDefValue) {
                _layer->SetField(path, SdfFieldKeys->Default, updatedDefValue);
            }

            // Check timeSample values
            for (double t : _layer->ListTimeSamplesForPath(path)) {
                VtValue timeSampleVal;
                if (_layer->QueryTimeSample(path, t, &timeSampleVal)) {
                    VtValue updatedVal = _UpdateAssetValue(timeSampleVal);
                    if (_remapPathFunc && timeSampleVal != updatedVal) {

                        if (updatedVal.IsEmpty()) {
                            _layer->EraseTimeSample(path, t);
                        }
                        else {
                            _layer->SetTimeSample(path, t, updatedVal);
                        }
                    }
                }
            }
        }
    }
}

// Determines if a metadata key should be processed.
// XXX: Currently operates on a single hardcoded value, but in the future we
//      would like to give users the ability to specify keys to filter.
static
bool _IgnoreAssetInfoIdentifier(const std::string &key) {
    return !TfStringEndsWith(key, "assetInfo:identifier");
}

void
_FileAnalyzer::_ProcessMetadata(const SdfPrimSpecHandle &primSpec)
{
    if (_refTypesToInclude == _ReferenceTypesToInclude::All) {
        for (const TfToken& infoKey : primSpec->GetMetaDataInfoKeys()) {
            VtValue value = primSpec->GetInfo(infoKey);
            VtValue updatedValue = _metadataFilteringEnabled
                 ? _UpdateAssetValue(infoKey, value, _IgnoreAssetInfoIdentifier)
                 : _UpdateAssetValue(value);
            if (_remapPathFunc && value != updatedValue) {
                if (updatedValue.IsEmpty()) {
                    primSpec->ClearInfo(infoKey);
                }
                else {
                    primSpec->SetInfo(infoKey, updatedValue);
                }
            }
        }
    }

    // Process clips["templateAssetPath"], which is a string value 
    // containing one or more #'s. See 
    // UsdClipsAPI::GetClipTemplateAssetPath for details. 
    VtValue clipsValue = primSpec->GetInfo(UsdTokens->clips);
    if (!clipsValue.IsEmpty() && clipsValue.IsHolding<VtDictionary>()) {
        const VtDictionary origClipsDict = 
                clipsValue.UncheckedGet<VtDictionary>();

        // Create a copy of the clips dictionary, as we may have to modify it.
        VtDictionary clipsDict = origClipsDict;
        for (auto &clipSetNameAndDict : clipsDict) {
            if (clipSetNameAndDict.second.IsHolding<VtDictionary>()) {
                VtDictionary clipDict = 
                    clipSetNameAndDict.second.UncheckedGet<VtDictionary>();

                if (VtDictionaryIsHolding<std::string>(clipDict, 
                        UsdClipsAPIInfoKeys->templateAssetPath
                            .GetString())) {
                    const std::string &templateAssetPath = 
                            VtDictionaryGet<std::string>(clipDict, 
                                UsdClipsAPIInfoKeys->templateAssetPath
                                    .GetString());

                    if (templateAssetPath.empty()) {
                        continue;
                    }

                    // Remap templateAssetPath if there's a remap function and 
                    // update the clip dictionary.
                    // This retains the #s in the templateAssetPath?
                    if (_remapPathFunc) {
                        // Not adding a dependency on the templated asset path
                        // since it can't be resolved by the resolver.
                        clipDict[UsdClipsAPIInfoKeys->templateAssetPath] = 
                            VtValue(_remapPathFunc(templateAssetPath, 
                                                   GetLayer()));
                        clipsDict[clipSetNameAndDict.first] = VtValue(clipDict);
                    }

                    // Compute the resolved location of the clips 
                    // directory, so we can do a TfGlob for the pattern.
                    // This contains a '/' in the end.
                    const std::string clipsDir = TfGetPathName(
                            templateAssetPath);
                    // Resolve clipsDir relative to this layer. 
                    if (clipsDir.empty()) {
                        TF_WARN("Invalid template asset path '%s'.",
                            templateAssetPath.c_str());
                        continue;
                    }
                    const std::string clipsDirAssetPath = 
                        SdfComputeAssetPathRelativeToLayer(_layer, clipsDir);

                    // We don't attempt to resolve the clips directory asset
                    // path, since Ar does not support directory-path 
                    // resolution. 
                    if (!TfIsDir(clipsDirAssetPath)) {
                        TF_WARN("Clips directory '%s' is not a valid directory "
                            "on the filesystem.", clipsDirAssetPath.c_str());
                        continue;
                    }

                    std::string clipsBaseName = TfGetBaseName(
                            templateAssetPath);
                    std::string globPattern = TfStringCatPaths(
                            clipsDirAssetPath, 
                            TfStringReplace(clipsBaseName, "#", "*"));
                    const std::vector<std::string> clipAssetRefs = 
                        TfGlob(globPattern);
                    for (auto &clipAsset : clipAssetRefs) {
                        // Reconstruct the raw, unresolved clip reference, for 
                        // which the dependency must be processed.
                        // 
                        // clipsDir contains a '/' in the end, but 
                        // clipsDirAssetPath does not. Hence, add a '/' to 
                        // clipsDirAssetPath before doing the replace.
                        std::string rawClipRef = TfStringReplace(
                                clipAsset, clipsDirAssetPath + '/', clipsDir);
                        _ProcessDependency(rawClipRef, _DepType::Reference);
                    }
                }
            }
        }

        // Update the clips dictionary only if it has been modified.
        if (_remapPathFunc && clipsDict != origClipsDict) {
            primSpec->SetInfo(UsdTokens->clips, VtValue(clipsDict));
        }
    }
}

void
_FileAnalyzer::_ProcessReferences(const SdfPrimSpecHandle &primSpec)
{
    if (_remapPathFunc) {
        primSpec->GetReferenceList().ModifyItemEdits(
            [this](const SdfReference& ref) {
                return _RemapRefOrPayload<SdfReference, _DepType::Reference>(
                    ref);
            });
    } else {
        for (SdfReference const& ref:
            primSpec->GetReferenceList().GetAddedOrExplicitItems()) {

            // If the asset path is empty this is a local reference. We can
            // ignore these since they refer to the same layer where the
            // reference was authored.
            if (!ref.GetAssetPath().empty()) {
                _ProcessDependency(ref.GetAssetPath(), _DepType::Reference);
            }
        }
    }
}

void
_FileAnalyzer::_AnalyzeDependencies()
{
    TRACE_FUNCTION();

    _ProcessSublayers();

    std::stack<SdfPrimSpecHandle> dfs;
    dfs.push(_layer->GetPseudoRoot());

    while (!dfs.empty()) {
        SdfPrimSpecHandle curr = dfs.top();
        dfs.pop();

        // Metadata is processed even on the pseudoroot, which ensures
        // we process layer metadata properly.
        _ProcessMetadata(curr);
        if (curr != _layer->GetPseudoRoot()) {
            _ProcessPayloads(curr);    
            _ProcessProperties(curr);
            _ProcessReferences(curr);
        }

        // variants "children"
        for (const SdfVariantSetsProxy::value_type& p :
            curr->GetVariantSets()) {
            for (const SdfVariantSpecHandle& variantSpec :
                p.second->GetVariantList()) {
                dfs.push(variantSpec->GetPrimSpec());
            }
        }

        // children
        for (const SdfPrimSpecHandle& child : curr->GetNameChildren()) {
            dfs.push(child);
        }
    }
}

class _AssetLocalizer {
public:
    using LayerAndDestPath = std::pair<SdfLayerRefPtr, std::string>;
    using SrcPathAndDestPath = std::pair<std::string, std::string>;
    using DestFilePathAndAnalyzer = std::pair<std::string, _FileAnalyzer>;
    using LayerDependenciesMap = std::unordered_map<SdfLayerRefPtr, 
            std::vector<std::string>, TfHash>;

    // Computes the given asset's dependencies recursively and determines
    // the information needed to localize the asset.
    // If \p destDir is empty, none of the asset layers are modified, allowing
    // this class to be used purely as a recursive dependency finder.
    // \p enableMetadataFiltering if true, will instruct FileAnalyzer to skip
    // processing asset paths that match a list of predefined names
    // \p firstLayerName if non-empty, holds desired the name of the root layer 
    // in the localized asset. 
    // 
    // If \p origRootFilePath is non-empty, it points to the original root layer 
    // of which \p assetPath is a flattened representation. This is by 
    // UsdUtilsCreateNewARKitUsdzPackage(), to point to the original 
    // (unflattened) asset with external dependencies.
    // 
    // \p dependenciesToSkip lists an optional set of dependencies that must be 
    // skipped in the created package. This list must contain fully resolved 
    // asset paths that must be skipped in the created package. It cannot 
    // contain the resolved root \p assetPath value itself. If a dependency
    // is skipped because it exists in the \p dependenciesToSkip list, none of 
    // the transitive dependencies referenced by the skipped dependency are 
    // processed and may be missing in the created package.
    _AssetLocalizer(const SdfAssetPath &assetPath, const std::string &destDir,
                    bool enableMetadataFiltering,
                    const std::string &firstLayerName=std::string(),
                    const std::string &origRootFilePath=std::string(),
                    const std::vector<std::string> 
                        &dependenciesToSkip=std::vector<std::string>())
    {
        _DirectoryRemapper dirRemapper;

        auto &layerDependenciesMap = _layerDependenciesMap;

        auto &resolver = ArGetResolver();

        const std::string assetPathStr = assetPath.GetAssetPath();
        const bool isAnonymousLayer = 
            SdfLayer::IsAnonymousLayerIdentifier(assetPathStr);
        std::string rootFilePath;

        if (!isAnonymousLayer) {
            rootFilePath = resolver.Resolve(assetPathStr);

            // Ensure that the resolved path is not empty.
            if (rootFilePath.empty()) {
                return;
            }
        }

        // Record all dependencies in layerDependenciesMap so we can recurse
        // on them.
        const auto processPathFunc =
            [&layerDependenciesMap](
                const std::string &ap, const SdfLayerRefPtr &layer,
                _DepType depType) {
            layerDependenciesMap[layer].push_back(ap);
        };

        // If destination directory is an empty string, skip any remapping
        // of asset paths.
        const auto remapAssetPathFunc = destDir.empty() ?
            _FileAnalyzer::RemapAssetPathFunc() : 
            [&layerDependenciesMap, &dirRemapper, &destDir, &rootFilePath, 
             &origRootFilePath, &firstLayerName](
                const std::string &ap, 
                const SdfLayerRefPtr &layer) {

            return _RemapAssetPath(ap, layer, 
                    origRootFilePath, rootFilePath, firstLayerName,
                    &dirRemapper, /* isRelativePath */ nullptr);
        };

        // Set of all seen files. We maintain this set to avoid redundant
        // dependency analysis of already seen files.
        std::unordered_set<std::string> seenFiles;
         
        std::stack<DestFilePathAndAnalyzer> filesToLocalize;

        if (isAnonymousLayer)
        {
            SdfLayerRefPtr layer = SdfLayer::Find(assetPathStr);
            if (!layer) {
                return;
            }

            rootFilePath = "anon_layer." + layer->GetFileExtension();

            seenFiles.insert(assetPathStr);
            std::string destFilePath = TfStringCatPaths(destDir, 
                    TfGetBaseName(rootFilePath));
            filesToLocalize.emplace(destFilePath, _FileAnalyzer(
                    layer,
                    /*refTypesToInclude*/ _ReferenceTypesToInclude::All,
                    /*enableMetadataFiltering*/ enableMetadataFiltering,
                    remapAssetPathFunc, processPathFunc));
        }
        else
        {
            seenFiles.insert(rootFilePath);
            std::string destFilePath = TfStringCatPaths(destDir, 
                    TfGetBaseName(rootFilePath));
            filesToLocalize.emplace(destFilePath, _FileAnalyzer(
                    assetPathStr, rootFilePath, 
                    /*refTypesToInclude*/ _ReferenceTypesToInclude::All,
                    /*enableMetadataFiltering*/ enableMetadataFiltering,
                    remapAssetPathFunc, processPathFunc));
        }

        while (!filesToLocalize.empty()) {
            // Copying data here since we're about to pop.
            const DestFilePathAndAnalyzer destFilePathAndAnalyzer = 
                filesToLocalize.top();
            filesToLocalize.pop();

            auto &destFilePath = destFilePathAndAnalyzer.first;
            auto &fileAnalyzer = destFilePathAndAnalyzer.second;

            if (!fileAnalyzer.GetLayer()) {
                const std::string &srcFilePath = fileAnalyzer.GetFilePath();
                if (UsdShadeUdimUtils::IsUdimIdentifier(srcFilePath)) {
                    _ResolveUdimPaths(srcFilePath, destFilePath);
                } else {
                    _fileCopyMap.emplace_back(srcFilePath, destFilePath);
                }

                continue;
            }

            _layerExportMap.emplace_back(fileAnalyzer.GetLayer(), 
                                         destFilePath);

            const auto &layerDepIt = layerDependenciesMap.find(
                    fileAnalyzer.GetLayer());

            if (layerDepIt == _layerDependenciesMap.end()) {
                // The layer has no external dependencies.
                continue;
            }

            for (std::string ref : layerDepIt->second) {
                // If this is a package-relative path, then simply copy the 
                // package over. 
                // Note: recursive search for dependencies ends here. 
                // This is because we don't want to be modifying packaged 
                // assets during asset isolation or archival. 
                // XXX: We may want to reconsider this approach in the future.
                if (ArIsPackageRelativePath(ref)) {
                    ref = ArSplitPackageRelativePathOuter(ref).first;
                }

                const std::string refAssetPath = 
                        SdfComputeAssetPathRelativeToLayer(
                            fileAnalyzer.GetLayer(), ref);
                std::string resolvedRefFilePath;

                // Specially handle UDIM paths
                if (UsdShadeUdimUtils::IsUdimIdentifier(ref)) {
                    resolvedRefFilePath = UsdShadeUdimUtils::ResolveUdimPath(
                        ref, fileAnalyzer.GetLayer());
                } else {
                    resolvedRefFilePath = resolver.Resolve(refAssetPath);
                }

                if (resolvedRefFilePath.empty()) {
                    TF_WARN("Failed to resolve reference @%s@ with computed "
                            "asset path @%s@ found in layer @%s@.", 
                            ref.c_str(),
                            refAssetPath.c_str(), 
                            fileAnalyzer.GetFilePath().c_str());

                    _unresolvedAssetPaths.push_back(refAssetPath);
                    continue;
                } 

                // Check if this dependency must skipped.
                if (std::find(dependenciesToSkip.begin(), 
                              dependenciesToSkip.end(), resolvedRefFilePath) != 
                           dependenciesToSkip.end()) {
                    continue;
                }

                // Given the way our remap function (_RemapAssetPath) works, we 
                // should only have to copy every resolved file once during
                // localization.
                if (!seenFiles.insert(resolvedRefFilePath).second) {
                    continue;
                }

                // XXX: We don't localize directory references. Should we copy 
                // the entire directory over?
                if (TfIsDir(resolvedRefFilePath)) {
                    continue;
                }

                bool isRelativePath = false;
                std::string remappedRef = _RemapAssetPath(ref, 
                    fileAnalyzer.GetLayer(),
                    origRootFilePath, rootFilePath, firstLayerName,
                    &dirRemapper, &isRelativePath);

                // If it's a relative path, construct the full path relative to
                // the final (destination) location of the reference-containing 
                // file.
                const std::string destDirForRef = 
                    isRelativePath ? TfGetPathName(destFilePath) : destDir; 
                const std::string destFilePathForRef = TfStringCatPaths(
                        destDirForRef, remappedRef);

                filesToLocalize.emplace(destFilePathForRef, _FileAnalyzer(
                        refAssetPath, resolvedRefFilePath,
                        /* refTypesToInclude */ _ReferenceTypesToInclude::All,
                        /*enableMetadataFiltering*/ enableMetadataFiltering,
                        remapAssetPathFunc, processPathFunc));
            }
        }
    }

    // Get the list of layers to be localized along with their corresponding 
    // destination paths.
    const std::vector<LayerAndDestPath> &GetLayerExportMap() const {
        return _layerExportMap;
    }

    // Get the list of source files to be copied along with their corresponding 
    // destination paths.
    const std::vector<SrcPathAndDestPath> &GetFileCopyMap() const {
        return _fileCopyMap;
    }

    // Returns ths list of all unresolved references.
    const std::vector<std::string> GetUnresolvedAssetPaths() const {
        return _unresolvedAssetPaths;
    }

private:
    // This function will ensure that all tiles that match the UDIM identifier
    // contained in src path are correctly added to the file copy map
    void _ResolveUdimPaths(
        const std::string &srcFilePath,
        const std::string &destFilePath) 
    {
        // Since the source path should already be pre-resolved,
        // a proper layer doesn't have to be provided
        const std::vector<UsdShadeUdimUtils::ResolvedPathAndTile> resolvedPaths =
            UsdShadeUdimUtils::ResolveUdimTilePaths(srcFilePath, SdfLayerHandle());

        for (const auto & resolvedPath : resolvedPaths) {
            const std::string destUdimPath = 
                UsdShadeUdimUtils::ReplaceUdimPattern(
                    destFilePath, resolvedPath.second);
            
            _fileCopyMap.emplace_back(resolvedPath.first, destUdimPath);
        }
    }

    // This will contain a mapping of SdfLayerRefPtr's mapped to their 
    // desination path inside the destination directory.
    std::vector<LayerAndDestPath> _layerExportMap;

    // This will contain a mapping of source file path to the corresponding 
    // desination file path.
    std::vector<SrcPathAndDestPath> _fileCopyMap;

    // A map of layers and their corresponding vector of raw external reference
    // paths.
    LayerDependenciesMap _layerDependenciesMap;

    // List of all the unresolvable asset paths.
    std::vector<std::string> _unresolvedAssetPaths;

    // Helper object for remapping paths to an artifically-generated path.
    class _DirectoryRemapper {
    public:
        _DirectoryRemapper() : _nextDirectoryNum(0) { }

        // Remap the given file path by replacing the directory with a
        // unique, artifically generated name. The generated directory name
        // will be reused if the original directory is seen again on a
        // subsequent call to Remap.
        std::string Remap(const std::string& filePath)
        {
            if (ArIsPackageRelativePath(filePath)) {
                std::pair<std::string, std::string> packagePath = 
                    ArSplitPackageRelativePathOuter(filePath);
                return ArJoinPackageRelativePath(
                    Remap(packagePath.first), packagePath.second);
            }

            const std::string pathName = TfGetPathName(filePath);
            if (pathName.empty()) {
                return filePath;
            }

            const std::string baseName = TfGetBaseName(filePath);
            
            auto insertStatus = _oldToNewDirectory.insert({pathName, ""});
            if (insertStatus.second) {
                insertStatus.first->second = 
                    TfStringPrintf("%zu", _nextDirectoryNum++);
            }
            
            return TfStringCatPaths(insertStatus.first->second, baseName);
        }

    private:
        size_t _nextDirectoryNum;
        std::unordered_map<std::string, std::string> _oldToNewDirectory;
    };

    // Remaps a given asset path (\p refPath) to be relative to the layer 
    // containing it (\p layer) for the purpose of localization.
    // \p dirRemapper should not be empty.
    // \p pathType is allowed to be a nullptr.
    // \p origRootFilePath should contain to the path to the original 
    // root file from which the asset at \p rootFilePath was created (possibly 
    // by flattening),
    // \p rootFilePath should contain a path to the resolved and localized root 
    // asset layer on disk.
    // \p firstLayerName if non-empty contains the final name of the asset's 
    // root layer.
    static std::string _RemapAssetPath(
        const std::string &refPath, 
        const SdfLayerRefPtr &layer,
        const std::string origRootFilePath, 
        const std::string rootFilePath, 
        const std::string &firstLayerName,
        _DirectoryRemapper *dirRemapper,
        bool *isRelativePath);
};

std::string 
_AssetLocalizer::_RemapAssetPath(const std::string &refPath, 
                                 const SdfLayerRefPtr &layer,
                                 std::string origRootFilePath,
                                 std::string rootFilePath, 
                                 const std::string &firstLayerName,
                                 _DirectoryRemapper *dirRemapper,
                                 bool *isRelativePathOut)
{
    auto &resolver = ArGetResolver();

    const bool isContextDependentPath =
        resolver.IsContextDependentPath(refPath);

    // We want to maintain relative paths where possible to keep localized
    // assets as close as possible to their original layout. However, we
    // skip this for context-dependent paths because those must be resolved
    // to determine what asset is being referred to.
    if (!isContextDependentPath) {
        // We determine if refPath is relative by creating identifiers with
        // and without the anchoring layer and seeing if they're the same.
        // If they aren't, then refPath depends on the anchor, so we assume
        // it's relative.
        const std::string anchored = 
            resolver.CreateIdentifier(refPath, layer->GetResolvedPath());
        const std::string unanchored =
            resolver.CreateIdentifier(refPath);
        const bool isRelativePath = (anchored != unanchored);

        if (isRelativePath) {
            // Asset localization is rooted at the location of the root layer.
            // If this relative path points somewhere outside that location
            // (e.g., a relative path like "../foo.jpg"). there will be nowhere
            // to put this asset in the localized asset structure. In that case,
            // we need to remap this path. Otherwise, we can keep the relative
            // asset path as-is.
            const ArResolvedPath resolvedRefPath = resolver.Resolve(anchored);
            const bool refPathIsOutsideAssetLocation = 
                !TfStringStartsWith(
                    TfNormPath(TfGetPathName(resolvedRefPath)),
                    TfNormPath(TfGetPathName(rootFilePath)));

            if (!refPathIsOutsideAssetLocation) {
                if (isRelativePathOut) {
                    *isRelativePathOut = true;
                }

                // Return relative paths unmodified.
                return refPath;
            }
        }
    }

    if (isRelativePathOut) {
        *isRelativePathOut = false;
    }

    std::string result = refPath;
    if (isContextDependentPath) {
        // Absolutize the search path, to avoid collisions resulting from the 
        // same search path resolving to different paths in different resolver
        // contexts.
        const std::string refAssetPath = 
                SdfComputeAssetPathRelativeToLayer(layer, refPath);
        const std::string refFilePath = resolver.Resolve(refAssetPath);

        const bool resolveOk = !refFilePath.empty();

        if (resolveOk) {
            result = refFilePath;
        } else {
            // Failed to resolve, hence retain the reference as is.
            result = refAssetPath;
        }
    }

    // Normalize paths compared below to account for path format differences.
    const std::string layerPath = 
        TfNormPath(layer->GetRealPath());
    result = TfNormPath(result);
    rootFilePath = TfNormPath(rootFilePath);
    origRootFilePath = TfNormPath(origRootFilePath);

    bool resultPointsToRoot = ((result == rootFilePath) || 
                               (result == origRootFilePath));
    // If this is a self-reference, then remap to a relative path that points 
    // to the file itself.
    if (result == layerPath) {
        // If this is a self-reference in the root layer and we're renaming the 
        // root layer, simply set the reference path to point to the renamed 
        // root layer.
        return resultPointsToRoot && !firstLayerName.empty() ? 
            firstLayerName : TfGetBaseName(result);
    }
   
    // References to the original (unflattened) root file need to be remapped 
    // to point to the new root file.
    if (resultPointsToRoot && layerPath == rootFilePath) {
        return !firstLayerName.empty() ? firstLayerName : TfGetBaseName(result);
    }

    // Result is now an absolute or a repository path. Simply strip off the 
    // leading slashes to make it relative.
    // Strip off any drive letters.
    if (result.size() >= 2 && result[1] == ':') {
        result.erase(0, 2);
    }

    // Strip off any initial slashes.
    result = TfStringTrimLeft(result, "/");

    // Remap the path to an artifically-constructed one so that the source 
    // directory structure isn't embedded in the final .usdz file. Otherwise,
    // sensitive information (e.g. usernames, movie titles...) in directory
    // names may be inadvertently leaked in the .usdz file.
    return dirRemapper->Remap(result);
}

// Returns a relative path for fullDestPath, relative to the given destination 
// directory (destDir).
static 
std::string 
_GetDestRelativePath(const std::string &fullDestPath, 
                     const std::string &destDir)
{
    std::string destPath = fullDestPath;
    // fullDestPath won't start with destDir if destDir is a relative path, 
    // relative to CWD.
    if (TfStringStartsWith(destPath, destDir)) {
        destPath = destPath.substr(destDir.length());
    }
    return destPath;
}

} // end of anonymous namespace


static void
_ExtractExternalReferences(
    const std::string& filePath,
    const _ReferenceTypesToInclude &refTypesToInclude,
    std::vector<std::string>* subLayers,
    std::vector<std::string>* references,
    std::vector<std::string>* payloads)
{
    auto &resolver = ArGetResolver();

    const auto processPathFunc = [&subLayers, &references, &payloads](
            const std::string &assetPath, const SdfLayerRefPtr &layer,
            const _DepType &depType) 
        {
            if (depType == _DepType::Reference) {
                references->push_back(assetPath);
            } else if (depType == _DepType::Sublayer) {
                subLayers->push_back(assetPath);
            } else if (depType == _DepType::Payload) {
                payloads->push_back(assetPath);
            }
        };

    // We only care about knowing what the dependencies are. Hence, set 
    // remapPathFunc to empty.
    if (SdfLayer::IsAnonymousLayerIdentifier(filePath)) {
        _FileAnalyzer(SdfLayer::Find(filePath), refTypesToInclude,
            /*enableMetadataFiltering*/ false,
            /*remapPathFunc*/ {}, 
            processPathFunc);
    }
    else {
        _FileAnalyzer(filePath, resolver.Resolve(filePath), refTypesToInclude,
            /*enableMetadataFiltering*/ false,
            /*remapPathFunc*/ {}, 
            processPathFunc);
    }


    // Sort and remove duplicates
    std::sort(references->begin(), references->end());
    references->erase(std::unique(references->begin(), references->end()),
        references->end());
    std::sort(payloads->begin(), payloads->end());
    payloads->erase(std::unique(payloads->begin(), payloads->end()),
        payloads->end());
}

// XXX: don't even know if it's important to distinguish where
// these asset paths are coming from..  if it's not important, maybe this
// should just go into Sdf's _GatherPrimAssetReferences?  if it is important,
// we could also have another function that takes 3 vectors.
void
UsdUtilsExtractExternalReferences(
    const std::string& filePath,
    std::vector<std::string>* subLayers,
    std::vector<std::string>* references,
    std::vector<std::string>* payloads)
{
    TRACE_FUNCTION();
    _ExtractExternalReferences(filePath, _ReferenceTypesToInclude::All, 
        subLayers, references, payloads);
}

static 
bool
_CreateNewUsdzPackage(const SdfAssetPath &assetPath,
                      const std::string &usdzFilePath,
                      const std::string &firstLayerName,
                      const std::string &origRootFilePath=std::string(),
                      const std::vector<std::string> &dependenciesToSkip
                            =std::vector<std::string>())
{
    TF_DEBUG(USDUTILS_CREATE_USDZ_PACKAGE).Msg("Creating USDZ package at '%s' "
        "containing asset @%s@.\n", usdzFilePath.c_str(), 
        assetPath.GetAssetPath().c_str());

    std::string destDir = TfGetPathName(usdzFilePath);
    destDir = destDir.empty() ? "./" : destDir;
    _AssetLocalizer localizer(assetPath, destDir, 
                            /* enableMetadataFiltering */ true,
                            firstLayerName, 
                            origRootFilePath, 
                            dependenciesToSkip);

    auto &layerExportMap = localizer.GetLayerExportMap();
    auto &fileCopyMap = localizer.GetFileCopyMap();

    if (layerExportMap.empty() && fileCopyMap.empty()) {
        return false;
    }

    // Set of all the packaged files.
    std::unordered_set<std::string> packagedFiles;

    const std::string tmpDirPath = ArchGetTmpDir();

    UsdZipFileWriter writer = UsdZipFileWriter::CreateNew(usdzFilePath);

    auto &resolver = ArGetResolver();
    // This returns true of src and dest have the same file extension.
    const auto extensionsMatch = [&resolver](const std::string &src, 
                                             const std::string &dest) {
        return resolver.GetExtension(src) == resolver.GetExtension(dest);
    };

    bool firstLayer = true;
    bool success = true;
    for (auto &layerAndDestPath : layerExportMap) {
        const auto &layer = layerAndDestPath.first;
        std::string destPath = _GetDestRelativePath(
                layerAndDestPath.second, destDir);

        // Change the first layer's name if requested.
        if (firstLayer && !firstLayerName.empty()) {
            const std::string pathName = TfGetPathName(destPath);
            destPath = TfStringCatPaths(pathName, firstLayerName);
            firstLayer = false;
        }

        if (!packagedFiles.insert(destPath).second) {
            TF_WARN("A file already exists at path \"%s\" in the package. "
                "Skipping export of layer @%s@.", destPath.c_str(), 
                layer->GetIdentifier().c_str());
            continue;
        }

        TF_DEBUG(USDUTILS_CREATE_USDZ_PACKAGE).Msg(
            ".. adding layer @%s@ to package at path '%s'.\n", 
            layer->GetIdentifier().c_str(), destPath.c_str());

        // If the layer is a package or if it's inside a package, copy the 
        // entire package. We could extract the package and copy only the 
        // dependencies, but this could get very complicated.
        if (layer->GetFileFormat()->IsPackage() ||
             ArIsPackageRelativePath(layer->GetIdentifier())) {
            std::string packagePath = ArSplitPackageRelativePathOuter(
                    layer->GetRealPath()).first;
            std::string destPackagePath = ArSplitPackageRelativePathOuter(
                    destPath).first;
            if (!packagePath.empty()) {
                std::string inArchivePath = writer.AddFile(packagePath,
                        destPackagePath);
                if (inArchivePath.empty()) {
                    success = false;
                }
            }
        } else if (!layer->IsDirty() && 
                   extensionsMatch(layer->GetRealPath(), destPath)) {
            // If the layer hasn't been modified from its persistent 
            // representation and if its extension isn't changing in the 
            // package, then simply copy it over from its real-path (i.e. 
            // location on disk). This preserves any existing comments in the 
            // file (which will be lost if we were to export all layers before 
            // adding them to to the package).
            std::string inArchivePath = writer.AddFile(layer->GetRealPath(), 
                    destPath);
            if (inArchivePath.empty()) {
                success = false;
            }
        } else {
            // If the layer has been modified or needs to be modified, then we 
            // need to export it to a temporary file before adding it to the 
            // package.
            SdfFileFormat::FileFormatArguments args;

            const SdfFileFormatConstPtr fileFormat = 
                    SdfFileFormat::FindByExtension(
                        SdfFileFormat::GetFileExtension(destPath));

            if (TfDynamic_cast<UsdUsdFileFormatConstPtr>(fileFormat)) {
                args[UsdUsdFileFormatTokens->FormatArg] = 
                        UsdUsdFileFormat::GetUnderlyingFormatForLayer(
                            *get_pointer(layer));
            }
            
            std::string tmpLayerExportPath = TfStringCatPaths(tmpDirPath, 
                    TfGetBaseName(destPath));
            layer->Export(tmpLayerExportPath, /*comment*/ "", args);

            std::string inArchivePath = writer.AddFile(tmpLayerExportPath, 
                    destPath);

            if (inArchivePath.empty()) {
                // XXX: Should we discard the usdz file and return early here?
                TF_WARN("Failed to add temporary layer at '%s' to the package "
                    "at path '%s'.", tmpLayerExportPath.c_str(), 
                    usdzFilePath.c_str());
                success = false;
            } else {
                // The file has been added to the package successfully. We can 
                // delete it now.
                TfDeleteFile(tmpLayerExportPath);
            }
        }
    }

    for (auto &fileSrcAndDestPath : fileCopyMap) {
        const std::string &srcPath = fileSrcAndDestPath.first;
        const std::string destPath = _GetDestRelativePath(
                fileSrcAndDestPath.second, destDir);
        TF_DEBUG(USDUTILS_CREATE_USDZ_PACKAGE).Msg(
            ".. adding file '%s' to package at path '%s'.\n", 
            srcPath.c_str(), destPath.c_str());

        if (!packagedFiles.insert(destPath).second) {
            TF_WARN("A file already exists at path \"%s\" in the package. "
                "Skipping copy of file \"%s\".", destPath.c_str(), 
                srcPath.c_str());
            continue;
        }

        // If the file is a package or inside a package, copy the
        // entire package. We could extract the package and copy only the 
        // dependencies, but this could get very complicated.
        if (ArIsPackageRelativePath(destPath)) {
            std::string packagePath = ArSplitPackageRelativePathOuter(
                    srcPath).first;
            std::string destPackagePath = ArSplitPackageRelativePathOuter(
                    destPath).first;
            if (!packagePath.empty()) {
                std::string inArchivePath = writer.AddFile(packagePath,
                    destPackagePath);
                if (inArchivePath.empty()) {
                    success = false;
                }
            }
        }
        else {
            std::string inArchivePath = writer.AddFile(srcPath, destPath);
            if (inArchivePath.empty()) {
                // XXX: Should we discard the usdz file and return early here?
                TF_WARN("Failed to add file '%s' to the package at path '%s'.",
                    srcPath.c_str(), usdzFilePath.c_str());
                success = false;
            }
        }
    }

    return writer.Save() && success;
}

bool
UsdUtilsCreateNewUsdzPackage(const SdfAssetPath &assetPath,
                             const std::string &usdzFilePath,
                             const std::string &firstLayerName)
{
    return _CreateNewUsdzPackage(assetPath, usdzFilePath, firstLayerName);
}

bool
UsdUtilsCreateNewARKitUsdzPackage(
    const SdfAssetPath &assetPath,
    const std::string &inUsdzFilePath,
    const std::string &firstLayerName)
{
    auto &resolver = ArGetResolver();

    std::string usdzFilePath = ArchNormPath(inUsdzFilePath);

    const std::string resolvedPath = resolver.Resolve(assetPath.GetAssetPath());
    if (resolvedPath.empty()) {
        return false;
    }
    
    // Check if the given asset has external dependencies that participate in 
    // the composition of the stage.
    std::vector<std::string> sublayers, references, payloads;
    _ExtractExternalReferences(resolvedPath, 
        _ReferenceTypesToInclude::CompositionOnly,
        &sublayers, &references, &payloads);

    // Ensure that the root layer has the ".usdc" extension.
    std::string targetBaseName = firstLayerName.empty() ? 
        TfGetBaseName(assetPath.GetAssetPath()) : firstLayerName;
    const std::string &fileExt = resolver.GetExtension(targetBaseName);
    bool renamingRootLayer = false;
    if (fileExt != UsdUsdcFileFormatTokens->Id) {
        renamingRootLayer = true;
        targetBaseName = targetBaseName.substr(0, targetBaseName.rfind(".")+1) +  
                UsdUsdcFileFormatTokens->Id.GetString();
    }

    // If there are no external dependencies needed for composition, we can 
    // invoke the regular packaging function.
    if (sublayers.empty() && references.empty() && payloads.empty()) {
        if (renamingRootLayer) {
            return _CreateNewUsdzPackage(assetPath, usdzFilePath, 
                    /*firstLayerName*/ targetBaseName, 
                    /* origRootFilePath*/ resolvedPath,
                    /* dependenciesToSkip */ {resolvedPath});
        } else {
            return _CreateNewUsdzPackage(assetPath, usdzFilePath, 
                /*firstLayerName*/ targetBaseName, 
                /* origRootFilePath*/ resolvedPath);
        }
    }

    TF_WARN("The given asset '%s' contains one or more composition arcs "
        "referencing external USD files. Flattening it to a single .usdc file "
        "before packaging. This will result in loss of features such as "
        "variantSets and all asset references to be absolutized.", 
        assetPath.GetAssetPath().c_str());

    const auto &usdStage = UsdStage::Open(resolvedPath);
    const std::string tmpFileName = 
            ArchMakeTmpFileName(targetBaseName, ".usdc");

    TF_DEBUG(USDUTILS_CREATE_USDZ_PACKAGE).Msg(
        "Flattening asset @%s@ located at '%s' to temporary layer at "
        "path '%s'.\n", assetPath.GetAssetPath().c_str(), resolvedPath.c_str(), 
        tmpFileName.c_str());

    if (!usdStage->Export(tmpFileName, /*addSourceFileComment*/ false)) {
        TF_WARN("Failed to flatten and export the USD stage '%s'.", 
            UsdDescribe(usdStage).c_str());
        return false;
    }

    bool success = _CreateNewUsdzPackage(SdfAssetPath(tmpFileName), 
        usdzFilePath, /* firstLayerName */ targetBaseName,
        /* origRootFilePath*/ resolvedPath,
        /*dependenciesToSkip*/ {resolvedPath});

    if (success) {
        TfDeleteFile(tmpFileName);
    } else {
        TF_WARN("Failed to create a .usdz package from temporary, flattened "
            "layer '%s'.", tmpFileName.c_str());;
    }

    return success;
}

bool
UsdUtilsComputeAllDependencies(const SdfAssetPath &assetPath,
                               std::vector<SdfLayerRefPtr> *layers,
                               std::vector<std::string> *assets,
                               std::vector<std::string> *unresolvedPaths)
{
    // We are not interested in localizing here, hence pass in the empty string
    // for destination directory.
    _AssetLocalizer localizer(assetPath, 
                              /* destDir */ std::string(), 
                              /* enableMetadataFiltering */ true);

    // Clear the vectors before we start.
    layers->clear();
    assets->clear();
    
    // Reserve space in the vectors.
    layers->reserve(localizer.GetLayerExportMap().size());
    assets->reserve(localizer.GetFileCopyMap().size());

    for (auto &layerAndDestPath : localizer.GetLayerExportMap()) {
        layers->push_back(layerAndDestPath.first);
    }

    for (auto &srcAndDestPath : localizer.GetFileCopyMap()) {
        assets->push_back(srcAndDestPath.first);
    }

    *unresolvedPaths = localizer.GetUnresolvedAssetPaths();

    // Return true if one or more layers or assets were added  to the results.
    return !layers->empty() || !assets->empty();
}

void 
UsdUtilsModifyAssetPaths(
        const SdfLayerHandle& layer,
        const UsdUtilsModifyAssetPathFn& modifyFn)
{
    _FileAnalyzer(layer,
        _ReferenceTypesToInclude::All,
        /* enableMetadataFiltering*/ false, 
        [&modifyFn](const std::string& assetPath, 
                    const SdfLayerRefPtr& layer) { 
            return modifyFn(assetPath);
        }
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
