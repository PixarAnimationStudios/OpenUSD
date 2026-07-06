//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file usdUtils/assetLocalizationDelegate.cpp

#include "pxr/usd/usdUtils/assetLocalizationDelegate.h"

#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/variableExpression.h"
#include "pxr/usd/usd/clipsAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

// Evaluates any variable expressions in a path. This function should be called
// before paths are passed to any processing functions.  If this function
// returns an empty string, it signals to the delegate that the path should be
// skipped for further processing.
std::string 
UsdUtils_EvaluateVariableExpressionInPath(
    const VtDictionary &expressionVariables,
    const std::string &path)
{
    if (!SdfVariableExpression::IsExpression(path)) {
        return path;
    }

    SdfVariableExpression expression(path);

    const std::vector<std::string>& parseErrors = 
        expression.GetErrors();
    if (!parseErrors.empty()) {
        TF_WARN("Failed to parse variable expression '%s': %s",
            path.c_str(), TfStringJoin(parseErrors, ", ").c_str());

        return {};
    }

    SdfVariableExpression::Result result = 
        expression.EvaluateTyped<std::string>(expressionVariables);

    if (!result.errors.empty()) {
        TF_WARN("Failed to evaluate variable expression '%s': %s",
            path.c_str(), TfStringJoin(result.errors, ", ").c_str());

        return {};
    }

    // The expression evaluated to None, this is not an error, and we should
    // return an empty string so the path is skipped.
    if (result.value.IsEmpty()) {
        return {};
    }

    return result.value.UncheckedGet<std::string>();
}

