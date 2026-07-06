//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_UTILS_ASSET_LOCALIZATION_DELEGATE_H
#define PXR_USD_USD_UTILS_ASSET_LOCALIZATION_DELEGATE_H

/// \file usdUtils/assetLocalizationDelegate.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/usd/usdUtils/userProcessingFunc.h"

#include "pxr/base/tf/hash.h"

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

// This is the base class for localization clients and defines the interface
// used by UsdUtils_LocalizationContext to facilitate the processing of
// scene description.
// Methods which directly process asset paths return a vector of std::string.
// The return value for these functions indicates additional asset paths
// that should be enqueued for traversal and processing by the localization
// context.
class UsdUtils_LocalizationClient {
public:
    virtual ~UsdUtils_LocalizationClient() = default;

    virtual std::vector<std::string> ProcessSublayers(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables) { return {}; }

    virtual std::vector<std::string> ProcessPayloads(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables,
        const SdfPrimSpecHandle &primSpec) { return {}; }

    virtual std::vector<std::string> ProcessReferences(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables,
        const SdfPrimSpecHandle &primSpec) { return {}; }

    // Signals the start of a new value.  This will only be triggered if the 
    // value is relevant for localization.  Therefore it will be either a
    // SdfAssetPath, VtArray<SdfAssetPath> or a dictionary.
    virtual void BeginProcessValue(
        const SdfLayerRefPtr &layer,
        const VtValue &val) {}

    virtual std::vector<std::string> ProcessValuePath(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies,
        const bool processingMetadata = false,
        const bool processingDictionary = false) { return {}; }

