//
// Copyright 2023 Pixar
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
/// \file usdUtils/assetLocalization.cpp
#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/assetLocalization.h"
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
#include "pxr/usd/usdShade/udimUtils.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/trace/trace.h"

#include <stack>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

UsdUtils_FileAnalyzer::UsdUtils_FileAnalyzer(const std::string &referencePath,
                  const std::string &resolvedPath,
                  UsdUtils_FileAnalyzer::ReferenceType refTypesToInclude,
                  bool enableMetadataFiltering,
                  const RemapAssetPathFunc &remapPathFunc,
                  const ProcessAssetPathFunc &processPathFunc) : 
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
    UsdUtils_FileAnalyzer::UsdUtils_FileAnalyzer(const SdfLayerHandle& layer,
                  UsdUtils_FileAnalyzer::ReferenceType refTypesToInclude,
                  bool enableMetadataFiltering,
                  const RemapAssetPathFunc &remapPathFunc,
                  const ProcessAssetPathFunc &processPathFunc) : 
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

std::string 
UsdUtils_FileAnalyzer::_ProcessDependency(const std::string &rawRefPath,
                                  const DependencyType &DependencyType)
{
    if (_processPathFunc) {
        _processPathFunc(rawRefPath, GetLayer(), DependencyType);
    }

    return _RemapDependency(rawRefPath);
}

std::string 
UsdUtils_FileAnalyzer::_RemapDependency(const std::string &rawRefPath) {
    if (_remapPathFunc) {
        return _remapPathFunc(rawRefPath, GetLayer());
    }

    // Return the raw reference path if there's no asset path remapping 
    // function.
    return rawRefPath;
}

VtValue 
UsdUtils_FileAnalyzer::_UpdateAssetValue(const VtValue &val)
{
    return _UpdateAssetValue(
        std::string(),
        val,
        [](const std::string &) { return true; }
    );
}