UsdUtilsDependencyInfo 
UsdUtils_LocalizationClient::_CreateUsdUtilsDependencyInfo(
        const std::string &assetPath,
        const std::vector<std::string> &dependencies,
        const std::string &rawAssetPath,
        const VtDictionary *expressionVariables)
{
    return UsdUtilsDependencyInfo(
        assetPath, dependencies, rawAssetPath, expressionVariables);
}

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
UsdUtils_CachedPathLocalizationClient::GetProcessedInfo(
    const SdfLayerRefPtr &layer, 
    const UsdUtilsDependencyInfo &dependencyInfo,
    UsdUtils_DependencyType dependencyType)
{
    auto depKey = 
        std::make_tuple(layer->GetRealPath(), dependencyInfo.GetAssetPath());
    auto result = _cachedPaths.find(depKey);
    if (result == _cachedPaths.end()) {
        UsdUtilsDependencyInfo depInfo = _ProcessDependency(
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
UsdUtils_WritableLocalizationClient::ProcessSublayers(
    const SdfLayerRefPtr &layer,
    const VtDictionary &expressionVariables)
{
    SdfSubLayerProxy sublayerPaths = layer->GetSubLayerPaths();
    std::vector<std::string> processedPaths, dependencies;

    for (const std::string& sublayerPath : sublayerPaths) {
        const std::string processedPath = 
        UsdUtils_EvaluateVariableExpressionInPath(
            expressionVariables, sublayerPath);
        if (processedPath.empty()) {
            continue;
        }

        UsdUtilsDependencyInfo depInfo = _CreateUsdUtilsDependencyInfo(
            processedPath, {}, sublayerPath, &expressionVariables);
        UsdUtilsDependencyInfo info = GetProcessedInfo( 
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
UsdUtils_WritableLocalizationClient::ProcessPayloads(
    const SdfLayerRefPtr &layer,
    const VtDictionary &expressionVariables,
    const SdfPrimSpecHandle &primSpec)
{
    return _ProcessReferencesOrPayloads
        <SdfPayloadListOp, UsdUtils_DependencyType::Payload>(layer,
        expressionVariables, primSpec, SdfFieldKeys->Payload);
}

std::vector<std::string> 
UsdUtils_WritableLocalizationClient::ProcessReferences(
    const SdfLayerRefPtr &layer,
    const VtDictionary &expressionVariables,
    const SdfPrimSpecHandle &primSpec)
{
    return _ProcessReferencesOrPayloads
        <SdfReferenceListOp, UsdUtils_DependencyType::Reference>(layer,
        expressionVariables, primSpec, SdfFieldKeys->References);
}


// Processes references or payloads for a prim.
// Will only attempt to get a writable layer if asset path processing modifies
// existing list ops.
template <class ListOpType, UsdUtils_DependencyType DEP_TYPE>
std::vector<std::string> 
UsdUtils_WritableLocalizationClient::_ProcessReferencesOrPayloads(
    const SdfLayerRefPtr &layer,
    const VtDictionary &expressionVariables,
    const SdfPrimSpecHandle &primSpec,
    const TfToken &listOpToken)
{
    std::vector<std::string> dependencies;
    ListOpType processedListOps;
    if (!primSpec->HasField(listOpToken, &processedListOps)) {
        return dependencies;
    }

    const bool modified = processedListOps.ModifyOperations(
        [this, &layer, &expressionVariables, &dependencies](
            const typename ListOpType::ItemType& item){
            return 
                _ProcessRefOrPayload <typename ListOpType::ItemType, DEP_TYPE>(
                    layer, expressionVariables, item, &dependencies);
        });

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
UsdUtils_WritableLocalizationClient::_ProcessRefOrPayload(
    const SdfLayerRefPtr &layer,
    const VtDictionary& expressionVariables,
    const RefOrPayloadType& refOrPayload,
    std::vector<std::string>* dependencies)
{
    // If the asset path is empty this is a local payload. We can ignore
    // these since they refer to the same layer where the payload was
    // authored.
    if (refOrPayload.GetAssetPath().empty()) {
        return std::optional<RefOrPayloadType>(refOrPayload);
    }

    const std::string processedPath = UsdUtils_EvaluateVariableExpressionInPath(
            expressionVariables, refOrPayload.GetAssetPath());
    
    // if we have a variable expression that evaluates to nothing, then we do
    // not want to do anything special here.
    if (processedPath.empty()) {
        return std::optional<RefOrPayloadType>(refOrPayload);
    }

    UsdUtilsDependencyInfo depInfo = _CreateUsdUtilsDependencyInfo(processedPath, 
        {}, refOrPayload.GetAssetPath(), &expressionVariables);
    const UsdUtilsDependencyInfo info = GetProcessedInfo( 
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
UsdUtils_WritableLocalizationClient::BeginProcessValue(
    const SdfLayerRefPtr &layer,
    const VtValue &val)
{
    if (val.IsHolding<VtDictionary>()) {
        _currentValueDictionary = val.UncheckedGet<VtDictionary>();
    }
}

std::vector<std::string> 
UsdUtils_WritableLocalizationClient::ProcessValuePath(
    const SdfLayerRefPtr &layer,
    const VtDictionary &expressionVariables,
    const std::string &keyPath,
    const std::string &authoredPath,
    const std::vector<std::string> &dependencies,
    const bool processingMetadata,
    const bool processingDictionary)
{
    if (authoredPath.empty()) {
        return {};
    }

    const std::string processedPath = UsdUtils_EvaluateVariableExpressionInPath(
        expressionVariables, authoredPath);

    if (processedPath.empty()) {
        return {};
    }

    UsdUtilsDependencyInfo depInfo = _CreateUsdUtilsDependencyInfo(processedPath,
        dependencies, authoredPath, &expressionVariables);
    UsdUtilsDependencyInfo info = GetProcessedInfo(
        layer, depInfo, UsdUtils_DependencyType::Reference);

    const std::string relativeKeyPath = _GetRelativeKeyPath(keyPath);

    if (relativeKeyPath.empty() || (
            processingMetadata &&
            !processingDictionary &&
            !info.GetAssetPath().empty())) {
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
UsdUtils_WritableLocalizationClient::ProcessValuePathArrayElement(
    const SdfLayerRefPtr &layer,
    const VtDictionary &expressionVariables,
    const std::string &keyPath,
    const std::string &authoredPath,
    const std::vector<std::string> &dependencies)
{
    const std::string processedPath = UsdUtils_EvaluateVariableExpressionInPath(
        expressionVariables, authoredPath);

    if (processedPath.empty()) {
        return {};
    }

    UsdUtilsDependencyInfo depInfo = _CreateUsdUtilsDependencyInfo(processedPath, 
        dependencies, authoredPath, &expressionVariables);
    const UsdUtilsDependencyInfo info = GetProcessedInfo(
        layer, depInfo, UsdUtils_DependencyType::Reference);
    
    if (!info.GetAssetPath().empty()) {
        _currentPathArray.emplace_back(info.GetAssetPath());
        return _AllDependenciesForInfo(info);
    }
    else {
        // We don't want to remove empty paths from arrays. They may be
        // meaningful, for example in primvar attributes that need to be
        // a certain length.
        if (_keepEmptyPathsInArrays) {
            _currentPathArray.emplace_back(SdfAssetPath());
        }

        return {};
    }
}

void 
UsdUtils_WritableLocalizationClient::EndProcessingValuePathArray(
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
UsdUtils_WritableLocalizationClient::_GetUpdatedValue(
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
UsdUtils_WritableLocalizationClient::EndProcessValue(
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
UsdUtils_WritableLocalizationClient::EndProcessTimeSampleValue(
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
UsdUtils_WritableLocalizationClient::ProcessClipTemplateAssetPath(
    const SdfLayerRefPtr &layer,
    const SdfPrimSpecHandle &primSpec,
    const std::string &clipSetName,
    const std::string &templateAssetPath,
    std::vector<std::string> dependencies)
{
    UsdUtilsDependencyInfo depInfo = {templateAssetPath, dependencies};
    const UsdUtilsDependencyInfo info = GetProcessedInfo(layer,
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
UsdUtils_WritableLocalizationClient::_GetRelativeKeyPath(
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
UsdUtils_WritableLocalizationClient::_GetOrCreateWritableLayer(
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

    // Even when editing layers in place we still want to insert them into the
    // map. This ensures that they will remain alive for the duration of the
    // localization process. The edits applied during processing will be
    // preserved until localization is complete.
    auto result = 
        _writableLayerMap.insert(std::make_pair(layer, layer));

    // a writable layer already exists
    if (!result.second || _editLayersInPlace) {
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
UsdUtils_WritableLocalizationClient::GetLayerUsedForWriting(
    const SdfLayerRefPtr& layer) 
{
    if (_editLayersInPlace || !layer) {
        return layer;
    }

    auto result = _writableLayerMap.find(layer);

    if (result != _writableLayerMap.end()) {
        return result->second;
    }

    return layer;
}

void
UsdUtils_WritableLocalizationClient::ClearLayerUsedForWriting(
    const SdfLayerRefPtr& layer)
{
    _writableLayerMap.erase(layer);
}

std::vector<std::string> 
UsdUtils_ReadOnlyLocalizationClient::ProcessSublayers(
    const SdfLayerRefPtr &layer,
    const VtDictionary &expressionVariables)
{
    SdfSubLayerProxy sublayerPaths = layer->GetSubLayerPaths();
    std::vector<std::string> dependencies;

    for (const auto &path : sublayerPaths) {
        const std::string processedPath = 
            UsdUtils_EvaluateVariableExpressionInPath(
                expressionVariables, path);
        if (processedPath.empty()) {
            continue;
        }

        UsdUtilsDependencyInfo depInfo = _CreateUsdUtilsDependencyInfo(
            processedPath, {}, path, &expressionVariables);
        UsdUtilsDependencyInfo info = GetProcessedInfo(
            layer, depInfo, UsdUtils_DependencyType::Sublayer);

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
UsdUtils_ReadOnlyLocalizationClient::ProcessPayloads(
    const SdfLayerRefPtr &layer,
    const VtDictionary& expressionVariables,
    const SdfPrimSpecHandle &primSpec)
{
    return ProcessReferencesOrPayloads
        <SdfPayload, UsdUtils_DependencyType::Payload>(layer,
            expressionVariables, primSpec->GetPayloadList().GetAppliedItems());
}

std::vector<std::string> 
UsdUtils_ReadOnlyLocalizationClient::ProcessReferences(
        const SdfLayerRefPtr &layer,
        const VtDictionary& expressionVariables,
        const SdfPrimSpecHandle &primSpec)
{
    return ProcessReferencesOrPayloads
        <SdfReference, UsdUtils_DependencyType::Reference>(layer,
            expressionVariables, primSpec->GetReferenceList().GetAppliedItems());
}


template <typename RefOrPayloadType, UsdUtils_DependencyType dependencyType>
std::vector<std::string> 
UsdUtils_ReadOnlyLocalizationClient::ProcessReferencesOrPayloads(
    const SdfLayerRefPtr &layer,
    const VtDictionary &expressionVariables,
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

        const std::string& processedPath = 
            UsdUtils_EvaluateVariableExpressionInPath(
                expressionVariables, refOrPayload.GetAssetPath());
        
        // if an expression variable evaluated to nothing, then we do not need
        // to take any action here
        if (processedPath.empty()) {
            continue;
        }

        UsdUtilsDependencyInfo depInfo = _CreateUsdUtilsDependencyInfo(
            processedPath, {}, refOrPayload.GetAssetPath(),
            &expressionVariables);
        UsdUtilsDependencyInfo info = GetProcessedInfo(layer, depInfo,
             dependencyType);

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
UsdUtils_ReadOnlyLocalizationClient::ProcessValuePath(
    const SdfLayerRefPtr &layer,
    const VtDictionary &expressionVariables,
    const std::string &keyPath,
    const std::string &authoredPath,
    const std::vector<std::string> &dependencies,
    const bool processingMetadata,
    const bool processingDictionary)
{
    if (authoredPath.empty()) {
        return {};
    }

    const std::string processedPath = UsdUtils_EvaluateVariableExpressionInPath(
        expressionVariables, authoredPath);

    if (processedPath.empty()) {
        return {};
    }

    UsdUtilsDependencyInfo depInfo = _CreateUsdUtilsDependencyInfo(processedPath, 
        dependencies, authoredPath, &expressionVariables);
    return _AllDependenciesForInfo(GetProcessedInfo(layer, depInfo,
        UsdUtils_DependencyType::Reference));
}

std::vector<std::string>
UsdUtils_ReadOnlyLocalizationClient::ProcessValuePathArrayElement(
    const SdfLayerRefPtr &layer,
    const VtDictionary &expressionVariables,
    const std::string &keyPath,
    const std::string &authoredPath,
    const std::vector<std::string> &dependencies)
{    
    // We may get passed an empty authored path if an array has some
    // explicitly empty elements.
    if (authoredPath.empty()) {
        return {};
    }

    const std::string processedPath = UsdUtils_EvaluateVariableExpressionInPath(
        expressionVariables, authoredPath);

    if (processedPath.empty()) {
        return {};
    }

    UsdUtilsDependencyInfo depInfo = _CreateUsdUtilsDependencyInfo(processedPath,
        dependencies, authoredPath, &expressionVariables);
    return _AllDependenciesForInfo(GetProcessedInfo(layer, depInfo,
        UsdUtils_DependencyType::Reference));
}

std::vector<std::string>
UsdUtils_ReadOnlyLocalizationClient::ProcessClipTemplateAssetPath(
    const SdfLayerRefPtr &layer,
    const SdfPrimSpecHandle &primSpec,
    const std::string &clipSetName,
    const std::string &templateAssetPath,
    std::vector<std::string> dependencies)
{
    return _AllDependenciesForInfo(GetProcessedInfo(
        layer, {templateAssetPath, dependencies}, 
        UsdUtils_DependencyType::ClipTemplateAssetPath));
}

PXR_NAMESPACE_CLOSE_SCOPE