    virtual std::vector<std::string> ProcessValuePathArrayElement(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables,
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

    // Returns true if the supplied path is expected to resolve.
    // An example case where resolution would not be expected is if it
    // is a remapped path (such as in a USDZ archive)
    virtual bool PathShouldResolve(const std::string &path) { return true; }

protected:
    virtual UsdUtilsDependencyInfo _ProcessDependency( 
        const SdfLayerRefPtr &layer, 
        const UsdUtilsDependencyInfo &dependencyInfo,
        UsdUtils_DependencyType dependencyType) = 0;

    // Use this function to create UsdUtilsDependencyInfo using the private
    // constructor containing the raw asset path and expression variables
    static UsdUtilsDependencyInfo _CreateUsdUtilsDependencyInfo(
        const std::string &assetPath,
        const std::vector<std::string> &dependencies,
        const std::string &rawAssetPath,
        const VtDictionary *expressionVariables);
};

// Base class for clients which would like to cache the results of the
// pure virtual _ProcessDependency method.
class UsdUtils_CachedPathLocalizationClient : 
    public UsdUtils_LocalizationClient
{
public:
    UsdUtilsDependencyInfo GetProcessedInfo(
        const SdfLayerRefPtr &layer, 
        const UsdUtilsDependencyInfo &dependencyInfo,
        UsdUtils_DependencyType dependencyType);

private:
    using PathKey = std::tuple<std::string, std::string>;

    struct ProcessedPathHash {
        size_t operator()(const PathKey& key) const
        {
            return TfHash::Combine(std::get<0>(key), std::get<1>(key));
        }
    };

    std::unordered_map<PathKey, std::string, ProcessedPathHash> _cachedPaths;
};

// A Delegate which allows for modification and optional removal of
// asset path values.  This delegate invokes a user supplied processing function
// on every asset path it encounters.  It will update the path with the returned
// value.  If this value is empty, it will remove the asset path from the layer.
class UsdUtils_WritableLocalizationClient
    : public UsdUtils_CachedPathLocalizationClient
{
public:
    virtual std::vector<std::string> ProcessSublayers(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables) override;

    virtual std::vector<std::string> ProcessPayloads(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables,
        const SdfPrimSpecHandle &primSpec) override;

    virtual std::vector<std::string> ProcessReferences(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables,
        const SdfPrimSpecHandle &primSpec) override;

    virtual void BeginProcessValue(
        const SdfLayerRefPtr &layer,
        const VtValue &val) override;

    virtual std::vector<std::string> ProcessValuePath(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies,
        const bool processingMetadata = false,
        const bool processingDictionary = false) override;

    virtual std::vector<std::string> ProcessValuePathArrayElement(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables,
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

    // Controls whether empty asset paths are kept in arrays.
    // If the value is false, paths that are empty after processing are
    // removed from the layer.  Setting this to true will write empty asset
    // paths into the array so that it's length remains unchanged after
    // processing.
    inline void SetKeepEmptyPathsInArrays(bool keep)
    {
        _keepEmptyPathsInArrays = keep;
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
        const VtDictionary& expressionVariables,
        const SdfPrimSpecHandle &primSpec,
        const TfToken &listOpToken);

    template <class RefOrPayloadType, UsdUtils_DependencyType DEP_TYPE>
    std::optional<RefOrPayloadType> _ProcessRefOrPayload(
        const SdfLayerRefPtr &layer,
        const VtDictionary& expressionVariables,
        const RefOrPayloadType& refOrPayload,
        std::vector<std::string>* dependencies);

    VtValue _GetUpdatedValue(const VtValue &val);

    // Creates or retrieves the anonymous layer used for writing changes to the
    // source layer. If _editLayersInPlace is true then the passed in layer will
    // be returned.
    SdfLayerRefPtr _GetOrCreateWritableLayer(const SdfLayerRefPtr& layer);

    static std::string _GetRelativeKeyPath(const std::string& fullPath);

    SdfAssetPath _currentValuePath;
    VtArray<SdfAssetPath> _currentValuePathArray;

    // Holds the current state of the dictionary value that is being processed.
    // Note that this is a copy of the original value dictionary that was
    // passed in to BeginProcessValue.
    VtDictionary _currentValueDictionary;

    // Current state of the asset[] being processed
    VtArray<SdfAssetPath> _currentPathArray;

    bool _editLayersInPlace = false;

    bool _keepEmptyPathsInArrays = false;

    // Maps source layer identifiers to their writable counterparts.
    // If _editLayersInPlace is true, the layers will map to themselves,
    // ensuring they stay alive for the duration of the localization process.
    // Otherwise, source layers will map to an anonymous copy that will be
    // used for all edits that would apply to the source layer
    std::map<SdfLayerRefPtr, SdfLayerRefPtr> _writableLayerMap;
};

// This delegate provides clients with ReadOnly access to processed
// asset references. This delegate does not maintain any state.
class UsdUtils_ReadOnlyLocalizationClient
: public UsdUtils_CachedPathLocalizationClient
{
public:
    virtual std::vector<std::string> ProcessSublayers(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables) override;

    virtual std::vector<std::string> ProcessPayloads(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables,
        const SdfPrimSpecHandle &primSpec) override;

    virtual std::vector<std::string> ProcessReferences(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables,
        const SdfPrimSpecHandle &primSpec) override;

    virtual std::vector<std::string> ProcessValuePath(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables,
        const std::string &keyPath,
        const std::string &authoredPath,
        const std::vector<std::string> &dependencies,
        const bool processingMetadata = false,
        const bool processingDictionary = false) override;

    virtual std::vector<std::string> ProcessValuePathArrayElement(
        const SdfLayerRefPtr &layer,
        const VtDictionary &expressionVariables,
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
        const VtDictionary &expressionVariables,
        const std::vector<RefOrPayloadType>& appliedItems);
};

// Evaluates any variable expressions in a path. If this function returns an
// empty string, it signals that the path should be skipped for further
// processing.
std::string UsdUtils_EvaluateVariableExpressionInPath(
    const VtDictionary &expressionVariables,
    const std::string &path);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_UTILS_ASSET_LOCALIZATION_DELEGATE_H
