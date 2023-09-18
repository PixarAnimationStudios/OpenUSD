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

#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/usd/clipsAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

void 
UsdUtils_WritableLocalizationDelegate::ProcessSublayers(
    const SdfLayerRefPtr &layer,
    SdfSubLayerProxy *sublayers)
{
    sublayers->ModifyItemEdits(
        [this, &layer](const std::string& path) {
            std::vector<std::string> dependencies = {path};

            const std::string processedPath = _processingFunc( 
                layer, path, dependencies, UsdUtilsDependencyType::Payload);

            return processedPath.empty() ?
                boost::optional<std::string>() :
                boost::optional<std::string>(std::move(processedPath));
    });
}

void 
UsdUtils_WritableLocalizationDelegate::ProcessPayloads(
    const SdfLayerRefPtr &layer,
    const SdfPrimSpecHandle &,
    SdfPayloadsProxy *payloads)
{
    payloads->ModifyItemEdits(
        [this, &layer](const SdfPayload& payload){
            return _ProcessRefOrPayload<SdfPayload, 
                UsdUtilsDependencyType::Payload>(layer, payload);
        });
}

void 
UsdUtils_WritableLocalizationDelegate::ProcessReferences(
    const SdfLayerRefPtr &layer,
    const SdfPrimSpecHandle &,
    SdfReferencesProxy *references)
{
    references->ModifyItemEdits(
        [this, &layer](const SdfReference& ref){
            return _ProcessRefOrPayload<SdfReference, 
                UsdUtilsDependencyType::Reference>(layer, ref);
        }
    );
}

