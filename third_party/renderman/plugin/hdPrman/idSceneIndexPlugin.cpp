//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/idSceneIndexPlugin.h"

#if PXR_VERSION >= 2605

#include "hdPrman/debugCodes.h"
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/version.h"

#include "pxr/base/arch/hash.h"
#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primOriginSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/schema.h" 
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdPrman_IdSceneIndex);

class HdPrman_IdSceneIndex : 
    public  HdSingleInputFilteringSceneIndexBase
{
public:
    static HdPrman_IdSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    HdPrman_IdSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex);
    ~HdPrman_IdSceneIndex();

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;
};

static HdDataSourceBaseHandle
_UpdatePrimvars(
    SdfPath const& primPath,
    HdContainerDataSourceHandle const& inputPrimDs,
    const HdSceneIndexBasePtr inputScene)
{
    // Create a primvars container data source if we don't already have one.
    HdContainerDataSourceHandle container;
    HdPrimvarsSchema schema = HdPrimvarsSchema::GetFromParent(inputPrimDs);
    if (schema) {
        container = schema.GetContainer();
    } else {
        container = HdPrimvarsSchema::BuildRetained(0, nullptr, nullptr);
    }
    HdContainerDataSourceEditor primvarsEditor(container);

    // First, look up the primOrigin/scenePath, if it exists;
    // fall back to the scene index prim path, otherwise.
    SdfPath originPath = primPath;
    if (auto originSchema =
        HdPrimOriginSchema::GetFromParent(inputPrimDs)) {
        originPath =
            originSchema.GetOriginPath(HdPrimOriginSchemaTokens->scenePath);
    }

    HdInstancerTopologySchema instancerTopologySchema =
        HdInstancerTopologySchema::GetFromParent(inputPrimDs);

    if (!instancerTopologySchema
        && HdInstancedBySchema::GetFromParent(inputPrimDs)) {
        // Prototype prim that is being instanced.
        // Provide primOrigin so that we can assign per-prim ID's.
        TfToken primOriginAttrName("user:primOrigin");
        primvarsEditor.Overlay(
            HdDataSourceLocator(
                primOriginAttrName),
            HdPrimvarSchema::Builder()
            .SetPrimvarValue(
                HdRetainedTypedSampledDataSource<std::string>::New(
                    originPath.GetString()))
            .SetInterpolation(
                HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->constant))
            .Build());
    }

    if (HdPathArrayDataSourceHandle instanceLocationsDs =
        instancerTopologySchema.GetInstanceLocations())
    {
        // Instancer with instancer locations specified.
        // In USD, this corresponds to native scenegraph instancing.

        VtArray<SdfPath> instanceLocations =
            instanceLocationsDs->GetTypedValue(0.0f);
        const size_t numInstances = instanceLocations.size();

        VtArray<std::string> instanceLocationStrArray(numInstances);

        // Find origin for each instance.
        for (size_t i=0; i<numInstances; ++i) {
            HdSceneIndexPrim instancePrim =
                inputScene->GetPrim(instanceLocations[i]);
            if (auto originSchema =
                HdPrimOriginSchema::GetFromParent(instancePrim.dataSource)) {
                instanceLocationStrArray[i] =
                    originSchema.GetOriginPath(
                        HdPrimOriginSchemaTokens->scenePath).GetString();
            } else {
                instanceLocationStrArray[i] = instanceLocations[i].GetString();
            }
        }

        // Set as instanceLocations primvar.
        primvarsEditor.Overlay(
            HdDataSourceLocator(
                HdInstancerTopologySchemaTokens->instanceLocations),
            HdPrimvarSchema::Builder()
            .SetPrimvarValue(
                HdRetainedTypedSampledDataSource<VtArray<std::string>>::New(
                    instanceLocationStrArray))
            .SetInterpolation(
                HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->instance))
            .Build());

    } else if (HdIntArrayVectorSchema instanceIndicesSchema =
               instancerTopologySchema.GetInstanceIndices())
    {
        // Instancer without instance locations specified.
        // In USD, this corresponds to a UsdGeomPoinstInstancer.

        // Tally up the instance count for each prototype to find
        // the total number of instances.
        size_t numInstances = 0;
        for (size_t i=0, n=instanceIndicesSchema.GetNumElements(); i<n; ++i) {
            VtArray<int> instanceIndices =
                instanceIndicesSchema.GetElement(i)->GetTypedValue(0.0);
            numInstances += instanceIndices.size();
        }

        VtArray<std::string> instanceLocationStrArray(numInstances);
        for (size_t i=0; i<numInstances; ++i) {
            instanceLocationStrArray[i] =
                originPath.GetString() + TfStringPrintf("[%zu]", i);
        }

        // Set as instanceLocations primvar.
        primvarsEditor.Overlay(
            HdDataSourceLocator(
                HdInstancerTopologySchemaTokens->instanceLocations),
            HdPrimvarSchema::Builder()
            .SetPrimvarValue(
                HdRetainedTypedSampledDataSource<VtArray<std::string>>::New(
                    instanceLocationStrArray))
            .SetInterpolation(
                HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->instance))
            .Build());
    }

    return primvarsEditor.Finish();
}

