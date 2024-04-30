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
/// \file usdUtils/assetLocalizationDelegate.cpp

#include "pxr/usd/usdUtils/assetLocalizationDelegate.h"

#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/usd/clipsAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

static
std::vector<std::string> 
_AllDependenciesForInfo(
    const UsdUtilsDependencyInfo &depInfo)
{
    const std::vector<std::string>& assetDeps = depInfo.GetDependencies();
    std::vector<std::string> dependencies;
    dependencies.reserve((assetDeps.size() + 1));
    dependencies.insert(dependencies.end(), assetDeps.begin(), assetDeps.end());
    dependencies.emplace_back(depInfo.GetAssetPath());

    return dependencies;
}

UsdUtilsDependencyInfo 
UsdUtils_ProcessedPathCache::GetProcessedInfo(
    const SdfLayerRefPtr &layer, 
    const UsdUtilsDependencyInfo &dependencyInfo,
    UsdUtils_DependencyType dependencyType)
{
    auto depKey = 
        std::make_tuple(layer->GetRealPath(), dependencyInfo.GetAssetPath());
    auto result = _cachedPaths.find(depKey);
    if (result == _cachedPaths.end()) {
        UsdUtilsDependencyInfo depInfo = _processingFunc(
            layer, dependencyInfo, dependencyType);

        _cachedPaths.insert(std::make_pair(depKey, depInfo.GetAssetPath()));

        return depInfo;
    }
    else {
        return UsdUtilsDependencyInfo(result->second);
    }
}

// Processes sublayer paths, removing duplicates and only updates the paths in
// the writable layer if the processed list differs from the source list.
std::vector<std::string> 
UsdUtils_WritableLocalizationDelegate::ProcessSublayers(
    const SdfLayerRefPtr &layer)
{
    SdfSubLayerProxy sublayerPaths = layer->GetSubLayerPaths();
    std::vector<std::string> processedPaths, dependencies;

    for (const std::string& sublayerPath : sublayerPaths) {
        UsdUtilsDependencyInfo depInfo(sublayerPath);
        UsdUtilsDependencyInfo info = _pathCache.GetProcessedInfo( 
            layer, depInfo, UsdUtils_DependencyType::Sublayer);

        if (info.GetAssetPath().empty()) {
            continue;
        }

        // duplicate paths are not allowed when calling SetSubLayerPaths
        auto existingValue = std::find(
            processedPaths.begin(), processedPaths.end(), info.GetAssetPath());
        if (existingValue != processedPaths.end()) {
            continue;
        }

        processedPaths.emplace_back(info.GetAssetPath());
        dependencies.emplace_back(info.GetAssetPath());
        dependencies.insert(dependencies.end(), 
            info.GetDependencies().begin(), info.GetDependencies().end());
    }

    if (processedPaths != sublayerPaths) {
        SdfLayerRefPtr writableLayer = _GetOrCreateWritableLayer(layer);
        
        if (writableLayer) {
            writableLayer->SetSubLayerPaths(processedPaths);
        }
    }

    return dependencies;
}

std::vector<std::string> 
UsdUtils_WritableLocalizationDelegate::ProcessPayloads(
    const SdfLayerRefPtr &layer,
    const SdfPrimSpecHandle &primSpec)
{
    return _ProcessReferencesOrPayloads
        <SdfPayloadListOp, UsdUtils_DependencyType::Payload>(
        layer, primSpec, SdfFieldKeys->Payload);
}

std::vector<std::string> 
UsdUtils_WritableLocalizationDelegate::ProcessReferences(
    const SdfLayerRefPtr &layer,
    const SdfPrimSpecHandle &primSpec)
{
    return _ProcessReferencesOrPayloads
        <SdfReferenceListOp, UsdUtils_DependencyType::Reference>(
        layer, primSpec, SdfFieldKeys->References);
}