template <class RefOrPayloadType, UsdUtilsDependencyType DEP_TYPE>
boost::optional<RefOrPayloadType>
UsdUtils_WritableLocalizationDelegate::_ProcessRefOrPayload(
    const SdfLayerRefPtr &layer,
    const RefOrPayloadType& refOrPayload)
{
    // If the asset path is empty this is a local payload. We can ignore
    // these since they refer to the same layer where the payload was
    // authored.
    if (refOrPayload.GetAssetPath().empty()) {
        return boost::optional<RefOrPayloadType>(refOrPayload);
    }

    std::vector<std::string> dependencies = {refOrPayload.GetAssetPath()};
    const std::string processedPath = _processingFunc( 
        layer, refOrPayload.GetAssetPath(), dependencies, DEP_TYPE);

        if (processedPath.empty()) {
            return boost::none;
        }

        RefOrPayloadType processedRefOrPayload = refOrPayload;
        processedRefOrPayload.SetAssetPath(processedPath);

        return boost::optional<RefOrPayloadType>(processedRefOrPayload);
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

void 
UsdUtils_WritableLocalizationDelegate::ProcessValuePath(
    const SdfLayerRefPtr &layer,
    const std::string &keyPath,
    const std::string &authoredPath,
    const std::vector<std::string> &dependencies)
{
    const std::string processedPath = _processingFunc(
        layer, authoredPath, dependencies, UsdUtilsDependencyType::Reference);

    const std::string relativeKeyPath = _GetRelativeKeyPath(keyPath);

    if (relativeKeyPath.empty()) {
        _currentValuePath = SdfAssetPath(processedPath);
    }
    else if (processedPath.empty()){
        _currentValueDictionary.EraseValueAtPath(relativeKeyPath);
    } else {
        _currentValueDictionary.SetValueAtPath(
            relativeKeyPath, VtValue(SdfAssetPath(processedPath)));
    }
}

void 
UsdUtils_WritableLocalizationDelegate::ProcessValuePathArrayElement(
    const SdfLayerRefPtr &layer,
    const std::string &keyPath,
    const std::string &authoredPath,
    const std::vector<std::string> &dependencies)
{
    const std::string processedPath = _processingFunc(
        layer, authoredPath, dependencies, UsdUtilsDependencyType::Reference);
    
    if (!processedPath.empty()) {
        _currentPathArray.emplace_back(processedPath);
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
    if (updatedValue.IsEmpty()) {
        layer->EraseField(path, key);
    }
    else if (val != updatedValue) {
        layer->SetField(path, key, updatedValue);
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
    if (updatedValue.IsEmpty()) {
        layer->EraseTimeSample(path, t);
    } else if ( updatedValue != val) {
        layer->SetTimeSample(path, t, updatedValue);
    }
}

void 
UsdUtils_WritableLocalizationDelegate::ProcessClipTemplateAssetPath(
    const SdfLayerRefPtr &layer,
    const SdfPrimSpecHandle &primSpec,
    const std::string &clipSetName,
    const std::string &templateAssetPath,
    std::vector<std::string> dependencies)
{
    const std::string processedTemplatePath = _processingFunc(
        layer, templateAssetPath, dependencies, UsdUtilsDependencyType::Reference);

    if (processedTemplatePath == templateAssetPath) {
        return;
    }

    VtValue clipsValue = primSpec->GetInfo(UsdTokens->clips);
    const VtDictionary origClipsDict = clipsValue.UncheckedGet<VtDictionary>();

    VtDictionary clipsDict = origClipsDict;
    VtDictionary clipSetDict = clipsDict[clipSetName].UncheckedGet<VtDictionary>();

    clipSetDict[UsdClipsAPIInfoKeys->templateAssetPath] = VtValue(processedTemplatePath);

    primSpec->SetInfo(UsdTokens->clips, VtValue(clipsDict));
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

void 
UsdUtils_ReadOnlyLocalizationDelegate::ProcessSublayers(
    const SdfLayerRefPtr &layer,
    SdfSubLayerProxy *sublayers)
{
    for (const auto &path : *sublayers) {
        std::vector<std::string> dependencies = {path};
        _processingFunc(layer, path, dependencies, 
            UsdUtilsDependencyType::SubLayer);
    }
}

void UsdUtils_ReadOnlyLocalizationDelegate::ProcessPayloads(
    const SdfLayerRefPtr &layer,
    const SdfPrimSpecHandle &,
    SdfPayloadsProxy *payloads)
{
    for (auto const& payload: payloads->GetAppliedItems()){
        // If the asset path is empty this is a local payload. We can ignore
        // these since they refer to the same layer where the payload was
        // authored.
        if (payload.GetAssetPath().empty()) {
            continue;
        }

        std::vector<std::string> dependencies = {payload.GetAssetPath()};
        _processingFunc(layer, payload.GetAssetPath(), dependencies, 
            UsdUtilsDependencyType::Payload);
    }
}

void 
UsdUtils_ReadOnlyLocalizationDelegate::ProcessReferences(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &,
        SdfReferencesProxy *references)
{
    for (const auto& reference: references->GetAppliedItems()){
        _ProcessRefOrPayload<SdfReference, UsdUtilsDependencyType::Reference>(
            layer, reference);
    }
}

template <class RefOrPayloadType, UsdUtilsDependencyType DEP_TYPE>
void 
UsdUtils_ReadOnlyLocalizationDelegate::_ProcessRefOrPayload(
    const SdfLayerRefPtr &layer,
    const RefOrPayloadType& refOrPayload)
{
    // If the asset path is empty this is a local ref/payload. We can ignore
    // these since they refer to the same layer where the payload was
    // authored.
    if (refOrPayload.GetAssetPath().empty()) {
        return;
    }

    std::vector<std::string> dependencies = {refOrPayload.GetAssetPath()};
    _processingFunc(layer, refOrPayload.GetAssetPath(), dependencies, DEP_TYPE);
}

void
UsdUtils_ReadOnlyLocalizationDelegate::ProcessValuePath(
    const SdfLayerRefPtr &layer,
    const std::string &keyPath,
    const std::string &authoredPath,
    const std::vector<std::string> &dependencies)
{
    _processingFunc(layer, authoredPath, dependencies, 
        UsdUtilsDependencyType::Reference);
}

void
UsdUtils_ReadOnlyLocalizationDelegate::ProcessValuePathArrayElement(
    const SdfLayerRefPtr &layer,
    const std::string &keyPath,
    const std::string &authoredPath,
    const std::vector<std::string> &dependencies)
{
    _processingFunc(layer, authoredPath, dependencies, 
        UsdUtilsDependencyType::Reference);
}

void
UsdUtils_ReadOnlyLocalizationDelegate::ProcessClipTemplateAssetPath(
    const SdfLayerRefPtr &layer,
    const SdfPrimSpecHandle &primSpec,
    const std::string &clipSetName,
    const std::string &templateAssetPath,
    std::vector<std::string> dependencies)
{
    _processingFunc(layer, templateAssetPath, dependencies, 
        UsdUtilsDependencyType::Reference);
}

PXR_NAMESPACE_CLOSE_SCOPE
