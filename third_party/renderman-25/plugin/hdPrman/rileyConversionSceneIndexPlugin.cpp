//
// Copyright 2024 Pixar
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
#include "hdPrman/rileyConversionSceneIndexPlugin.h"

#include "hdPrman/rileyFallbackMaterialSceneIndexPlugin.h"
#include "hdPrman/rileyGeometryInstanceSchema.h"
#include "hdPrman/rileyGeometryPrototypeSchema.h"
#include "hdPrman/rileyParamSchema.h"
#include "hdPrman/rileyPrimvarSchema.h"
#include "hdPrman/sceneIndexObserverApi.h"
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/sphereSchema.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_RileyConversionSceneIndexPlugin"))

    ((geometryInstance, "RileyConversionGeometryInstance"))
    ((riRadius, "Ri:radius"))
);

static const char * const _rendererDisplayName = "Prman";

namespace {

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

TF_DECLARE_REF_PTRS(_RileyConversionSceneIndex);

class _GeometryPrototypeParamsDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_GeometryPrototypeParamsDataSource);

    _GeometryPrototypeParamsDataSource(
        HdContainerDataSourceHandle const &primSource)
      : _primSource(primSource)
    {
    }

    TfTokenVector GetNames() override
    {
        return { _tokens->riRadius };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == _tokens->riRadius) {
            return 
                HdPrmanRileyPrimvarSchema::Builder()
                    .SetValue(
                        HdSphereSchema::GetFromParent(_primSource).GetRadius())
                    .SetDetailType(
                        HdPrmanRileyPrimvarSchema::BuildDetailTypeDataSource(
                            HdPrmanRileyPrimvarSchemaTokens->constant))
                    .Build();
        }
        return nullptr;
    }

    HdContainerDataSourceHandle const _primSource;
};

class _GeometryInstanceDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_GeometryInstanceDataSource);

    _GeometryInstanceDataSource(
        const SdfPath &protoPrimPath,
        HdContainerDataSourceHandle const &primSource)
      : _protoPrimPath(protoPrimPath)
      , _primSource(primSource)
    {
    }

    TfTokenVector GetNames() override
    {
        return {
            HdPrmanRileyGeometryInstanceSchemaTokens->geoPrototype,
            HdPrmanRileyGeometryInstanceSchemaTokens->material,
            HdPrmanRileyGeometryInstanceSchemaTokens->xform,
            HdPrmanRileyGeometryInstanceSchemaTokens->attributes };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdPrmanRileyGeometryInstanceSchemaTokens->geoPrototype) {
            return HdRetainedTypedSampledDataSource<SdfPath>::New(
                _protoPrimPath);
        }
        if (name == HdPrmanRileyGeometryInstanceSchemaTokens->material) {
            return HdRetainedTypedSampledDataSource<SdfPath>::New(
                HdPrman_RileyFallbackMaterialSceneIndexPlugin::
                GetFallbackMaterialPath());
        }
        if (name == HdPrmanRileyGeometryInstanceSchemaTokens->xform) {
            return HdXformSchema::GetFromParent(_primSource).GetMatrix();
        }
        if (name == HdPrmanRileyGeometryInstanceSchemaTokens->attributes) {
            HdContainerDataSourceHandle const params =
                HdRetainedContainerDataSource::New(
                    TfToken("identifierName"),
                    HdPrmanRileyParamSchema::Builder()
                        .SetValue(
                            HdRetainedTypedSampledDataSource<TfToken>::New(
                                _protoPrimPath.GetToken()))
                        .Build(),
                    TfToken("lightfilter:subset"),
                    HdPrmanRileyParamSchema::Builder()
                         .SetValue(
                             HdRetainedTypedSampledDataSource<TfToken>::New(
                                 TfToken()))
                         .Build(),
                    TfToken("lighting:subset"),
                    HdPrmanRileyParamSchema::Builder()
                         .SetValue(
                             HdRetainedTypedSampledDataSource<TfToken>::New(
                                 TfToken("default")))
                         .Build(),
                    TfToken("user:hydra:doubleSided"),
                    HdPrmanRileyParamSchema::Builder()
                         .SetValue(
                             HdRetainedTypedSampledDataSource<int>::New(
                                 0))
                         .Build());

            return
                HdPrmanRileyParamListSchema::Builder()
                    .SetParams(params)
                    .Build();
        }
        return nullptr;
    }

private:
    const SdfPath _protoPrimPath;
    HdContainerDataSourceHandle const _primSource;
};