// Processes references or payloads for a prim.
// Will only attempt to get a writable layer if asset path processing modifies
// existing list ops.
template <class ListOpType, UsdUtils_DependencyType DEP_TYPE>
std::vector<std::string> 
UsdUtils_WritableLocalizationDelegate::_ProcessReferencesOrPayloads(
    const SdfLayerRefPtr &layer,
    const SdfPrimSpecHandle &primSpec,
    const TfToken &listOpToken)
{
    std::vector<std::string> dependencies;
    ListOpType processedListOps;
    if (!primSpec->HasField(listOpToken, &processedListOps)) {
        return dependencies;
    }

    const bool modified = processedListOps.ModifyOperations(
        [this, &layer, &dependencies](
            const typename ListOpType::ItemType& item){
            return 
                _ProcessRefOrPayload <typename ListOpType::ItemType, DEP_TYPE>(
                    layer, item, &dependencies);
        },
        /*removeDuplicates*/ true
    );

    if (!modified) {
        return dependencies;
    }

    SdfLayerRefPtr writableLayer = _GetOrCreateWritableLayer(layer);

    if (!writableLayer) {
        return dependencies;
    }

    SdfPrimSpecHandle writablePrim = 
        writableLayer->GetPrimAtPath(primSpec->GetPath());
    
    if (processedListOps.HasKeys()) {
        writablePrim->SetField(listOpToken, processedListOps);
    } else {
        writablePrim->ClearField(listOpToken);
    }

    return dependencies;
}

template <class RefOrPayloadType, UsdUtils_DependencyType DEP_TYPE>
std::optional<RefOrPayloadType>
UsdUtils_WritableLocalizationDelegate::_ProcessRefOrPayload(
    const SdfLayerRefPtr &layer,
    const RefOrPayloadType& refOrPayload,
    std::vector<std::string>* dependencies)
{
    // If the asset path is empty this is a local payload. We can ignore
    // these since they refer to the same layer where the payload was
    // authored.
    if (refOrPayload.GetAssetPath().empty()) {
        return std::optional<RefOrPayloadType>(refOrPayload);
    }

    UsdUtilsDependencyInfo depInfo(refOrPayload.GetAssetPath());
    const UsdUtilsDependencyInfo info = _pathCache.GetProcessedInfo( 
        layer, depInfo, DEP_TYPE);

    if (info.GetAssetPath().empty()) {
        return std::nullopt;
    }

    RefOrPayloadType processedRefOrPayload = refOrPayload;
    processedRefOrPayload.SetAssetPath(info.GetAssetPath());

    // Add the processed info to the list of paths the system will need
    // to further traverse
    dependencies->push_back(info.GetAssetPath());
    dependencies->insert(dependencies->end(), 
        info.GetDependencies().begin(), info.GetDependencies().end());

    return std::optional<RefOrPayloadType>(processedRefOrPayload);
}

// When beginning to process a value, if the value is a dictionary, explicitly
// make a copy of it.  As asset paths are encountered and updated, they will be
// updated in this copied dictionary.  We will only get callbacks for asset 
// related keys, so other properties will be left unaffected.
void
UsdUtils_WritableLocalizationDelegate::BeginProcessValue(
    const SdfLayerRefPtr &layer,
    const VtValue &val)
{
    if (val.IsHolding<VtDictionary>()) {
        _currentValueDictionary = val.UncheckedGet<VtDictionary>();
    }
}

std::vector<std::string> 
UsdUtils_WritableLocalizationDelegate::ProcessValuePath(
    const SdfLayerRefPtr &layer,
    const std::string &keyPath,
    const std::string &authoredPath,
    const std::vector<std::string> &dependencies)
{
    UsdUtilsDependencyInfo depInfo = {authoredPath, dependencies};
    UsdUtilsDependencyInfo info = _pathCache.GetProcessedInfo(
        layer, depInfo, UsdUtils_DependencyType::Reference);

    const std::string relativeKeyPath = _GetRelativeKeyPath(keyPath);

    if (relativeKeyPath.empty()) {
        _currentValuePath = SdfAssetPath(info.GetAssetPath());
    }
    else if (info.GetAssetPath().empty()){
        _currentValueDictionary.EraseValueAtPath(relativeKeyPath);
        return {};
    } else {
        _currentValueDictionary.SetValueAtPath(
            relativeKeyPath, VtValue(SdfAssetPath(info.GetAssetPath())));
    }

    return _AllDependenciesForInfo(info);
}

