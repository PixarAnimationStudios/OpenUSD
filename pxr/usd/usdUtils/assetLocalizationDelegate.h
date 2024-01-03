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
#ifndef PXR_USD_USD_UTILS_ASSET_LOCALIZATION_DELEGATE_H
#define PXR_USD_USD_UTILS_ASSET_LOCALIZATION_DELEGATE_H

/// \file usdUtils/assetLocalizationDelegate.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/usdUtils/dependencies.h"

#include <vector>
#include <string>
#include <map>

PXR_NAMESPACE_OPEN_SCOPE

// This class defines the interface between the UsdUtils_LocalizationContext and
// localization clients.
struct UsdUtils_LocalizationDelegate
{
    virtual void ProcessSublayers(
        const SdfLayerRefPtr &layer,
        SdfSubLayerProxy *sublayers) {}

    virtual void ProcessPayloads(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec,
        SdfPayloadEditorProxy *payloads) {}

    virtual void ProcessReferences(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec,
        SdfReferencesProxy *references) {}

    // Signals the start of a new value.  This will only be triggered if the 
    // value is relevant for localization.  Therefore it will be either a
    // SdfAssetPath, VtArray<SdfAssetPath> or a dictionary.
    virtual void BeginProcessValue(
        const SdfLayerRefPtr &layer,
        const VtValue &val) {}

    virtual void ProcessValuePath(
        const SdfLayerRefPtr &layer,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies) {}

    virtual void ProcessValuePathArrayElement(
        const SdfLayerRefPtr &layer,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies) {}

    // Signals that an asset path array value has been processed.  It is safe
    // to modify the array in this callback.
    virtual void EndProcessingValuePathArray(
        const SdfLayerRefPtr &layer,
        const std::string &keyPath) {}

    virtual void EndProcessTimeSampleValue(
        const SdfLayerRefPtr &layer,
        const SdfPath &path,
        double t,
        const VtValue &val) {}

    virtual void EndProcessValue(
        const SdfLayerRefPtr &layer,
        const SdfPath &path,
        const TfToken &key,
        const VtValue &val) {};

    virtual void ProcessClipTemplateAssetPath(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec,
        const std::string &clipSetName,
        const std::string &templateAssetPath,
        std::vector<std::string> dependencies) {}
};

// A Delegate which allows for modification and optional removal of
// asset path values.  This delegate invokes a user supplied processing function
// on every asset path it encounters.  It will update the path with the returned
// value.  If this value is empty, it will remove the asset path from the layer.
class UsdUtils_WritableLocalizationDelegate
    :public UsdUtils_LocalizationDelegate
{
public:
    UsdUtils_WritableLocalizationDelegate(UsdUtilsProcessingFunc processingFunc)
        :_processingFunc(processingFunc)
    {}

    virtual void ProcessSublayers(
        const SdfLayerRefPtr &layer,
        SdfSubLayerProxy *sublayers) override;

    virtual void ProcessPayloads(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec,
        SdfPayloadsProxy *payloads) override;

    virtual void ProcessReferences(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec,
        SdfReferencesProxy *references) override;

    virtual void BeginProcessValue(
        const SdfLayerRefPtr &layer,
        const VtValue &val) override;

    virtual void ProcessValuePath(
        const SdfLayerRefPtr &layer,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies) override;

    virtual void ProcessValuePathArrayElement(
        const SdfLayerRefPtr &layer,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies) override;

    virtual void EndProcessingValuePathArray(
        const SdfLayerRefPtr &layer,
        const std::string &keyPath) override;

    virtual void EndProcessValue(
        const SdfLayerRefPtr &layer,
        const SdfPath &path,
        const TfToken &key,
        const VtValue &val) override;

    virtual void EndProcessTimeSampleValue(
        const SdfLayerRefPtr &layer,
        const SdfPath &path,
        double t,
        const VtValue &val) override;

    virtual void ProcessClipTemplateAssetPath(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec,
        const std::string &clipSetName,
        const std::string &templateAssetPath,
        std::vector<std::string> dependencies) override;
    
private:
    template <class RefOrPayloadType, UsdUtilsDependencyType DEP_TYPE>
    boost::optional<RefOrPayloadType> _ProcessRefOrPayload(
        const SdfLayerRefPtr &layer,
        const RefOrPayloadType& refOrPayload);

    VtValue _GetUpdatedValue(const VtValue &val);

    static std::string _GetRelativeKeyPath(const std::string& fullPath);
        
    // the user supplied processing function that will be invoked on every path.
    UsdUtilsProcessingFunc _processingFunc;

    SdfAssetPath _currentValuePath;
    VtArray<SdfAssetPath> _currentValuePathArray;

    // Holds the current state of the dictionary value that is being processed.
    // Note that this is a copy of the original value dictionary that was
    // passed in to BeginProcessValue.
    VtDictionary _currentValueDictionary;

    // Current state of the asset[] being processed
    VtArray<SdfAssetPath> _currentPathArray;
};

// This delegate provides clients with ReadOnly access to processed
// asset references. This delegate does not maintain any state.
class UsdUtils_ReadOnlyLocalizationDelegate
: public UsdUtils_LocalizationDelegate
{
public:
    using ProcessingFunc = std::function<void(
            const SdfLayerRefPtr &layer, 
            const std::string assetPath, 
            const std::vector<std::string>& additionalPaths,
            UsdUtilsDependencyType dependencyType)>;

    UsdUtils_ReadOnlyLocalizationDelegate(ProcessingFunc processingFunc)
        : _processingFunc(processingFunc) {}

    virtual void ProcessSublayers(
        const SdfLayerRefPtr &layer,
        SdfSubLayerProxy *sublayers) override;

    virtual void ProcessPayloads(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec,
        SdfPayloadsProxy *payloads);

    virtual void ProcessReferences(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec,
        SdfReferencesProxy *references) override;

    virtual void ProcessValuePath(
        const SdfLayerRefPtr &layer,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies) override;

    virtual void ProcessValuePathArrayElement(
        const SdfLayerRefPtr &layer,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies) override;

    virtual void ProcessClipTemplateAssetPath(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec,
        const std::string &clipSetName,
        const std::string &templateAssetPath,
        std::vector<std::string> dependencies) override;

private:
    template <class RefOrPayloadType, UsdUtilsDependencyType DEP_TYPE>
    void _ProcessRefOrPayload(
        const SdfLayerRefPtr &layer,
        const RefOrPayloadType& refOrPayload);

    ProcessingFunc _processingFunc;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_UTILS_ASSET_LOCALIZATION_DELEGATE_H
