//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/renderIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

//! [QuadSceneIndex_Example]
class QuadSceneIndex : public HdSceneIndexBase {
public:
    void Populate(bool populate) {
        if (populate && !_isPopulated) {
            _SendPrimsAdded({{SdfPath("/Quad"), HdPrimTypeTokens->mesh}});
        } else if (!populate && _isPopulated) {
            _SendPrimsRemoved({{SdfPath("/Quad")}});
        }
        _isPopulated = populate;
    }

    virtual HdSceneIndexPrim GetPrim(const SdfPath &primPath) const {
        static HdSceneIndexPrim prim = {
            HdPrimTypeTokens->mesh,
            HdRetainedContainerDataSource::New(
                HdMeshSchemaTokens->mesh,
                    HdMeshSchema::Builder()
                        .SetTopology(
                            HdMeshTopologySchema::Builder()
                                .SetFaceVertexCounts(
                                    HdRetainedTypedSampledDataSource<
                                        VtIntArray>::New({4}))
                                .SetFaceVertexIndices(
                                    HdRetainedTypedSampledDataSource<
                                        VtIntArray>::New({0, 1, 2, 3}))
                                .SetOrientation(
                                    HdMeshTopologySchema::
                                        BuildOrientationDataSource(
                                            HdMeshTopologySchemaTokens
                                                ->rightHanded))
                                .SetHoleIndices(nullptr)
                                .Build())
                        .SetSubdivisionScheme(
                            HdRetainedTypedSampledDataSource<TfToken>::New(
                                PxOsdOpenSubdivTokens->none))
                        .SetSubdivisionTags(nullptr)
                        .SetGeomSubsets(nullptr)
                        .SetDoubleSided(
                            HdRetainedTypedSampledDataSource<bool>::New(true))
                        .Build(),
                HdPrimvarsSchemaTokens->primvars,
                    HdRetainedContainerDataSource::New(
                        HdPrimvarsSchemaTokens->points,
                        HdPrimvarSchema::Builder()
                            .SetPrimvarValue(
                                HdRetainedTypedSampledDataSource<VtVec3fArray>::New(
                                    {{-1, -1, 0},{1, -1, 0},{1, 1, 0},{-1, 1, 0}}))
                            .SetIndexedPrimvarValue(nullptr)
                            .SetIndices(nullptr)
                            .SetInterpolation(
                                HdPrimvarSchema::BuildInterpolationDataSource(
                                    HdPrimvarSchemaTokens->vertex))
                            .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                                HdPrimvarSchemaTokens->point))
                            .Build()),
                HdPurposeSchemaTokens->purpose,
                    HdPurposeSchema::Builder()
                        .SetPurpose(
                            HdRetainedTypedSampledDataSource<TfToken>::New(
                                HdRenderTagTokens->geometry))
                        .Build(),
                HdVisibilitySchemaTokens->visibility,
                    HdVisibilitySchema::Builder()
                        .SetVisibility(
                            HdRetainedTypedSampledDataSource<bool>::New(true))
                        .Build(),
                HdXformSchemaTokens->xform,
                    HdXformSchema::Builder()
                        .SetMatrix(
                            HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                                GfMatrix4d(1)))
                        .SetResetXformStack(
                            HdRetainedTypedSampledDataSource<bool>::New(false))
                        .Build(),
                HdExtentSchemaTokens->extent,
                    HdExtentSchema::Builder()
                        .SetMin(HdRetainedTypedSampledDataSource<GfVec3d>::New(
                            GfVec3d(-1, -1, 0)))
                        .SetMax(HdRetainedTypedSampledDataSource<GfVec3d>::New(
                            GfVec3d(1, 1, 0)))
                        .Build()
            )
        };
        if (primPath == SdfPath("/Quad")) {
            return prim;
        } else {
            return { TfToken(), nullptr };
        }
    }

    virtual SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const {
        if (primPath == SdfPath::AbsoluteRootPath()) {
            return { SdfPath("/Quad") };
        } else {
            return {};
        }
    }