std::vector<std::string> 
UsdUtils_WritableLocalizationDelegate::ProcessValuePathArrayElement(
    const SdfLayerRefPtr &layer,
    const std::string &keyPath,
    const std::string &authoredPath,
    const std::vector<std::string> &dependencies)
{
    UsdUtilsDependencyInfo depInfo = {authoredPath, dependencies};
    const UsdUtilsDependencyInfo info = _pathCache.GetProcessedInfo(
        layer, depInfo, UsdUtils_DependencyType::Reference);
    
    if (!info.GetAssetPath().empty()) {
        _currentPathArray.emplace_back(info.GetAssetPath());
        return _AllDependenciesForInfo(info);
    }
    else {
        return {};
    }
    
}

void 
UsdUtils_WritableLocalizationDelegate::EndProcessingValuePathArray(
    const SdfLayerRefPtr &layer,
    const std::string &keyPath)
{
    const std::string relativeKeyPath = _GetRelativeKeyPath(keyPath);

    if (relativeKeyPath.empty()) {
        _currentValuePathArray = std::move(_currentPathArray);
    }
    else if (_currentPathArray.empty()){
        _currentValueDictionary.EraseValueAtPath(relativeKeyPath);
    } else {
        _currentValueDictionary.SetValueAtPath(relativeKeyPath, 
            _currentPathArray.empty() ? 
            VtValue() : 
            VtValue::Take(_currentPathArray));
    }

    _currentPathArray.clear();
}

VtValue 
UsdUtils_WritableLocalizationDelegate::_GetUpdatedValue(
    const VtValue &val)
{
    VtValue updatedValue;

    if (val.IsHolding<SdfAssetPath>()) {
        auto originalAssetPath = val.UncheckedGet<SdfAssetPath>();

        updatedValue = 
            _currentValuePath.GetAssetPath().empty() && 
                !originalAssetPath.GetAssetPath().empty() ?
                    VtValue() :
                    VtValue::Take(_currentValuePath);
        } else if (val.IsHolding<VtArray<SdfAssetPath>>()) {
            const VtArray<SdfAssetPath>& originalArray = 
                val.UncheckedGet< VtArray<SdfAssetPath> >();

        updatedValue = 
            _currentValuePathArray.empty() && !originalArray.empty()?
            VtValue() : 
            VtValue::Take(_currentValuePathArray);
    }
    else if (val.IsHolding<VtDictionary>()){
        const VtDictionary& originalDict = val.UncheckedGet<VtDictionary>();

        updatedValue = 
            _currentValueDictionary.empty() && !originalDict.empty() ?
            VtValue() :
            VtValue::Take(_currentValueDictionary);
    }

    return updatedValue;
}
 
void 
UsdUtils_WritableLocalizationDelegate::EndProcessValue(
    const SdfLayerRefPtr &layer,
    const SdfPath &path,
    const TfToken &key,
    const VtValue &val)
{
    VtValue updatedValue = _GetUpdatedValue(val);

    if (updatedValue == val) {
        return;
    }

    SdfLayerRefPtr writableLayer = _GetOrCreateWritableLayer(layer);

    if (writableLayer) {
        if (updatedValue.IsEmpty()) {
            writableLayer->EraseField(path, key);
        }
        else if (val != updatedValue) {
            writableLayer->SetField(path, key, updatedValue);
        }
    }
}