// This data source overlays the primvars schema with ID assignments.
class HdPrman_IdDataSource
    : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(HdPrman_IdDataSource);

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names = _inputPrimDs->GetNames();
        if (std::find(names.begin(), names.end(),
                      HdPrimvarsSchemaTokens->primvars) == names.end()) {
            names.push_back(HdPrimvarsSchemaTokens->primvars);
        }
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (name == HdPrimvarsSchemaTokens->primvars) {
            return _UpdatePrimvars(_primPath, _inputPrimDs, _inputScene);
        }
        return _inputPrimDs->Get(name);
    }

protected:
    HdPrman_IdDataSource(
        SdfPath const& primPath,
        HdContainerDataSourceHandle const& inputDs,
        HdSceneIndexBasePtr const& inputScene)
    : _primPath(primPath), _inputPrimDs(inputDs), _inputScene(inputScene)
    {
    }

private:
    const SdfPath _primPath;
    const HdContainerDataSourceHandle _inputPrimDs;
    const HdSceneIndexBasePtr _inputScene;
};



/* static */
HdPrman_IdSceneIndexRefPtr
HdPrman_IdSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(  
        new HdPrman_IdSceneIndex(inputSceneIndex));
}

HdPrman_IdSceneIndex::HdPrman_IdSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{ }

static bool
_IsGeometryOrInstancerType(TfToken const& primType)
{
    return
        primType == HdPrimTypeTokens->mesh ||
        primType == HdPrimTypeTokens->basisCurves ||
        primType == HdPrimTypeTokens->points ||
        primType == HdPrimTypeTokens->volume ||
        primType == HdPrimTypeTokens->cone ||
        primType == HdPrimTypeTokens->cylinder ||
        primType == HdPrimTypeTokens->sphere ||
        primType == HdPrimTypeTokens->instancer;
}

HdSceneIndexPrim 
HdPrman_IdSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (_IsGeometryOrInstancerType(prim.primType) && prim.dataSource) {
        return {
            prim.primType,
            HdPrman_IdDataSource::New(primPath, prim.dataSource,
                _GetInputSceneIndex())
        };
    }
    return prim;
}

SdfPathVector 
HdPrman_IdSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdPrman_IdSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{    
    // TODO: invalidate when instancer topology changes
    _SendPrimsAdded(entries);
}

void 
HdPrman_IdSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdPrman_IdSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    // TODO: invalidate when instancer topology changes
    _SendPrimsDirtied(entries);
}

HdPrman_IdSceneIndex::
~HdPrman_IdSceneIndex() = default;

// -----------------------------------------------------------------------------

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry
        ::Define<HdPrman_IdSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 115;

    for (const auto& rendererDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance()
            .RegisterSceneIndexForRenderer(
            rendererDisplayName,
            HdPrmanPluginTokens->idAssigning,
            /* inputArgs = */ nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtEnd);
    }
}

HdSceneIndexBaseRefPtr
HdPrman_IdSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputScene,
    const HdContainerDataSourceHandle& inputArgs)
{
    TF_UNUSED(inputArgs);
    return HdPrman_IdSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2605
