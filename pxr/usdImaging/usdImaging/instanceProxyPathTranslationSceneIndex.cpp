//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/instanceProxyPathTranslationSceneIndex.h"
#include "pxr/imaging/hd/dataSourceHash.h"
#include "pxr/imaging/hd/instanceSchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/trace/trace.h" 
#include "pxr/base/tf/stackTrace.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImaging_InstanceProxyPathTranslationSceneIndexImpl {

struct Data
{
public:
    Data(TfTokenVector const& proxyPathDataSourceNames)
        : _proxyPathDataSourceNames(proxyPathDataSourceNames) {}
    
    bool
    ShouldTranslatePathsForDataSourceName(TfToken const& name) const
    {
        return std::find(_proxyPathDataSourceNames.begin(),
                         _proxyPathDataSourceNames.end(),
                         name) != _proxyPathDataSourceNames.end();
    }
private:
    // Prim-level data source names under which to apply instance proxy path
    // translation.
    TfTokenVector _proxyPathDataSourceNames;

};

} // namespace UsdImaging_InstanceProxyPathTranslationSceneIndexImpl

// -----------------------------------------------------------------------------

namespace {

// Forward declare helper for recursive path translation in data sources.
HdDataSourceBaseHandle
_TranslateDataSource(
    HdDataSourceBaseHandle const& ds,
    HdSceneIndexBaseConstRefPtr const& si);

std::optional<SdfPath>
_GetPrototypePath(
    HdInstanceSchema const& instanceSchema,
    HdSceneIndexBaseConstRefPtr const& sceneIndex)
{
    const auto instancerPathDs = instanceSchema.GetInstancer();
    if (!instancerPathDs) {
        return {};
    }
    const auto prototypeIdxDs = instanceSchema.GetPrototypeIndex();
    if (!prototypeIdxDs) {
        return {};
    }

    const SdfPath instancerPath = instancerPathDs->GetTypedValue(0.0);
    const int protoIdx = prototypeIdxDs->GetTypedValue(0.0);

    const HdSceneIndexPrim instancerPrim = sceneIndex->GetPrim(instancerPath);
    HdInstancerTopologySchema instancerTopologySchema =
        HdInstancerTopologySchema::GetFromParent(instancerPrim.dataSource);
    
    if (HdPathArrayDataSourceHandle protoPathsDs =
            instancerTopologySchema.GetPrototypes()) {

        const VtArray<SdfPath> protoPaths = protoPathsDs->GetTypedValue(0.0);
        if (protoIdx >= 0 && protoIdx < static_cast<int>(protoPaths.size())) {
            return protoPaths[protoIdx];
        }
    }

    return {};
}

bool
_IsValid(HdSceneIndexPrim const &prim)
{
    return !prim.primType.IsEmpty() || prim.dataSource;
}

SdfPath
_TranslatePath(
    SdfPath const& path,
    HdSceneIndexBaseConstRefPtr const& sceneIndex)
{
    TRACE_FUNCTION();
    // Don't translate a path to a valid scene index prim.
    // We do this for two reasons:
    // 1. Avoid querying the scene index at each path prefix for the general
    //    case where the prim is not a descendant of an instance prim.
    // 2. To not incorreclty translate an instance prim path to its
    //    prototype path. We want to do this only for *descendant* paths of
    //    instance prims that don't have a corresponding prim in the scene
    //    index.
    //
    const HdSceneIndexPrim prim = sceneIndex->GetPrim(path);
    if (_IsValid(prim)) {
        return path;
    }

    // Iterate over the prefixes, adding the terminal path element of each
    // prefix and checking if the prim at the resulting path is an instance.
    // If it is, replace the result with the prototype path of the instance and
    // continue iterating over the prefixes.
    //
    SdfPath result = SdfPath::AbsoluteRootPath();

    for (SdfPath const& p: path.GetPrefixes()) {
        result = result.AppendChild(p.GetNameToken());

        HdInstanceSchema instanceSchema = 
            HdInstanceSchema::GetFromParent(
                sceneIndex->GetPrim(result).dataSource);
        
        if (const std::optional<SdfPath> prototypePath =
                _GetPrototypePath(instanceSchema, sceneIndex)) {
            result = *prototypePath;
        }
    }

    return result;
}

// Data source that recursively wraps data sources to apply path translation,
// given a HdVectorDataSource.
class _VectorDs : public HdVectorDataSource
{
public:
    HD_DECLARE_DATASOURCE(_VectorDs);

    size_t GetNumElements() override {
        return _underlyingDs->GetNumElements();
    }
    HdDataSourceBaseHandle GetElement(size_t i) override {
        return _TranslateDataSource(_underlyingDs->GetElement(i), _sceneIndex);
    }

private:
    _VectorDs(
        HdSceneIndexBaseConstRefPtr const& sceneIndex,
        HdVectorDataSourceHandle const& underlyingDs)
    : _sceneIndex(sceneIndex)
    , _underlyingDs(underlyingDs)
    {
    }