void 
UsdUtils_WritableLocalizationDelegate::EndProcessTimeSampleValue(
        const SdfLayerRefPtr &layer,
        const SdfPath &path,
        double t,
        const VtValue &val)
{
    VtValue updatedValue = _GetUpdatedValue(val);

    if (updatedValue == val) {
        return;
    } 
    
    SdfLayerRefPtr writableLayer = _GetOrCreateWritableLayer(layer);

    if (writableLayer) {
        if (updatedValue.IsEmpty()) {
            writableLayer->EraseTimeSample(path, t);
        } else {
            writableLayer->SetTimeSample(path, t, updatedValue);
        }
    }
}

std::vector<std::string>
UsdUtils_WritableLocalizationDelegate::ProcessClipTemplateAssetPath(
    const SdfLayerRefPtr &layer,
    const SdfPrimSpecHandle &primSpec,
    const std::string &clipSetName,
    const std::string &templateAssetPath,
    std::vector<std::string> dependencies)
{
    UsdUtilsDependencyInfo depInfo = {templateAssetPath, dependencies};
    const UsdUtilsDependencyInfo info = _pathCache.GetProcessedInfo(layer,
        depInfo, UsdUtils_DependencyType::Reference);

    if (info.GetAssetPath() == templateAssetPath) {
        return _AllDependenciesForInfo(info);
    }

    SdfLayerRefPtr writableLayer = _GetOrCreateWritableLayer(layer);
    if (!writableLayer) {
        return _AllDependenciesForInfo(info);
    }

    SdfPrimSpecHandle writablePrim = 
        writableLayer->GetPrimAtPath(primSpec->GetPath());

    VtValue clipsValue = writablePrim->GetInfo(UsdTokens->clips);
    VtDictionary clipsDict = clipsValue.UncheckedGet<VtDictionary>();
    const std::string keyPath = 
        clipSetName + ":" + UsdClipsAPIInfoKeys->templateAssetPath.GetString();

    clipsDict.SetValueAtPath(keyPath, VtValue(info.GetAssetPath()), ":");

    writablePrim->SetInfo(UsdTokens->clips, VtValue(clipsDict));

    return _AllDependenciesForInfo(info);
}

std::string 
UsdUtils_WritableLocalizationDelegate::_GetRelativeKeyPath(
    const std::string& fullPath) 
{
    std::string::size_type pos = fullPath.find_first_of(':');

    if (pos == std::string::npos) {
        return fullPath;
    } else {
        return fullPath.substr(pos + 1);
    }
}

SdfLayerRefPtr 
UsdUtils_WritableLocalizationDelegate::_GetOrCreateWritableLayer(
    const SdfLayerRefPtr& layer)
{
    if (!layer ) {
        return nullptr;
    }

    // We do not allow writing to package layers or layers contained within
    // existing packages. Doing so would require us to expand and rebuild
    // the existing package.
    if (layer->GetFileFormat()->IsPackage() ||
        ArIsPackageRelativePath(layer->GetIdentifier())) {
        TF_CODING_ERROR("Unable to edit asset path in package layer: %s",
            layer->GetIdentifier().c_str());
        return nullptr;
    }

    if (_editLayersInPlace) {
        return layer;
    }

    auto result = 
        _layerCopyMap.insert(std::make_pair(layer, layer));

    // a writable layer already exists
    if (!result.second) {
        return result.first->second;
    }

    SdfLayerRefPtr copiedLayer = SdfLayer::CreateAnonymous( 
        layer->GetDisplayName(),layer->GetFileFormat(), 
        layer->GetFileFormatArguments());
    copiedLayer->TransferContent(layer);

    result.first->second = copiedLayer;

    return copiedLayer;
}

SdfLayerConstHandle
UsdUtils_WritableLocalizationDelegate::GetLayerUsedForWriting(
    const SdfLayerRefPtr& layer) 
{
    if (_editLayersInPlace || !layer) {
        return layer;
    }

    auto result = _layerCopyMap.find(layer);

    if (result != _layerCopyMap.end()) {
        return result->second;
    }

    return layer;
}

void
UsdUtils_WritableLocalizationDelegate::ClearLayerUsedForWriting(
    const SdfLayerRefPtr& layer)
{
    _layerCopyMap.erase(layer);
}