private:
    bool _isPopulated;
};
//! [QuadSceneIndex_Example]

//! [UnboxingSceneIndexFilter]
class UnboxingSceneIndexFilter : public HdSingleInputFilteringSceneIndexBase {
public:
    HdSceneIndexBaseRefPtr New(const HdSceneIndexBaseReftPtr &inputSceneIndex) {...}

    virtual HdSceneIndexPrim GetPrim(const SdfPath &primPath) const {
        HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        if (prim.primType == HdPrimTypeTokens->cube) {
            return { TfToken(), nullptr };
        }
        return prim;
    }

    virtual SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const {
        return _GetInputSceneIndex->GetChildPrimPaths(primPath);
    }

protected:
    HdSingleInputFilteringSceneIndexBase(const HdSceneIndexBaseRefPtr &inputSceneIndex) {...}

    virtual void _PrimsAdded(const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::AddedPrimEntries &entries) {
        HdSceneIndexObserver::AddedPrimEntries filtered;
        std::copy_if(entries.begin(), entries.end(), std::back_inserter(filtered),
            [](const HdSceneIndexObserver::AddedPrimEntry &entry) {
                return entry.primType != HdPrimTypeTokens->cube; });
        _SendPrimsAdded(filtered);
    }

    virtual void _PrimsRemoved(const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::RemovedPrimEntries &entries) {
        _SendPrimsRemoved(entries);
    }
    virtual void _PrimsDirtied(const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) {
        _SendPrimsDirtied(entries);
    }
};
//! [UnboxingSceneIndexFilter]

//! [GreeningSceneIndexFilter]
class GreeningSceneIndexFilter : public HdSingleInputFilteringSceneIndexBase {
public:
    HdSceneIndexBaseRefPtr New(const HdSceneIndexBaseReftPtr &inputSceneIndex) {...}

    virtual HdSceneIndexPrim GetPrim(const SdfPath &primPath) const {
        HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        if (HdPrimTypeIsGprim(prim.primType)) {
            HdContainerDataSourceEditor e(prim.dataSource);
            e.Set(HdDataSourceLocator(HdPrimvarsSchemaTokens->primvars, HdTokens->displayColor),
                HdPrimvarSchema::BuildRetained(
                    /* primvarValue = */ HdVec3fDataSource::New({0,1.0f,0}),
                    /* indexedPrimvarValue = */ nullptr,
                    /* indices = */ nullptr,
                    /* interpolation = */ HdTokenDataSource::New(HdTokens->constant),
                    /* role = */ HdTokenDataSource::New(HdPrimvarSchemaTokens->color)));
            prim.dataSource = e.Finish();

            // Note that you could also do:
#if 0
            prim.dataSource = HdOverlayContainerDataSource::New(
                HdRetainedContainerDataSource::New(
                    HdPrimvarsSchemaTokens->primvars,
                    HdRetainedContainerDataSource::New(
                        HdTokens->displayColor,
                        HdPrimvarSchema::BuildRetained(...))),
                prim.dataSource);
#endif
        }
        return prim;
    }

    virtual SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const {
        return _GetInputSceneIndex->GetChildPrimPaths(primPath);
    }

protected:
    HdSingleInputFilteringSceneIndexBase(const HdSceneIndexBaseRefPtr &inputSceneIndex) {...}

    virtual void _PrimsAdded(const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::AddedPrimEntries &entries) {
        _SendPrimsAdded(entries);
    }

    virtual void _PrimsRemoved(const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::RemovedPrimEntries &entries) {
        _SendPrimsRemoved(entries);
    }
    virtual void _PrimsDirtied(const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) {
        _SendPrimsDirtied(entries);
    }
};
//! [GreeningSceneIndexFilter]

//! [RendererSceneIndexFilterPlugin]
TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<MyFilterPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // integer priority
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 1000;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        "GL", // ... or "Prman" or etc, or "" for any
        MyFilterPlugin,
        nullptr,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtEnd);
}