class _RileyConversionSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _RileyConversionSceneIndexRefPtr New(
        HdSceneIndexBaseRefPtr const &inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _RileyConversionSceneIndex(inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override
    {
        if (primPath.GetName() == _tokens->geometryInstance) {
            
            const SdfPath protoPrimPath = primPath.GetParentPath();

            const HdSceneIndexPrim protoPrim = _GetInputSceneIndex()->GetPrim(protoPrimPath);
            if (!protoPrim.dataSource) {
                return protoPrim;
            }

            return {
                HdPrmanRileyPrimTypeTokens->geometryInstance,
                HdRetainedContainerDataSource::New(
                    HdPrmanRileyGeometryInstanceSchema::GetSchemaToken(),
                    _GeometryInstanceDataSource::New(
                        protoPrimPath, protoPrim.dataSource)) };
        }

        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        if (!prim.dataSource) {
            return prim;
        }

        if (prim.primType == HdPrimTypeTokens->sphere) {
            return {
                HdPrmanRileyPrimTypeTokens->geometryPrototype,
                HdRetainedContainerDataSource::New(
                    HdPrmanRileyGeometryPrototypeSchema::GetSchemaToken(),
                    HdPrmanRileyGeometryPrototypeSchema::Builder()
                        .SetType(
                            HdRetainedTypedSampledDataSource<TfToken>::New(
                                TfToken("Ri:Sphere")))
                        .SetPrimvars(
                            HdPrmanRileyPrimvarListSchema::Builder()
                                .SetNumUniform(
                                    HdRetainedTypedSampledDataSource<size_t>::New(
                                        1))
                                .SetNumVertex(
                                    HdRetainedTypedSampledDataSource<size_t>::New(
                                        1))
                                .SetNumVarying(
                                    HdRetainedTypedSampledDataSource<size_t>::New(
                                        1))
                                .SetNumFaceVarying(
                                    HdRetainedTypedSampledDataSource<size_t>::New(
                                        1))
                                .SetParams(
                                    _GeometryPrototypeParamsDataSource::New(
                                        prim.dataSource))
                                .Build())
                        .Build()) };
        }

        return prim;
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override
    {
        SdfPathVector children = _GetInputSceneIndex()->GetChildPrimPaths(primPath);

        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        if (prim.dataSource && prim.primType == HdPrimTypeTokens->sphere) {
            children.push_back(
                primPath.AppendChild(
                    _tokens->geometryInstance));
        }
        
        return children;
    }

protected:
    _RileyConversionSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex)
     : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    {
    }

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override
    {
        HdSceneIndexObserver::AddedPrimEntries newEntries;

        for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
            if (entry.primType == HdPrimTypeTokens->sphere) {
                newEntries.push_back(
                    { entry.primPath,
                      HdPrmanRileyPrimTypeTokens->geometryPrototype });
                newEntries.push_back(
                    { entry.primPath.AppendChild(
                            _tokens->geometryInstance),
                      HdPrmanRileyPrimTypeTokens->geometryInstance });
            } else {
                newEntries.push_back(entry);
            }
        }

        _SendPrimsAdded(newEntries);
    }

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override
    {
        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override
    {
        HdSceneIndexObserver::DirtiedPrimEntries addlEntries;

        for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
            if (entry.dirtyLocators.Intersects(HdXformSchema::GetDefaultLocator())) {
                addlEntries.push_back(
                    { entry.primPath.AppendChild(
                            _tokens->geometryInstance),
                      HdPrmanRileyGeometryInstanceSchema::GetXformLocator() });
            }
            if (entry.dirtyLocators.Intersects(HdSphereSchema::GetDefaultLocator())) {
                addlEntries.push_back(
                    { entry.primPath,
                      HdPrmanRileyGeometryPrototypeSchema::GetPrimvarsLocator() });
            }
        }

        _SendPrimsDirtied(entries);
        if (!addlEntries.empty()) {
            _SendPrimsDirtied(addlEntries);
        }
    }
};

#endif

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_RileyConversionSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 101;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _rendererDisplayName,
        _tokens->sceneIndexPluginName,
        /* inputArgs = */ nullptr,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtEnd);
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index plugin Implementation
////////////////////////////////////////////////////////////////////////////////

HdPrman_RileyConversionSceneIndexPlugin::
HdPrman_RileyConversionSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_RileyConversionSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
    if (!TfGetEnvSetting(HD_PRMAN_EXPERIMENTAL_RILEY_SCENE_INDEX_OBSERVER)) {
        return inputScene;
    }

    return _RileyConversionSceneIndex::New(inputScene);
#else 
    return inputScene;
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