VtValue 
UsdUtils_FileAnalyzer::_UpdateAssetValue(const std::string &key, 
                            const VtValue &val, 
                            const ShouldProcessAssetValueFunc shouldProcessFunc)
{
     if (val.IsHolding<SdfAssetPath>()) {
        auto assetPath = val.UncheckedGet<SdfAssetPath>();
        const std::string& rawAssetPath = assetPath.GetAssetPath();
        if (!rawAssetPath.empty()) {
            const std::string remappedPath = shouldProcessFunc(key)
                ? _ProcessDependency(rawAssetPath, DependencyType::Reference)
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
                    ? _ProcessDependency(rawAssetPath, DependencyType::Reference)
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
UsdUtils_FileAnalyzer::_ProcessSublayers()
{
    if (_remapPathFunc) {
        _layer->GetSubLayerPaths().ModifyItemEdits(
            [this](const std::string& path) {
                std::string remappedPath =
                    _ProcessDependency(path, DependencyType::Sublayer); 
                return remappedPath.empty() ? 
                    boost::optional<std::string>() : 
                    boost::optional<std::string>(std::move(remappedPath));
            });
    } else {
        for (const auto &subLayer: _layer->GetSubLayerPaths()) {
            _ProcessDependency(subLayer, DependencyType::Sublayer);
        }
    }
}

template <class RefOrPayloadType, UsdUtils_FileAnalyzer::DependencyType DEP_TYPE>
boost::optional<RefOrPayloadType> 
UsdUtils_FileAnalyzer::_RemapRefOrPayload(const RefOrPayloadType &refOrPayload)
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
UsdUtils_FileAnalyzer::_ProcessPayloads(const SdfPrimSpecHandle &primSpec)
{
    if (_remapPathFunc) {
        primSpec->GetPayloadList().ModifyItemEdits(
            [this](const SdfPayload& payload) {
                return _RemapRefOrPayload<SdfPayload, DependencyType::Payload>(
                    payload);
            });
    } else {
        for (SdfPayload const& payload:
             primSpec->GetPayloadList().GetAddedOrExplicitItems()) {

            // If the asset path is empty this is a local payload. We can ignore
            // these since they refer to the same layer where the payload was
            // authored.
            if (!payload.GetAssetPath().empty()) {
                _ProcessDependency(payload.GetAssetPath(), 
                    DependencyType::Payload);
            }
        }
    }
}

void
UsdUtils_FileAnalyzer::_ProcessProperties(const SdfPrimSpecHandle &primSpec)
{
    // Include external references in property values and metadata only if 
    // the client is interested in all reference types. i.e. return early if 
    // _refTypesToInclude is CompositionOnly.
    if (_refTypesToInclude == 
            UsdUtils_FileAnalyzer::ReferenceType::CompositionOnly)
        return;

    // XXX:2016-04-14 Note that we use the field access API
    // here rather than calling GetAttributes, as creating specs for
    // large numbers of attributes, most of which are *not* asset
    // path-valued and therefore not useful here, is expensive.
    //
    const VtValue propertyNames =
        primSpec->GetField(SdfChildrenKeys->PropertyChildren);

    if (!propertyNames.IsHolding<std::vector<TfToken>>()) {
        return;
    }

    for (const auto& name : propertyNames.UncheckedGet<std::vector<TfToken>>()) {
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
UsdUtils_FileAnalyzer::_ProcessMetadata(const SdfPrimSpecHandle &primSpec)
{
    if (_refTypesToInclude == UsdUtils_FileAnalyzer::ReferenceType::All) {
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
                        _ProcessDependency(rawClipRef, 
                            DependencyType::Reference);
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
UsdUtils_FileAnalyzer::_ProcessReferences(const SdfPrimSpecHandle &primSpec)
{
    if (_remapPathFunc) {
        primSpec->GetReferenceList().ModifyItemEdits(
            [this](const SdfReference& ref) {
                return _RemapRefOrPayload<SdfReference, 
                    DependencyType::Reference>(ref);
            });
    } else {
        for (SdfReference const& ref:
            primSpec->GetReferenceList().GetAddedOrExplicitItems()) {

            // If the asset path is empty this is a local reference. We can
            // ignore these since they refer to the same layer where the
            // reference was authored.
            if (!ref.GetAssetPath().empty()) {
                _ProcessDependency(ref.GetAssetPath(), 
                    DependencyType::Reference);
            }
        }
    }
}

void
UsdUtils_FileAnalyzer::_AnalyzeDependencies()
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

class UsdUtils_AssetLocalizer::_DirectoryRemapper {
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

UsdUtils_AssetLocalizer::UsdUtils_AssetLocalizer(const SdfAssetPath &assetPath, 
                    const std::string &destDir,
                    bool enableMetadataFiltering,
                    const std::string &firstLayerName,
                    const std::string &origRootFilePath,
                    const std::vector<std::string> &dependenciesToSkip)
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
            UsdUtils_FileAnalyzer::DependencyType DependencyType) {
        layerDependenciesMap[layer].push_back(ap);
    };

    // If destination directory is an empty string, skip any remapping
    // of asset paths.
    const auto remapAssetPathFunc = destDir.empty() ?
        UsdUtils_FileAnalyzer::RemapAssetPathFunc() : 
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
        filesToLocalize.emplace(destFilePath, UsdUtils_FileAnalyzer(
                layer,
                /*refTypesToInclude*/ UsdUtils_FileAnalyzer::ReferenceType::All,
                /*enableMetadataFiltering*/ enableMetadataFiltering,
                remapAssetPathFunc, processPathFunc));
    }
    else
    {
        seenFiles.insert(rootFilePath);
        std::string destFilePath = TfStringCatPaths(destDir, 
                TfGetBaseName(rootFilePath));
        filesToLocalize.emplace(destFilePath, UsdUtils_FileAnalyzer(
                assetPathStr, rootFilePath, 
                /*refTypesToInclude*/ UsdUtils_FileAnalyzer::ReferenceType::All,
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

            filesToLocalize.emplace(destFilePathForRef, UsdUtils_FileAnalyzer(
                refAssetPath, resolvedRefFilePath,
                /* refTypesToInclude */ UsdUtils_FileAnalyzer::ReferenceType::All,
                /*enableMetadataFiltering*/ enableMetadataFiltering,
                remapAssetPathFunc, processPathFunc));
        }
    }
}

void UsdUtils_AssetLocalizer::_ResolveUdimPaths(
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

std::string 
UsdUtils_AssetLocalizer::_RemapAssetPath(const std::string &refPath, 
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

void
UsdUtils_ExtractExternalReferences(
    const std::string& filePath,
    const UsdUtils_FileAnalyzer::ReferenceType &refTypesToInclude,
    std::vector<std::string>* subLayers,
    std::vector<std::string>* references,
    std::vector<std::string>* payloads)
{
    auto &resolver = ArGetResolver();

    const auto processPathFunc = [&subLayers, &references, &payloads](
            const std::string &assetPath, const SdfLayerRefPtr &layer,
            const UsdUtils_FileAnalyzer::DependencyType &dependencyType) 
        {
            switch(dependencyType) {
                case UsdUtils_FileAnalyzer::DependencyType::Reference:
                    references->push_back(assetPath);
                    break;

                case UsdUtils_FileAnalyzer::DependencyType::Sublayer:
                    subLayers->push_back(assetPath);
                    break;

                    case UsdUtils_FileAnalyzer::DependencyType::Payload:
                    payloads->push_back(assetPath);
                    break;
            }
        };

    // We only care about knowing what the dependencies are. Hence, set 
    // remapPathFunc to empty.
    if (SdfLayer::IsAnonymousLayerIdentifier(filePath)) {
        UsdUtils_FileAnalyzer(SdfLayer::Find(filePath), refTypesToInclude,
            /*enableMetadataFiltering*/ false,
            /*remapPathFunc*/ {}, 
            processPathFunc);
    }
    else {
        UsdUtils_FileAnalyzer(filePath, resolver.Resolve(filePath), 
            refTypesToInclude,
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

PXR_NAMESPACE_CLOSE_SCOPE