std::vector<std::string> 
UsdUtils_ReadOnlyLocalizationDelegate::ProcessSublayers(
    const SdfLayerRefPtr &layer)
{
    std::vector<std::string> dependencies;

    for (const auto &path : layer->GetSubLayerPaths()) {
        UsdUtilsDependencyInfo info = _pathCache.GetProcessedInfo(
            layer, {path, {}}, UsdUtils_DependencyType::Sublayer);

        if (info.GetAssetPath().empty()) {
            continue;
        }

        dependencies.emplace_back(info.GetAssetPath());
        dependencies.insert(dependencies.end(), 
            info.GetDependencies().begin(), info.GetDependencies().end());
    }

    return {};
}

std::vector<std::string>
UsdUtils_ReadOnlyLocalizationDelegate::ProcessPayloads(
    const SdfLayerRefPtr &layer,
    const SdfPrimSpecHandle &primSpec)
{
    return ProcessReferencesOrPayloads
        <SdfPayload, UsdUtils_DependencyType::Payload>(
            layer, primSpec->GetPayloadList().GetAppliedItems());
}

std::vector<std::string> 
UsdUtils_ReadOnlyLocalizationDelegate::ProcessReferences(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec)
{
    return ProcessReferencesOrPayloads
        <SdfReference, UsdUtils_DependencyType::Reference>(
            layer, primSpec->GetReferenceList().GetAppliedItems());
}


template <typename RefOrPayloadType, UsdUtils_DependencyType dependencyType>
std::vector<std::string> 
UsdUtils_ReadOnlyLocalizationDelegate::ProcessReferencesOrPayloads(
    const SdfLayerRefPtr &layer,
    const std::vector<RefOrPayloadType>& appliedItems)
{
    std::vector<std::string> dependencies;

    for (const auto& refOrPayload: appliedItems) {
        // If the asset path is empty this is a local reference or payload.
        // We can ignore these since they refer to the same layer where it was 
        // authored.
        if (refOrPayload.GetAssetPath().empty()) {
            continue;
        }

        UsdUtilsDependencyInfo info = _pathCache.GetProcessedInfo(layer, 
            {refOrPayload.GetAssetPath(), {}}, dependencyType);

        if (info.GetAssetPath().empty()) {
            continue;
        }

        dependencies.emplace_back(info.GetAssetPath());
        dependencies.insert(dependencies.end(), 
            info.GetDependencies().begin(), info.GetDependencies().end());
    }

    return dependencies;
}

std::vector<std::string>
UsdUtils_ReadOnlyLocalizationDelegate::ProcessValuePath(
    const SdfLayerRefPtr &layer,
    const std::string &keyPath,
    const std::string &authoredPath,
    const std::vector<std::string> &dependencies)
{
    return _AllDependenciesForInfo(_pathCache.GetProcessedInfo(layer, 
        {authoredPath, dependencies}, UsdUtils_DependencyType::Reference));
}

std::vector<std::string>
UsdUtils_ReadOnlyLocalizationDelegate::ProcessValuePathArrayElement(
    const SdfLayerRefPtr &layer,
    const std::string &keyPath,
    const std::string &authoredPath,
    const std::vector<std::string> &dependencies)
{    
    return _AllDependenciesForInfo(_pathCache.GetProcessedInfo(layer, 
        {authoredPath, dependencies}, UsdUtils_DependencyType::Reference));
}

std::vector<std::string>
UsdUtils_ReadOnlyLocalizationDelegate::ProcessClipTemplateAssetPath(
    const SdfLayerRefPtr &layer,
    const SdfPrimSpecHandle &primSpec,
    const std::string &clipSetName,
    const std::string &templateAssetPath,
    std::vector<std::string> dependencies)
{
    return _AllDependenciesForInfo(_pathCache.GetProcessedInfo(
        layer, {templateAssetPath, dependencies}, 
        UsdUtils_DependencyType::ClipTemplateAssetPath));
}

PXR_NAMESPACE_CLOSE_SCOPE