    const HdSceneIndexBaseConstRefPtr _sceneIndex;
    const HdVectorDataSourceHandle _underlyingDs;
};

// Data source that recursively wraps data sources to apply path translation,
// given a HdContainerDataSource.
class _ContainerDs : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ContainerDs);

    TfTokenVector GetNames() override {
        return _underlyingDs->GetNames();
    }
    HdDataSourceBaseHandle Get(TfToken const& name) override {
        return _TranslateDataSource(_underlyingDs->Get(name), _sceneIndex);
    }

protected:
    _ContainerDs(
        HdSceneIndexBaseConstRefPtr const& sceneIndex,
        HdContainerDataSourceHandle const& underlyingDs)
    : _sceneIndex(sceneIndex)
    , _underlyingDs(underlyingDs)
    {
    }

    const HdSceneIndexBaseConstRefPtr _sceneIndex;
    const HdContainerDataSourceHandle _underlyingDs;
};

// Data source that recursively wraps data sources to apply path translation,
// given a prim-level container data source.
class _PrimDs : public HdContainerDataSource
{
public:
    using ImplDataSharedPtr =
        UsdImaging_InstanceProxyPathTranslationSceneIndexImpl::DataSharedPtr;

    HD_DECLARE_DATASOURCE(_PrimDs);

    TfTokenVector GetNames() override {
        return _underlyingDs->GetNames();
    }
    HdDataSourceBaseHandle Get(TfToken const& name) override {
        return _data->ShouldTranslatePathsForDataSourceName(name)
            ? _TranslateDataSource(_underlyingDs->Get(name), _sceneIndex)
            : _underlyingDs->Get(name);
    }

private:
    _PrimDs(
        HdSceneIndexBaseRefPtr const& inputSceneIndex,
        HdContainerDataSourceHandle const& underlyingDs,
        ImplDataSharedPtr const& data)
    : _sceneIndex(inputSceneIndex)
    , _underlyingDs(underlyingDs)
    , _data(data)
    {
    }

    const HdSceneIndexBaseConstRefPtr _sceneIndex;
    const HdContainerDataSourceHandle _underlyingDs;
    const ImplDataSharedPtr _data;
};

// Apply instance path-translation and recursive wrapping to the data source.
HdDataSourceBaseHandle
_TranslateDataSource(
    HdDataSourceBaseHandle const& ds,
    HdSceneIndexBaseConstRefPtr const& si)
{
    // Translate SdfPath-valued data sources.
    if (auto pathDs = HdPathDataSource::Cast(ds)) {
        SdfPath path = pathDs->GetTypedValue(0.0); 
        return HdRetainedTypedSampledDataSource<SdfPath>
            ::New(_TranslatePath(path, si));
    }

    // Translate VtArray<SdfPath>-valued data sources.
    if (auto pathArrayDs = HdPathArrayDataSource::Cast(ds)) {
        VtArray<SdfPath> pathArray = pathArrayDs->GetTypedValue(0.0);
        for (SdfPath& path: pathArray) {
            path = _TranslatePath(path, si);
        }
        return HdRetainedTypedSampledDataSource<
            VtArray<SdfPath>>::New(pathArray);
    }

    // Recursively wrap container data sources.
    if (auto containerDs = HdContainerDataSource::Cast(ds)) {
        return _ContainerDs::New(si, containerDs);
    }

    // Recursively wrap vector data sources.
    if (auto vectorDs = HdVectorDataSource::Cast(ds)) {
        return _VectorDs::New(si, vectorDs);
    }

    return ds;
}

} // anon

// -----------------------------------------------------------------------------

UsdImaging_InstanceProxyPathTranslationSceneIndexRefPtr
UsdImaging_InstanceProxyPathTranslationSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    TfTokenVector const& proxyPathTranslationDataSourceNames)
{
    return TfCreateRefPtr(
        new UsdImaging_InstanceProxyPathTranslationSceneIndex(inputSceneIndex,
            proxyPathTranslationDataSourceNames));
}

UsdImaging_InstanceProxyPathTranslationSceneIndex::
UsdImaging_InstanceProxyPathTranslationSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    TfTokenVector const& proxyPathTranslationDataSourceNames)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    , _data(
        std::make_shared<
            UsdImaging_InstanceProxyPathTranslationSceneIndexImpl::Data>(
                proxyPathTranslationDataSourceNames))
{
}

HdSceneIndexPrim
UsdImaging_InstanceProxyPathTranslationSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.dataSource) {
        prim.dataSource = _PrimDs::New(
            _GetInputSceneIndex(), prim.dataSource, _data);
    }
    return prim;
}

SdfPathVector
UsdImaging_InstanceProxyPathTranslationSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdImaging_InstanceProxyPathTranslationSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
UsdImaging_InstanceProxyPathTranslationSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
UsdImaging_InstanceProxyPathTranslationSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
