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

        // Process the next layer in the queue.
        // If the layer is a package then we halt traversal because the entire
        // package should be included as dependency if any file contained inside 
        // it is encountered.
        SdfLayerRefPtr layer = SdfLayer::FindOrOpen(anchoredPath);
        if (layer && !layer->GetFileFormat()->IsPackage() ) {
            _ProcessLayer(layer);
        }
    }

    return true;
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
    _delegate->ProcessSublayers(layer, &sublayers);

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
            _delegate->EndProcessValue(layer, primSpec->GetPath(), infoKey, value);
        }
    }

    // Process clips["templateAssetPath"], which is a string value 
    // containing one or more #'s. See 
    // UsdClipsAPI::GetClipTemplateAssetPath for details. 
    VtValue clipsValue = primSpec->GetInfo(UsdTokens->clips);
    if (!clipsValue.IsEmpty() && clipsValue.IsHolding<VtDictionary>()) {
        const VtDictionary origClipsDict = 
                clipsValue.UncheckedGet<VtDictionary>();

        for (auto &clipSetNameAndDict : origClipsDict) {
            if (clipSetNameAndDict.second.IsHolding<VtDictionary>()) {
                VtDictionary clipDict = 
                    clipSetNameAndDict.second.UncheckedGet<VtDictionary>();

                if (VtDictionaryIsHolding<std::string>(clipDict, 
                        UsdClipsAPIInfoKeys->templateAssetPath.GetString())) {
                    const std::string &templateAssetPath = 
                            VtDictionaryGet<std::string>(clipDict, 
                                UsdClipsAPIInfoKeys->templateAssetPath
                                    .GetString());

                    if (templateAssetPath.empty()) {
                        continue;
                    }

                    const std::vector<std::string> clipFiles = 
                        _GetTemplatedClips(layer, templateAssetPath);
                    if (clipFiles.size() > 0) {
                        _delegate->ProcessClipTemplateAssetPath(
                            layer, primSpec, clipSetNameAndDict.first, 
                                templateAssetPath, clipFiles);
                    }
                }
            }
        }
    }
}



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

    // XXX: This also acts as a guard against non-filesystem based resolvers
    // In the future it may be worth investigating if _DeriveClipInfo from
    // clipSetDefinition may be leveraged here for a more robust approach.
    if (!TfIsDir(clipsDirAssetPath)) {
        TF_WARN("Clips directory '%s' is not a valid directory "
            "on the filesystem.", clipsDirAssetPath.c_str());
        return std::vector<std::string>();
    }

    std::string clipsBaseName = TfGetBaseName(templateAssetPath);
    std::string globPattern = TfStringCatPaths(
            clipsDirAssetPath, 
            TfStringReplace(clipsBaseName, "#", "*"));
    std::vector<std::string> clipAssetRefs = TfGlob(globPattern);

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

    _delegate->ProcessPayloads(layer, primSpec, &payloads);
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

    _delegate->ProcessReferences(layer, primSpec, &references);
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

        if (!rawAssetPath.empty()) {
            const std::vector<std::string> dependencies = 
                _GetDependencies(layer, rawAssetPath);

            _delegate->ProcessValuePath(
                    layer, keyPath, rawAssetPath, dependencies);
            
            _EnqueueDependency(layer, rawAssetPath);
        }
    } else if (val.IsHolding<VtArray<SdfAssetPath>>()) {
        const VtArray<SdfAssetPath>& originalArray = 
            val.UncheckedGet< VtArray<SdfAssetPath> >();
        
        // ensure explicit empty array value is preserved
        if (originalArray.empty()) {
            return;
        }

        for (const SdfAssetPath& assetPath : originalArray) {                
            const std::string& rawAssetPath = assetPath.GetAssetPath();
            if (!rawAssetPath.empty()) {
                const std::vector<std::string> dependencies = 
                    _GetDependencies(layer, rawAssetPath);

                _delegate->ProcessValuePathArrayElement(
                        layer, keyPath, rawAssetPath, dependencies);
                
                _EnqueueDependency(layer, rawAssetPath);
            }
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

std::vector<std::string> 
UsdUtils_LocalizationContext::_GetDependencies(
    const SdfLayerRefPtr& layer,
    const std::string &assetPath)
{
    std::vector<std::string> dependencies = _GetUdimTiles(layer, assetPath);

    if (dependencies.empty()) {
        dependencies.emplace_back(assetPath);
    }

    return dependencies;
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

void UsdUtils_ExtractExternalReferences(
    const std::string& filePath,
    const UsdUtils_LocalizationContext::ReferenceType refTypesToInclude,
    std::vector<std::string>* outSublayers,
    std::vector<std::string>* outReferences,
    std::vector<std::string>* outPayloads)
{
    TRACE_FUNCTION();

    std::vector<std::string> sublayers, references, payloads;

    const auto processFunc = [&sublayers, &references, &payloads]( 
        const SdfLayerRefPtr &, 
        const std::string &,
        const std::vector<std::string> &dependencies,
        UsdUtilsDependencyType dependencyType)
    {
        switch(dependencyType) {
            case UsdUtilsDependencyType::SubLayer:
                for (const auto& dependency : dependencies) {
                    sublayers.emplace_back(dependency);
                }
                break;
            case UsdUtilsDependencyType::Reference:
                for (const auto& dependency : dependencies) {
                    references.emplace_back(dependency);
                }
                break;
            case UsdUtilsDependencyType::Payload:
                for (const auto& dependency : dependencies) {
                    payloads.emplace_back(dependency);
                }
                break;
        }
    };

    UsdUtils_ReadOnlyLocalizationDelegate delegate(processFunc);
    UsdUtils_LocalizationContext context(&delegate);
    context.SetRefTypesToInclude(refTypesToInclude);
    context.SetRecurseLayerDependencies(false);

    context.Process(SdfLayer::FindOrOpen(filePath));

    // Sort and remove duplicates
    std::sort(sublayers.begin(), sublayers.end());
    sublayers.erase(std::unique(sublayers.begin(), sublayers.end()),
        sublayers.end());
    
    std::sort(references.begin(), references.end());
    references.erase(std::unique(references.begin(), references.end()),
        references.end());

    std::sort(payloads.begin(), payloads.end());
    payloads.erase(std::unique(payloads.begin(), payloads.end()),
        payloads.end());

    if (outSublayers) {
        *outSublayers = std::move(sublayers);
    }
    if (outReferences) {
        *outReferences = std::move(references);
    }
    if (outPayloads) {
        *outPayloads = std::move(payloads);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