class MyFilterPlugin : public HdSceneIndexPlugin
{
public:
    MyFilterPlugin();    

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

HdSceneIndexBaseRefPtr
MyFilterPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return MyFilter::New(inputScene);
}
//! [RendererSceneIndexFilterPlugin]

//! [AppSceneIndexFilter]
HdSceneIndexBaseRefPtr
_AppendAppFilterSceneIndexCallback(
        void *appState,
        const std::string &renderInstanceId,
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs)
{
    // Note that if appState were a map from "renderInstanceId" to e.g. a list of
    // per-viewport overrides, we could pull them out of appState here when creating the
    // hydra instance for each viewport:
    // auto foo = static_cast<_StateMap*>(appState)->at(rendererInstanceId);
    return AppFilterSceneIndex::New(inputScene);
}

void AppStartup()
{
    // integer priority
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        "",
        std::bind(_AppendAppFilterSceneIndexCallback, appState, _1, _2, _3),
        /* inputArgs = */ nullptr,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart
        );
}
//! [AppSceneIndexFilter]

//! [MyUSDPrimAdapter]
TF_REGISTRY_FUNCTION(TfType)
{
    typedef MyUSDPrimAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

class MyUSDPrimAdapter : public UsdImagingPrimAdapter {
public:
    typedef UsdImagingPrimAdapter BaseAdapter;

    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    TfToken GetImagingSubprimType(
        UsdPrim const& prim, TfToken const& subprim) override;

    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    HdDataSourceLocatorSet InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        UsdImagingPropertyInvalidationType invalidationType) override;
};
//! [MyUSDPrimAdapter]

//! [MyUSDAPIAdapter]
TF_REGISTRY_FUNCTION(TfType)
{
    typedef MyUSDAPIAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}

class MyUSDAPIAdapter : public UsdImagingAPISchemaAdapter {
public:
    typedef UsdImagingAPISchemaAdapter BaseAdapter;

    TfTokenVector GetImagingSubprims(
            UsdPrim const& prim,
            TfToken const& appliedInstanceName) override;

    TfToken GetImagingSubprimType(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName) override;

    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType) override;
};
//! [MyUSDAPIAdapter]

//! [SyncWithSceneIndexAPI]
void MyPrimSync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits, TfToken const& reprToken)
{
    HdSceneIndexPrim siPrim =
        sceneDelegate->GetRenderIndex().GetTerminalSceneIndex()->GetPrim(GetId());
    // ... Pull data from siPrim instead of sceneDelegate ...
}
//! [SyncWithSceneIndexAPI]

//! [HdSceneIndexBaseExcerpt]
struct HdSceneIndexPrim {
    TfToken primType;
    HdContainerDataSourceHandle dataSource;
};

class HdSceneIndexBase {
public:
    virtual HdSceneIndexPrim GetPrim(const SdfPath &primPath) const = 0;
    virtual SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const = 0;

protected:
    void _SendPrimsAdded(const HdSceneIndexObserver::AddedPrimEntries &entries);
    void _SendPrimsRemoved(const HdSceneIndexObserver::RemovedPrimEntries &entries);
    void _SendPrimsDirtied(const HdSceneIndexObserver::DirtiedPrimEntries &entries);
    void _SendPrimsRenamed(const HdSceneIndexObserver::RenamedPrimEntries &entries);
};
//! [HdSceneIndexBaseExcerpt]

//! [HdSceneIndexObserverExcerpt]
class HdSceneIndexObserver {
public:
    struct AddedPrimEntry {
        // NOTE: if primPath has already been added, this notice acts as a resync;
        // observers should re-fetch all data and note the potentially new prim type.
        SdfPath primPath;
        TfToken primType;
    };
    virtual void PrimsAdded(const HdSceneIndexBase &sender,
                            const AddedPrimEntries &entries) = 0;

