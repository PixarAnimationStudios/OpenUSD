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

#include "pxr/base/arch/regex.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/trace/trace.h"

#include <stack>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

bool 
UsdUtils_LocalizationContext::Process(
    const SdfLayerRefPtr &layer)
{
    if (!layer) {
        TF_CODING_ERROR("Unable to process null layer");
        return false;
    }

    _rootLayer = layer;

    _encounteredPaths.insert(_rootLayer->GetIdentifier());
    _ProcessLayer(_rootLayer);

    while (!_queue.empty()) {
        std::string anchoredPath = _queue.back();
        _queue.pop_back();

        if (!UsdStage::IsSupportedFile(anchoredPath)) {
            continue;
        }

        SdfLayerRefPtr layer = SdfLayer::FindOrOpen(anchoredPath);
        if (layer) {
            _ProcessLayer(layer);
        }
    }

    return true;
}

void
UsdUtils_LocalizationContext::_EnqueueDependencies(
    const SdfLayerRefPtr layer,
    const std::vector<std::string> &dependencies)
{
    for (const auto &dependency : dependencies) {
        _EnqueueDependency(layer, dependency);
    }
}

void 
UsdUtils_LocalizationContext::_EnqueueDependency(
    const SdfLayerRefPtr layer,
    const std::string &assetPath) 
{
    if (!_recurseLayerDependencies || assetPath.empty()) {
        return;
    }

    const std::string anchoredPath = 
        SdfComputeAssetPathRelativeToLayer(layer, assetPath);

    if (_encounteredPaths.count(anchoredPath) > 0 || 
        _dependenciesToSkip.count(anchoredPath)) {
        return;
    }

    ArResolvedPath resolvedPath = ArGetResolver().Resolve(anchoredPath);
    if (resolvedPath.empty()) {
        TF_WARN("Failed to resolve reference @%s@ with computed asset path "
            "@%s@ found in layer @%s@.",
            assetPath.c_str(),
            anchoredPath.c_str(),
            layer->GetRealPath().c_str());
        return;
    }

    _encounteredPaths.insert(anchoredPath);

    _queue.emplace_back(anchoredPath);
}

