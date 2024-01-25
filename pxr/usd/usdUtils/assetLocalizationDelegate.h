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
#include "pxr/usd/usdUtils/userProcessingFunc.h"

#include <vector>
#include <string>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

/// Enum representing the type of dependency.
enum class UsdUtils_DependencyType {
    Reference,
    Sublayer,
    Payload,
    ClipTemplateAssetPath
};

// This class defines the interface between the UsdUtils_LocalizationContext and
// localization clients.
// Methods which directly process asset paths return a vector of std::string.
// The return value for these functions indicates additional asset paths
// that should be enqueued for traversal and processing by the localization
// context.
struct UsdUtils_LocalizationDelegate
{
    virtual std::vector<std::string> ProcessSublayers(
        const SdfLayerRefPtr &layer) { return {}; }

    virtual std::vector<std::string> ProcessPayloads(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec) { return {}; }

    virtual std::vector<std::string> ProcessReferences(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec) { return {}; }

    // Signals the start of a new value.  This will only be triggered if the 
    // value is relevant for localization.  Therefore it will be either a
    // SdfAssetPath, VtArray<SdfAssetPath> or a dictionary.
    virtual void BeginProcessValue(
        const SdfLayerRefPtr &layer,
        const VtValue &val) {}

    virtual std::vector<std::string> ProcessValuePath(
        const SdfLayerRefPtr &layer,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies) { return {}; }

    virtual std::vector<std::string> ProcessValuePathArrayElement(
        const SdfLayerRefPtr &layer,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies) { return {}; }

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

    virtual std::vector<std::string> ProcessClipTemplateAssetPath(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec,
        const std::string &clipSetName,
        const std::string &templateAssetPath,
        std::vector<std::string> dependencies) { return {}; }
};

// A Delegate which allows for modification and optional removal of
// asset path values.  This delegate invokes a user supplied processing function
// on every asset path it encounters.  It will update the path with the returned
// value.  If this value is empty, it will remove the asset path from the layer.
class UsdUtils_WritableLocalizationDelegate
    : public UsdUtils_LocalizationDelegate
{
public:
    using ProcessingFunc = std::function<UsdUtilsDependencyInfo(
        const SdfLayerRefPtr &layer, 
        const UsdUtilsDependencyInfo &dependencyInfo,
        UsdUtils_DependencyType dependencyType)>;

    UsdUtils_WritableLocalizationDelegate(
        ProcessingFunc processingFunc)
        :_processingFunc(processingFunc)
    {}

    virtual std::vector<std::string> ProcessSublayers(
        const SdfLayerRefPtr &layer) override;

    virtual std::vector<std::string> ProcessPayloads(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec) override;

    virtual std::vector<std::string> ProcessReferences(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec) override;

    virtual void BeginProcessValue(
        const SdfLayerRefPtr &layer,
        const VtValue &val) override;

    virtual std::vector<std::string> ProcessValuePath(
        const SdfLayerRefPtr &layer,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies) override;

    virtual std::vector<std::string> ProcessValuePathArrayElement(
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

    virtual std::vector<std::string> ProcessClipTemplateAssetPath(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec,
        const std::string &clipSetName,
        const std::string &templateAssetPath,
        std::vector<std::string> dependencies) override;

    // Controls whether layers are edited in place.  If this is enabled, the
    // source layers will be written to directly.  If diabled, anonymous copies 
    // of layers will be created before writing any changes to asset paths as a
    // result of the user supplied processing function.
    inline void SetEditLayersInPlace(bool editLayersInPlace)
    {
        _editLayersInPlace = editLayersInPlace;
    }

    // Returns the layer that was used for writing the passed in layer.
    // Note that if _editLayersInPlace is true, or there were no edits to
    // the particular layer, the passed in value will be returned.
    SdfLayerConstHandle GetLayerUsedForWriting(const SdfLayerRefPtr& layer);

    // Removes the reference to the layer used for writing changes to the
    // source layer.
    void ClearLayerUsedForWriting(const SdfLayerRefPtr& layer);
    
private:
    template <class ListOpType, UsdUtils_DependencyType DEP_TYPE>
    std::vector<std::string> _ProcessReferencesOrPayloads(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec,
        const TfToken &listOpToken);

    template <class RefOrPayloadType, UsdUtils_DependencyType DEP_TYPE>
    std::optional<RefOrPayloadType> _ProcessRefOrPayload(
        const SdfLayerRefPtr &layer,
        const RefOrPayloadType& refOrPayload,
        std::vector<std::string>* dependencies);

    VtValue _GetUpdatedValue(const VtValue &val);

    // Creates or retrieves the anonymous layer used for writing changes to the
    // source layer. If _editLayersInPlace is true then the passed in layer will
    // be returned.
    SdfLayerRefPtr _GetOrCreateWritableLayer(const SdfLayerRefPtr& layer);

    static std::string _GetRelativeKeyPath(const std::string& fullPath);

    // the user supplied processing function that will be invoked on every path.
    ProcessingFunc _processingFunc;

    SdfAssetPath _currentValuePath;
    VtArray<SdfAssetPath> _currentValuePathArray;

    // Holds the current state of the dictionary value that is being processed.
    // Note that this is a copy of the original value dictionary that was
    // passed in to BeginProcessValue.
    VtDictionary _currentValueDictionary;

    // Current state of the asset[] being processed
    VtArray<SdfAssetPath> _currentPathArray;

    bool _editLayersInPlace = false;

    // Maps source layer identifiers to their anonymous writable copies.
    std::map<SdfLayerRefPtr, SdfLayerRefPtr> _layerCopyMap;
};

// This delegate provides clients with ReadOnly access to processed
// asset references. This delegate does not maintain any state.
class UsdUtils_ReadOnlyLocalizationDelegate
: public UsdUtils_LocalizationDelegate
{
public:
    using ProcessingFunc = std::function<UsdUtilsDependencyInfo(
            const SdfLayerRefPtr &layer, 
            const std::string &assetPath,
            const std::vector<std::string> &dependencies,
            UsdUtils_DependencyType dependencyType)>;

    UsdUtils_ReadOnlyLocalizationDelegate(ProcessingFunc processingFunc)
        : _processingFunc(processingFunc) {}

    virtual std::vector<std::string> ProcessSublayers(
        const SdfLayerRefPtr &layer) override;

    virtual std::vector<std::string> ProcessPayloads(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec);

    virtual std::vector<std::string> ProcessReferences(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec) override;

    virtual std::vector<std::string> ProcessValuePath(
        const SdfLayerRefPtr &layer,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies) override;

    virtual std::vector<std::string> ProcessValuePathArrayElement(
        const SdfLayerRefPtr &layer,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies) override;

    virtual std::vector<std::string> ProcessClipTemplateAssetPath(
        const SdfLayerRefPtr &layer,
        const SdfPrimSpecHandle &primSpec,
        const std::string &clipSetName,
        const std::string &templateAssetPath,
        std::vector<std::string> dependencies) override;

private:
    template <typename RefOrPayloadType, UsdUtils_DependencyType DepType>
    std::vector<std::string> ProcessReferencesOrPayloads(
        const SdfLayerRefPtr &layer,
        const std::vector<RefOrPayloadType>& appliedItems);

    ProcessingFunc _processingFunc;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_UTILS_ASSET_LOCALIZATION_DELEGATE_H