    struct RemovedPrimEntry {
        // NOTE: primPath here is a subtree root; this notice means that primPath and all
        // descendants have been removed.
        SdfPath primPath;
    };
    virtual void PrimsRemoved(const HdSceneIndexBase &sender,
                              const RemovedPrimEntries &entries) = 0;

    struct DirtiedPrimEntry {
        SdfPath primPath;
        // NOTE: locators are interpreted hierarchically; i.e. if "primvars" is in
        // dirtyLocators, the value of "primvars/points" is also considered dirty.
        HdDataSourceLocatorSet dirtyLocators;
    };
    virtual void PrimsDirtied(const HdSceneIndexBase &sender,
                              const DirtiedPrimEntries &entries) = 0;

    struct RenamedPrimEntry {
        // NOTE: oldPrimPath and newPrimPath here are subtree roots; this notice means that
        // oldPrimPath and all descendants have been re-rooted to the location newPrimPath.
        SdfPath oldPrimPath;
        SdfPath newPrimPath;
    };
    virtual void PrimsRenamed(const HdSceneIndexBase &sender,
                              const RenamedPrimEntries &entries) = 0;
};
//! [HdSceneIndexObserverExcerpt]

//! [HdDataSourceBaseSpecializationsExcerpt]
class HdContainerDataSource {
public:
    virtual TfTokenVector GetNames() = 0;
    virtual HdDataSourceBaseHandle Get(const TfToken &name) = 0;
};

class HdVectorDataSource {
public:
    virtual size_t GetNumElements() = 0;
    virtual HdDataSourceBaseHandle GetElement(size_t element) = 0;
};

class HdSampledDataSource {
public:
    virtual VtValue GetValue(Time shutterOffset) = 0;
    virtual bool GetContributingSampleTimesForInterval(
        Time startTime, Time endTime, std::vector<Time> * outSampleTimes) = 0;
};
//! [HdDataSourceBaseSpecializationsExcerpt]

//! [HdDataSourceLocatorExcerpt]
class HdDataSourceLocator {
public:
    HdDataSourceLocator(size_t count, const TfToken *tokens);
    bool HasPrefix(const HdDataSourceLocator &prefix) const;
    // ... plus, other manipulation & testing code.
};
//! [HdDataSourceLocatorExcerpt]

//! [HdMeshTopologySchemaExcerpt]
class HdMeshTopologySchema : public HdSchema {
public:
    HdMeshTopologySchema(HdContainerDataSourceHandle container);

    HdIntArrayDataSourceHandle GetFaceVertexCounts();
    HdIntArrayDataSourceHandle GetFaceVertexIndices();
    HdIntArrayDataSourceHandle GetHoleIndices();
    HdTokenDataSourceHandle GetOrientation();
};
//! [HdMeshTopologySchemaExcerpt]

//! [HdSingleInputFilteringSceneIndexBaseExcerpt]
class HdSingleInputFilteringSceneIndexBase : public HdSceneIndexBase {
public:
    // ... from HdSceneIndexBase
    virtual HdSceneIndexPrim GetPrim(const SdfPath &primPath) const = 0;
    virtual SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const = 0;

protected:
    HdSingleInputFilteringSceneIndexBase(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

    virtual void _PrimsAdded(const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) = 0;
    virtual void _PrimsRemoved(const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) = 0;
    virtual void _PrimsDirtied(const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) = 0;
    virtual void _PrimsRenamed(const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RenamedPrimEntries &entries);

private:
    const HdSceneIndexBaseRefPtr &_GetInputSceneIndex() const;
};
//! [HdSingleInputFilteringSceneIndexBaseExcerpt]

//! [HdSceneIndexPrimViewExample]
// From a notice handler, iterate through the descendants of rootPath
// in the input scene
for (const SdfPath &primPath : HdSceneIndexPrimView(_GetInputSceneIndex(), rootPath)) 
{
    ....
}
//! [HdSceneIndexPrimViewExample]


PXR_NAMESPACE_CLOSE_SCOPE