void 
UsdUtils_LocalizationContext::_ProcessLayer(
    const SdfLayerRefPtr& layer ) 
{
    _ProcessSublayers(layer);

    std::stack<SdfPrimSpecHandle> dfs;
    dfs.push(layer->GetPseudoRoot());

    while (!dfs.empty()) {
        SdfPrimSpecHandle curr = dfs.top();
        dfs.pop();

        // Metadata is processed even on the pseudoroot, which ensures
        // we process layer metadata properly.
        _ProcessMetadata(layer, curr);
        if (curr != layer->GetPseudoRoot()) {
            _ProcessPayloads(layer, curr);    
            _ProcessProperties(layer, curr);
            _ProcessReferences(layer, curr);
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

void 
UsdUtils_LocalizationContext::_ProcessSublayers(
    const SdfLayerRefPtr& layer)
{
    SdfSubLayerProxy sublayers = layer->GetSubLayerPaths();

    if (sublayers.size() == 0) {
        return;
    }

    for (const auto& sublayerPath : sublayers) {
        _EnqueueDependency(layer, sublayerPath);
    }
    
    const std::vector<std::string> processedDeps = 
        _delegate->ProcessSublayers(layer);
    _EnqueueDependencies(layer, processedDeps);

}

static 
std::vector<std::string> 
_GetClipSets(
    const SdfPrimSpecHandle &primSpec)
{
    std::vector<std::string> clipSets;
    VtValue clipsValue = primSpec->GetInfo(UsdTokens->clips);

    if (clipsValue.IsEmpty() || !clipsValue.IsHolding<VtDictionary>()) {
        return clipSets;
    }

    const VtDictionary& clipsDict = clipsValue.UncheckedGet<VtDictionary>();

    for (auto &clipSet : clipsDict) {
        if (clipSet.second.IsHolding<VtDictionary>()) {
            clipSets.emplace_back(clipSet.first);
        }
    }

    return clipSets;
}

static
std::string
_GetTemplateAssetPathForClipSet(
    const SdfPrimSpecHandle &primSpec,
    const std::string &clipSetName)
{
    const VtDictionary clipsDict = 
        primSpec->GetInfo(UsdTokens->clips).UncheckedGet<VtDictionary>();

    const std::string keyPath = 
        clipSetName + ":" + UsdClipsAPIInfoKeys->templateAssetPath.GetString();

    VtValue const * value = clipsDict.GetValueAtPath(keyPath);

    if (value){
        return value->UncheckedGet<std::string>();
    }
    else {
        return {};
    }
}


void
UsdUtils_LocalizationContext::_ProcessMetadata(
    const SdfLayerRefPtr& layer,
    const SdfPrimSpecHandle &primSpec)
{
    if (_refTypesToInclude == ReferenceType::All) {
        for (const TfToken& infoKey : primSpec->GetMetaDataInfoKeys()) {
            VtValue value = primSpec->GetInfo(infoKey);

            if (!_ValueTypeIsRelevant(value)) {
                continue;
            }

            _delegate->BeginProcessValue(layer, value);

            _ProcessAssetValue(layer, infoKey, value, 
                /*processingMetadata*/ true);
            _delegate->EndProcessValue(
                layer, primSpec->GetPath(), infoKey, value);
        }
    }

    // Process clips["templateAssetPath"], which is a string value 
    // containing one or more #'s. See 
    // UsdClipsAPI::GetClipTemplateAssetPath for details.
    auto clipSets = _GetClipSets(primSpec);

    for (const auto& clipSet : clipSets) {
        std::string templatePath = 
            _GetTemplateAssetPathForClipSet(primSpec, clipSet);

        if (templatePath.empty()) {
            continue;
        }

        const std::vector<std::string> clipFiles = 
            _GetTemplatedClips(layer, templatePath);

        const std::vector<std::string> dependencies =
            _delegate->ProcessClipTemplateAssetPath(layer, primSpec, clipSet, 
                templatePath, clipFiles);

        _EnqueueDependencies(layer, dependencies);
    }
}


// XXX: In the future it may be worth investigating if _DeriveClipInfo from
// clipSetDefinition may be leveraged here for a more robust approach.
std::vector<std::string> 
UsdUtils_LocalizationContext::_GetTemplatedClips(
    const SdfLayerRefPtr& layer,
    const std::string &templateAssetPath)
{
    const std::string clipsDir = TfGetPathName(templateAssetPath);
    // Resolve clipsDir relative to this layer. 
    if (clipsDir.empty()) {
        TF_WARN("Invalid template asset path '%s'.",
            templateAssetPath.c_str());
        return std::vector<std::string>();
    }
    const std::string clipsDirAssetPath = 
        SdfComputeAssetPathRelativeToLayer(layer, clipsDir);

    // This acts as a guard against non-filesystem based resolvers
    if (!TfIsDir(clipsDirAssetPath)) {
        TF_WARN("Clips directory '%s' is not a valid directory "
            "on the filesystem.", clipsDirAssetPath.c_str());
        return std::vector<std::string>();
    }

    std::string clipsBaseName = TfGetBaseName(templateAssetPath);
    std::string globPattern = TfStringCatPaths(
            clipsDirAssetPath, TfStringReplace(clipsBaseName, "#", "*"));
    std::vector<std::string> clipAssetRefs = TfGlob(globPattern);

    if (clipAssetRefs.size() == 1 && clipAssetRefs[0] == globPattern) {
        clipAssetRefs.clear();
    }

    const auto fixupRefFunc = 
        [&clipsDirAssetPath, &clipsDir](const std::string & clipAsset) {
            // Reconstruct the raw, unresolved clip reference, for 
            // which the dependency must be processed.
            // 
            // clipsDir contains a '/' in the end, but 
            // clipsDirAssetPath does not. Hence, add a '/' to 
            // clipsDirAssetPath before doing the replace.
            return TfStringReplace(clipAsset, clipsDirAssetPath + '/', clipsDir);
    };

    std::transform(clipAssetRefs.begin(), clipAssetRefs.end(), 
                        clipAssetRefs.begin(), fixupRefFunc);

    return clipAssetRefs;
}

void 
UsdUtils_LocalizationContext::_ProcessPayloads(
    const SdfLayerRefPtr& layer,
    const SdfPrimSpecHandle &primSpec)
{
    SdfPayloadsProxy payloads = primSpec->GetPayloadList();
    if (!payloads.HasKeys()) {
        return;
    }

    for (auto const& payload : payloads.GetAppliedItems()) {
        if (!payload.GetAssetPath().empty()) {
            _EnqueueDependency(layer, payload.GetAssetPath());
        }
    }

    const std::vector<std::string> processedDeps = 
        _delegate->ProcessPayloads(layer, primSpec);
    _EnqueueDependencies(layer, processedDeps);
}

void
UsdUtils_LocalizationContext::_ProcessReferences(
    const SdfLayerRefPtr& layer,
    const SdfPrimSpecHandle &primSpec)
{
    SdfReferencesProxy references = primSpec->GetReferenceList();
    if (!references.HasKeys()) {
        return;
    }

    for (SdfReference const& reference : references.GetAppliedItems()) {
        if (!reference.GetAssetPath().empty()) {
            _EnqueueDependency(layer, reference.GetAssetPath());
        }
    }

    const std::vector<std::string> processedDeps = 
        _delegate->ProcessReferences(layer, primSpec);
    _EnqueueDependencies(layer, processedDeps);
}

void
UsdUtils_LocalizationContext::_ProcessProperties(
    const SdfLayerRefPtr& layer,
    const SdfPrimSpecHandle &primSpec)
{
    // Include external references in property values and metadata only if 
    // the client is interested in all reference types. i.e. return early if 
    // _refTypesToInclude is CompositionOnly.
    if (_refTypesToInclude == ReferenceType::CompositionOnly)
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
        for (const TfToken& infoKey : layer->ListFields(path)) {
            if (infoKey != SdfFieldKeys->Default &&
                infoKey != SdfFieldKeys->TimeSamples) {
                VtValue value = layer->GetField(path, infoKey);

                if (!_ValueTypeIsRelevant(value)) {
                    continue;
                }

                _delegate->BeginProcessValue(layer, value);
                _ProcessAssetValue(layer, value);
                _delegate->EndProcessValue(layer, path, infoKey, value);
                
            }
        }

        // Check property existence
        const VtValue vtTypeName =
            layer->GetField(path, SdfFieldKeys->TypeName);
        if (!vtTypeName.IsHolding<TfToken>()) {
            continue;
        }

        const TfToken typeName = vtTypeName.UncheckedGet<TfToken>();
        if (typeName == SdfValueTypeNames->Asset ||
            typeName == SdfValueTypeNames->AssetArray) {

            // Check default value
            VtValue defValue = layer->GetField(path, SdfFieldKeys->Default);

            if (_ValueTypeIsRelevant(defValue)) {
                _delegate->BeginProcessValue(layer, defValue);
                _ProcessAssetValue(layer, defValue);
                _delegate->EndProcessValue(
                        layer, path, SdfFieldKeys->Default, defValue);
            }

            // Check timeSample values
            for (double t : layer->ListTimeSamplesForPath(path)) {
                VtValue timeSampleVal;
                if (layer->QueryTimeSample(path, t, &timeSampleVal)) {
                    if (!_ValueTypeIsRelevant(timeSampleVal)) {
                        continue;
                    }

                    _delegate->BeginProcessValue(layer, timeSampleVal);
                    _ProcessAssetValue(layer, timeSampleVal);
                    _delegate->EndProcessTimeSampleValue(
                            layer, path, t, timeSampleVal);
                }
            }
        }
    }
}

void 
UsdUtils_LocalizationContext::_ProcessAssetValue(
    const SdfLayerRefPtr& layer,
    const VtValue &val,
    bool processingMetadata)
{
    _ProcessAssetValue(layer, std::string(), val, processingMetadata);
}

void
UsdUtils_LocalizationContext::_ProcessAssetValue(
    const SdfLayerRefPtr& layer,
    const std::string &keyPath,
    const VtValue &val,
    bool processingMetadata) 
{
    if (_ShouldFilterAssetPath(keyPath, processingMetadata)) {
        return;
    }

    if (val.IsHolding<SdfAssetPath>()) {
        auto assetPath = val.UncheckedGet<SdfAssetPath>();
        const std::string& rawAssetPath = assetPath.GetAssetPath();

        const std::vector<std::string> dependencies = 
            _GetDependencies(layer, rawAssetPath);

        const std::vector<std::string> processedDeps = 
            _delegate->ProcessValuePath(
                    layer, keyPath, rawAssetPath, dependencies);
        
        _EnqueueDependency(layer, rawAssetPath);
        _EnqueueDependencies(layer, processedDeps);
    } else if (val.IsHolding<VtArray<SdfAssetPath>>()) {
        const VtArray<SdfAssetPath>& originalArray = 
            val.UncheckedGet< VtArray<SdfAssetPath> >();
        
        // ensure explicit empty array value is preserved
        if (originalArray.empty()) {
            return;
        }

        for (const SdfAssetPath& assetPath : originalArray) {                
            const std::string& rawAssetPath = assetPath.GetAssetPath();
            const std::vector<std::string> dependencies = 
                _GetDependencies(layer, rawAssetPath);

            const std::vector<std::string> processedDeps = 
                _delegate->ProcessValuePathArrayElement(
                        layer, keyPath, rawAssetPath, dependencies);
            
            _EnqueueDependency(layer, rawAssetPath);
            _EnqueueDependencies(layer, processedDeps);
        }

        _delegate->EndProcessingValuePathArray(layer, keyPath);
    }
    else if (val.IsHolding<VtDictionary>()) {
        const VtDictionary& originalDict = val.UncheckedGet<VtDictionary>();

        // ensure explicit empty dictionary value is preserved
        if (originalDict.empty()) {
            return;
        }

        for (const auto& p : originalDict) {
            const std::string dictKey = 
                keyPath.empty() ? p.first : keyPath + ':' + p.first;
            _ProcessAssetValue(layer, dictKey, p.second, processingMetadata);

        }
    }
}

// XXX: If we are going to add support for automatically processing additional
// dependencies, they should be added here.
std::vector<std::string> 
UsdUtils_LocalizationContext::_GetDependencies(
    const SdfLayerRefPtr& layer,
    const std::string &assetPath)
{
    return _GetUdimTiles(layer, assetPath);
}

std::vector<std::string> 
UsdUtils_LocalizationContext::_GetUdimTiles(
    const SdfLayerRefPtr& layer, 
    const std::string &assetPath)
{
    std::vector<std::string> additionalPaths;

    if (!UsdShadeUdimUtils::IsUdimIdentifier(assetPath)) {
        return additionalPaths;
    }

    const std::string resolvedUdimPath = 
        UsdShadeUdimUtils::ResolveUdimPath(assetPath, layer);

    if (resolvedUdimPath.empty()) {
        return additionalPaths;
    }

    const std::vector<UsdShadeUdimUtils::ResolvedPathAndTile> resolvedPaths =
    UsdShadeUdimUtils::ResolveUdimTilePaths(resolvedUdimPath, SdfLayerHandle());

    for (const auto & udim : resolvedPaths) {
        additionalPaths.emplace_back(
            UsdShadeUdimUtils::ReplaceUdimPattern(assetPath, udim.second));
    }

    return additionalPaths;
}

bool 
UsdUtils_LocalizationContext::_ShouldFilterAssetPath(
    const std::string &key,
    bool processingMetadata) 
{
    if (!processingMetadata || !_metadataFilteringEnabled) {
        return false;
    }

    // We explicitly filter this key when the feature is enabled
    return key == "assetInfo:identifier";
}

bool 
UsdUtils_LocalizationContext::_ValueTypeIsRelevant(
    const VtValue &val)
{
    return  val.IsHolding<SdfAssetPath>() ||
            val.IsHolding<VtArray<SdfAssetPath>>() ||
            val.IsHolding<VtDictionary>();
}

struct UsdUtils_ExtractExternalReferencesClient {
    UsdUtilsDependencyInfo 
    Process (
        const SdfLayerRefPtr &, 
        const UsdUtilsDependencyInfo &depInfo,
        UsdUtils_DependencyType dependencyType
    )
    {
        if (depInfo.GetDependencies().empty()) {
            PlaceAsset(depInfo.GetAssetPath(), dependencyType);
        }
        else {
            for (const auto& dependency : depInfo.GetDependencies()) {
                PlaceAsset(dependency, dependencyType);
            }
        }

        return {};
    }

    void PlaceAsset(
        const std::string &dependency, 
        UsdUtils_DependencyType dependencyType)
    {
        switch(dependencyType) {
            case UsdUtils_DependencyType::Sublayer:
                sublayers.emplace_back(dependency);
                break;
            case UsdUtils_DependencyType::Reference:
            case UsdUtils_DependencyType::ClipTemplateAssetPath:
                references.emplace_back(dependency);
                break;
            case UsdUtils_DependencyType::Payload:
                payloads.emplace_back(dependency);
                break;
        }
    }

    void SortAndRemoveDuplicates() {
        std::sort(sublayers.begin(), sublayers.end());
        sublayers.erase(std::unique(sublayers.begin(), sublayers.end()),
            sublayers.end());
        
        std::sort(references.begin(), references.end());
        references.erase(std::unique(references.begin(), references.end()),
            references.end());

        std::sort(payloads.begin(), payloads.end());
        payloads.erase(std::unique(payloads.begin(), payloads.end()),
            payloads.end());
    }

    std::vector<std::string> sublayers, references, payloads;
};

void UsdUtils_ExtractExternalReferences(
    const std::string& filePath,
    const UsdUtils_LocalizationContext::ReferenceType refTypesToInclude,
    std::vector<std::string>* outSublayers,
    std::vector<std::string>* outReferences,
    std::vector<std::string>* outPayloads)
{
    TRACE_FUNCTION();

    UsdUtils_ExtractExternalReferencesClient client;
    UsdUtils_ReadOnlyLocalizationDelegate delegate(
        std::bind(&UsdUtils_ExtractExternalReferencesClient::Process, &client,
        std::placeholders::_1, std::placeholders::_2, 
        std::placeholders::_3));
    UsdUtils_LocalizationContext context(&delegate);
    context.SetRefTypesToInclude(refTypesToInclude);
    context.SetRecurseLayerDependencies(false);

    context.Process(SdfLayer::FindOrOpen(filePath));
    client.SortAndRemoveDuplicates();

    if (outSublayers) {
        *outSublayers = std::move(client.sublayers);
    }
    if (outReferences) {
        *outReferences = std::move(client.references);
    }
    if (outPayloads) {
        *outPayloads = std::move(client.payloads);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
