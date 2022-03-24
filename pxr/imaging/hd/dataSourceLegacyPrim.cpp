//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hd/dataSourceLegacyPrim.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/basisCurvesSchema.h"
#include "pxr/imaging/hd/basisCurvesTopologySchema.h"
#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/categoriesSchema.h"
#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/dataSharingSchema.h"
#include "pxr/imaging/hd/extComputationInputComputationSchema.h"
#include "pxr/imaging/hd/extComputationOutputSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarsSchema.h"
#include "pxr/imaging/hd/extComputationSchema.h"
#include "pxr/imaging/hd/extentSchema.h"
#include "pxr/imaging/hd/geomSubsetSchema.h"
#include "pxr/imaging/hd/geomSubsetsSchema.h"
#include "pxr/imaging/hd/instanceCategoriesSchema.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/instanceSchema.h"
#include "pxr/imaging/hd/legacyDisplayStyleSchema.h"
#include "pxr/imaging/hd/lightSchema.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/renderBufferSchema.h"
#include "pxr/imaging/hd/subdivisionTagsSchema.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hd/volumeFieldBindingSchema.h"
#include "pxr/imaging/hd/volumeFieldSchema.h"
#include "pxr/imaging/hd/xformSchema.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdLegacyPrimTypeTokens, HD_LEGACY_PRIMTYPE_TOKENS);

// XXX: currently private and duplicated where used so as to not yet formally
//      define this convention.
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (prmanParams)
    ((prmanParamsNames, ""))
);

// ----------------------------------------------------------------------------

bool
HdLegacyPrimTypeIsVolumeField(TfToken const &primType)
{
    return (primType == HdLegacyPrimTypeTokens->openvdbAsset ||
            primType == HdLegacyPrimTypeTokens->field3dAsset);
}

// ----------------------------------------------------------------------------

namespace {

class Hd_DataSourceLegacyPrimvarValue : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceLegacyPrimvarValue);

    Hd_DataSourceLegacyPrimvarValue(
        const TfToken &primvarName,
        const SdfPath &primId,
        HdSceneDelegate *sceneDelegate)
    : _primvarName(primvarName)
    , _primId(primId)
    , _sceneDelegate(sceneDelegate)
    {
        TF_VERIFY(_sceneDelegate);
    }

    VtValue GetValue(Time shutterOffset) override
    {
        if (shutterOffset == 0) {
            VtValue result = _sceneDelegate->Get(_primId, _primvarName);
            if (!result.IsEmpty()) {
                return result;
            }

            // XXX: In UsdImaging, lights derived from the base prim adapter
            //      directly and therefore its Get doesn't have "primvars:"
            //      namespace awareness. It is supported by SamplePrimvar so
            //      we can handle we'll fall back to use that if the Get
            //      query fails
            float sampleTimes;
            _sceneDelegate->SamplePrimvar(
                  _primId, _primvarName, 1,  &sampleTimes,  &result);

            return result;
        } else {
            _GetTimeSamples();
            return _timeSamples.Resample(shutterOffset);
        }
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime, Time endTime,
        std::vector<Time> *outSampleTimes) override
    {
        _GetTimeSamples();

        // XXX: Start and end times come from the sene delegate, so we can't
        // get samples outside of those provided. However, we can clamp
        // returned samples to be in the right range.
        if (outSampleTimes != nullptr) {
            for (const float &t : _timeSamples.times) {
                if (t >= startTime && t <= endTime) {
                    outSampleTimes->push_back(t);
                }
            }
        }

        return true;
    }

private:
    void _GetTimeSamples()
    {
        if (_timeSamples.count == 0) {
            _sceneDelegate->SamplePrimvar(_primId, _primvarName, &_timeSamples);
        }
    }

    TfToken _primvarName;
    SdfPath _primId;
    HdTimeSampleArray<VtValue, 1> _timeSamples;
    HdSceneDelegate *_sceneDelegate;
};

// ----------------------------------------------------------------------------

class Hd_DataSourceLegacyIndexedPrimvarValue : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceLegacyIndexedPrimvarValue);

    Hd_DataSourceLegacyIndexedPrimvarValue(
        const TfToken &primvarName,
        const SdfPath &primId,
        HdSceneDelegate *sceneDelegate)
    : _primvarName(primvarName)
    , _primId(primId)
    , _sceneDelegate(sceneDelegate)
    {
        TF_VERIFY(_sceneDelegate);
    }

    VtValue GetValue(Time shutterOffset) override
    {
        if (shutterOffset == 0) {
            VtIntArray indices(0);
            return _sceneDelegate->GetIndexedPrimvar(_primId, _primvarName, 
                    &indices);
            
        } else {
            _GetTimeSamples();
            std::pair<VtValue, VtIntArray> pv =  
                _timeSamples.ResampleIndexed(shutterOffset);
            return pv.first;
        }
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time> *outSampleTimes) override
    {
        _GetTimeSamples();

        // XXX: Start and end times come from the sene delegate, so we can't
        // get samples outside of those provided. However, we can clamp
        // returned samples to be in the right range.
        if (outSampleTimes != nullptr) {
            for (const float &t : _timeSamples.times) {
                if (t >= startTime && t <= endTime) {
                    outSampleTimes->push_back(t);
                }
            }
        }

        return true;
    }

private:
    void _GetTimeSamples()
    {
        if (_timeSamples.count == 0) {
            _sceneDelegate->SampleIndexedPrimvar(_primId, _primvarName, 
                    &_timeSamples);
        }
    }

    TfToken _primvarName;
    SdfPath _primId;
    HdIndexedTimeSampleArray<VtValue, 1> _timeSamples;
    HdSceneDelegate *_sceneDelegate;
};

// ----------------------------------------------------------------------------

class Hd_DataSourceLegacyPrimvarIndices : 
    public HdTypedSampledDataSource<VtArray<int>>
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceLegacyPrimvarIndices);

    Hd_DataSourceLegacyPrimvarIndices(
        const TfToken &primvarName,
        const SdfPath &primId,
        HdSceneDelegate *sceneDelegate)
    : _primvarName(primvarName)
    , _primId(primId)
    , _sceneDelegate(sceneDelegate)
    {
        TF_VERIFY(_sceneDelegate);
    }

    VtValue GetValue(Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtIntArray GetTypedValue(Time shutterOffset) override
    {
        if (shutterOffset == 0) {
            VtIntArray indices(0);
            VtValue value = _sceneDelegate->GetIndexedPrimvar(_primId, 
                    _primvarName, &indices);
            return indices;
        } else {
            _GetTimeSamples();
            std::pair<VtValue, VtIntArray> pv = 
                    _timeSamples.ResampleIndexed(shutterOffset);
            return pv.second;
        }
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time> *outSampleTimes) override
    {
        _GetTimeSamples();

        // XXX: Start and end times come from the sene delegate, so we can't
        // get samples outside of those provided. However, we can clamp
        // returned samples to be in the right range.
        if (outSampleTimes != nullptr) {
            for (const float &t : _timeSamples.times) {
                if (t >= startTime && t <= endTime) {
                    outSampleTimes->push_back(t);
                }
            }
        }

        return true;
    }

private:
    void _GetTimeSamples()
    {
        if (_timeSamples.count == 0) {
            _sceneDelegate->SampleIndexedPrimvar(
                _primId, _primvarName, &_timeSamples);
        }
    }

    TfToken _primvarName;
    SdfPath _primId;
    HdIndexedTimeSampleArray<VtValue, 1> _timeSamples;
    HdSceneDelegate *_sceneDelegate;
};

// ----------------------------------------------------------------------------

class Hd_DataSourceLegacyPrimvarsContainer : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceLegacyPrimvarsContainer);

    Hd_DataSourceLegacyPrimvarsContainer(
        const SdfPath &primId,
        HdSceneDelegate *sceneDelegate)
    : _primId(primId)
    , _sceneDelegate(sceneDelegate)
    {
        TF_VERIFY(_sceneDelegate);
    }

    void AddDesc(const TfToken &name, const TfToken &interpolation,
        const TfToken &role, bool indexed)
    {
        _entries[name] = {interpolation, role, indexed};
    }

    bool Has(const TfToken &name) override
    {
        return _entries.find(name) != _entries.end();
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector result;
        result.reserve(_entries.size());
        for (const auto &pair : _entries) {
            result.push_back(pair.first);
        }
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        _EntryMap::const_iterator it = _entries.find(name);
        if (it == _entries.end()) {
            return nullptr;
        }

        if ((*it).second.indexed) {
            return HdPrimvarSchema::Builder()
                .SetIndexedPrimvarValue(
                    Hd_DataSourceLegacyIndexedPrimvarValue::New(
                        name, _primId, _sceneDelegate))
                .SetIndices(Hd_DataSourceLegacyPrimvarIndices::New(
                    name, _primId, _sceneDelegate))
                .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                    (*it).second.interpolation))
                .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                    (*it).second.role))
                .Build();
        } else {
            return HdPrimvarSchema::Builder()
                .SetPrimvarValue(Hd_DataSourceLegacyPrimvarValue::New(
                    name, _primId, _sceneDelegate))
                .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                    (*it).second.interpolation))
                .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                    (*it).second.role))
                .Build();
        }
    }

private:
    struct _Entry
    {
        TfToken interpolation;
        TfToken role;
        bool indexed;
    };

    using _EntryMap =  TfDenseHashMap<TfToken, _Entry,
            TfToken::HashFunctor, std::equal_to<TfToken>, 32>;

    _EntryMap _entries;
    SdfPath _primId;
    HdSceneDelegate *_sceneDelegate;
};

HD_DECLARE_DATASOURCE_HANDLES(Hd_DataSourceLegacyPrimvarsContainer);

// ----------------------------------------------------------------------------

class Hd_DataSourceLegacyMatrixValue : public HdMatrixDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceLegacyMatrixValue);

    Hd_DataSourceLegacyMatrixValue(
        const TfToken &type,
        const SdfPath &primId,
        HdSceneDelegate *sceneDelegate)
    : _type(type)
    , _primId(primId)
    , _sceneDelegate(sceneDelegate)
    {
        TF_VERIFY(_sceneDelegate);
    }

    VtValue GetValue(Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    GfMatrix4d GetTypedValue(Time shutterOffset) override
    {
        if (shutterOffset == 0) {
            if (_type == HdPrimTypeTokens->instancer) {
                return _sceneDelegate->GetInstancerTransform(_primId);
            } else {
                return _sceneDelegate->GetTransform(_primId);
            }
        } else {
            _GetTimeSamples();
            return _timeSamples.Resample(shutterOffset);
        }
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time> *outSampleTimes) override
    {
        _GetTimeSamples();

        // XXX: Start and end times come from the scene delegate, so we can't
        // get samples outside of those provided. However, we can clamp
        // returned samples to be in the right range.
        if (outSampleTimes != nullptr) {
            for (const float &t : _timeSamples.times) {
                if (t >= startTime && t <= endTime) {
                    outSampleTimes->push_back(t);
                }
            }
        }

        return true;
    }

private:
    void _GetTimeSamples()
    {
        if (_timeSamples.count == 0) {
            if (_type == HdPrimTypeTokens->instancer) {
                _sceneDelegate->SampleInstancerTransform(_primId, 
                    &_timeSamples);
            } else {
                _sceneDelegate->SampleTransform(_primId, &_timeSamples);
            }
        }
    }

    TfToken _type;
    SdfPath _primId;
    HdTimeSampleArray<GfMatrix4d, 1> _timeSamples;
    HdSceneDelegate *_sceneDelegate;
};

// ----------------------------------------------------------------------------

using Hd_MeshTopologyStoreSharedPtr =
    std::shared_ptr<class Hd_MeshTopologyStore>;

class Hd_MeshTopologyStore
{
public:
    Hd_MeshTopologyStore(const SdfPath &id, HdSceneDelegate *sceneDelegate)
    : _id(id)
    , _sceneDelegate(sceneDelegate)
    {
    }

    HdMeshTopologySharedPtr Get()
    {
        HdMeshTopologySharedPtr mt = std::atomic_load(&_meshTopology);
        if (mt) {
            return mt;
        }

        mt = std::make_shared<HdMeshTopology>(
            _sceneDelegate->GetMeshTopology(_id));
        std::atomic_store(&_meshTopology, mt);
        return mt;
    }

    void Invalidate()
    {
        HdMeshTopologySharedPtr nullmt;
        std::atomic_store(&_meshTopology, nullmt);
    }

    template <typename T>
    class MemberDataSource : public HdTypedSampledDataSource<T>
    {
    public:
        HD_DECLARE_DATASOURCE_ABSTRACT(MemberDataSource<T>);

        T GetTypedValue(HdSampledDataSource::Time shutterOffset) override = 0;

        VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
        {
            return VtValue(GetTypedValue(shutterOffset));
        }

        bool GetContributingSampleTimesForInterval( 
                HdSampledDataSource::Time startTime, 
                HdSampledDataSource::Time endTime,
                std::vector<HdSampledDataSource::Time> * outSampleTimes)
                    override
        {
            return false;
        }

        MemberDataSource(const Hd_MeshTopologyStoreSharedPtr &mts)
        : _mts(mts)
        {
        }

    protected:
        Hd_MeshTopologyStoreSharedPtr _mts;
    };


    #define DEFINE_MESH_TOPOLOGY_ACCESSOR_DATASOURCE(N, T, A) \
    class N : public MemberDataSource<T> \
    {                                                                         \
    public:                                                                   \
        HD_DECLARE_DATASOURCE(N);                                             \
                                                                              \
        N(const Hd_MeshTopologyStoreSharedPtr &mts)                           \
        : MemberDataSource(mts) {}                                            \
                                                                              \
        T GetTypedValue(HdSampledDataSource::Time shutterOffset) override     \
        {                                                                     \
            return _mts->Get()->A();                                          \
        }                                                                     \
    };

    DEFINE_MESH_TOPOLOGY_ACCESSOR_DATASOURCE(
        FaceVertexCountsDataSource, VtArray<int>, GetFaceVertexCounts);
    DEFINE_MESH_TOPOLOGY_ACCESSOR_DATASOURCE(
        FaceVertexIndicesDataSource, VtArray<int>, GetFaceVertexIndices);
    DEFINE_MESH_TOPOLOGY_ACCESSOR_DATASOURCE(
        HoleIndicesDataSource, VtArray<int>, GetHoleIndices);
    DEFINE_MESH_TOPOLOGY_ACCESSOR_DATASOURCE(
        OrientationDataSource, TfToken, GetOrientation);
    DEFINE_MESH_TOPOLOGY_ACCESSOR_DATASOURCE(
        SubdivisionSchemeDataSource, TfToken, GetScheme);

private:
    SdfPath _id;
    HdSceneDelegate *_sceneDelegate;
    HdMeshTopologySharedPtr _meshTopology;
};


class Hd_DataSourceMeshTopology : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceMeshTopology);
    Hd_DataSourceMeshTopology(const Hd_MeshTopologyStoreSharedPtr &mts)
    : _mts(mts)
    {
    }

    bool Has(const TfToken &name) override
    {
        if (name == HdMeshTopologySchemaTokens->faceVertexCounts
                || name == HdMeshTopologySchemaTokens->faceVertexIndices
                || name == HdMeshTopologySchemaTokens->holeIndices
                || name == HdMeshTopologySchemaTokens->orientation) {

            return true;
        }
        return false;
    }

    TfTokenVector GetNames() override
    {
        return {
            HdMeshTopologySchemaTokens->faceVertexCounts,
            HdMeshTopologySchemaTokens->faceVertexIndices,
            HdMeshTopologySchemaTokens->holeIndices,
            HdMeshTopologySchemaTokens->orientation,
        };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdMeshTopologySchemaTokens->faceVertexCounts) {
            return Hd_MeshTopologyStore::FaceVertexCountsDataSource::New(_mts);
        }
        if (name == HdMeshTopologySchemaTokens->faceVertexIndices) {
            return Hd_MeshTopologyStore::FaceVertexIndicesDataSource::New(_mts);
        }
        if (name == HdMeshTopologySchemaTokens->holeIndices) {
            return Hd_MeshTopologyStore::HoleIndicesDataSource::New(_mts);
        }
        if (name == HdMeshTopologySchemaTokens->orientation) {
            return Hd_MeshTopologyStore::OrientationDataSource::New(_mts);
        }

        return nullptr;
    }

private:
    Hd_MeshTopologyStoreSharedPtr _mts;
};


class Hd_DataSourceMesh : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceMesh);

    Hd_DataSourceMesh(const SdfPath &id, HdSceneDelegate *sceneDelegate)
    : _id(id)
    , _sceneDelegate(sceneDelegate)
    {
    }

    bool Has(const TfToken &name) override
    {
        if (name == HdMeshSchemaTokens->topology) {
            return true;
        }
        if (name == HdMeshSchemaTokens->subdivisionTags) {
            return true;
        }
        if (name == HdMeshSchemaTokens->doubleSided) {
            return true;
        }
        if (name == HdMeshSchemaTokens->geomSubsets) {
            return true;
        }
        return false;
    }

    TfTokenVector GetNames() override
    {
        return {
            HdMeshSchemaTokens->topology,
            HdMeshSchemaTokens->subdivisionTags,
            HdMeshSchemaTokens->doubleSided,
            HdMeshSchemaTokens->geomSubsets,
        };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdMeshSchemaTokens->topology) {
            return Hd_DataSourceMeshTopology::New(_GetMeshTopologyStore());
        }

        if (name == HdMeshSchemaTokens->subdivisionTags) {
            PxOsdSubdivTags t = _sceneDelegate->GetSubdivTags(_id);
            return HdSubdivisionTagsSchema::BuildRetained(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    t.GetFaceVaryingInterpolationRule()),
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    t.GetVertexInterpolationRule()),
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    t.GetTriangleSubdivision()),
                HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    t.GetCornerIndices()),
                HdRetainedTypedSampledDataSource<VtFloatArray>::New(
                    t.GetCornerWeights()),
                HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    t.GetCreaseIndices()),
                HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    t.GetCreaseLengths()),
                HdRetainedTypedSampledDataSource<VtFloatArray>::New(
                    t.GetCreaseWeights()));
        }

        if (name == HdMeshSchemaTokens->subdivisionScheme) {
            return  Hd_MeshTopologyStore::SubdivisionSchemeDataSource::New(
                _GetMeshTopologyStore());
        }

        if (name == HdMeshSchemaTokens->doubleSided) {
            return HdRetainedTypedSampledDataSource<bool>::New(
                _sceneDelegate->GetDoubleSided(_id));
        }

        if (name == HdMeshSchemaTokens->geomSubsets) {
            return _BuildGeomSubsets();
        }
        
        return nullptr;
    }

private:
    HdDataSourceBaseHandle _BuildGeomSubsets()
    {
        std::vector<TfToken> names;
        std::vector<HdDataSourceBaseHandle> values;

        const HdGeomSubsets &gs =
            _GetMeshTopologyStore()->Get()->GetGeomSubsets();
        for (const HdGeomSubset &geomSubset : gs) {
            HdDataSourceBaseHandle materialIdDs = 
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                        geomSubset.materialId);
            TfToken t = HdMaterialBindingSchemaTokens->allPurpose;

            HdContainerDataSourceHandle containers[2] = {
                HdGeomSubsetSchema::BuildRetained(
                        HdGeomSubsetSchema::BuildTypeDataSource(
                            HdGeomSubsetSchemaTokens->typeFaceSet),
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            geomSubset.indices)),

                HdRetainedContainerDataSource::New(
                        HdMaterialBindingSchemaTokens->materialBinding,
                        HdMaterialBindingSchema::BuildRetained(
                            1, &t, &materialIdDs))
            };

            names.push_back(TfToken(geomSubset.id.GetText()));
            values.push_back(HdOverlayContainerDataSource::New(
                        2, containers));
        }

        static const TfToken invisibleFacesToken("__invisibleFaces");
        static const TfToken invisiblePointsToken("__invisiblePoints");

        VtIntArray invisibleFaces =
            _GetMeshTopologyStore()->Get()->GetInvisibleFaces();
        if (!invisibleFaces.empty()) {
            HdContainerDataSourceHandle containers[2] = {
                HdGeomSubsetSchema::BuildRetained(
                        HdGeomSubsetSchema::BuildTypeDataSource(
                            HdGeomSubsetSchemaTokens->typeFaceSet),
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            invisibleFaces)),
                HdRetainedContainerDataSource::New(
                    HdVisibilitySchemaTokens->visibility,
                    HdVisibilitySchema::BuildRetained(
                        HdRetainedTypedSampledDataSource<bool>::New(false))),
            };

            names.push_back(invisibleFacesToken);
            values.push_back(HdOverlayContainerDataSource::New(
                        2, containers));
        }

        VtIntArray invisiblePoints =
            _GetMeshTopologyStore()->Get()->GetInvisiblePoints();
        if (!invisiblePoints.empty()) {
            HdContainerDataSourceHandle containers[2] = {
                HdGeomSubsetSchema::BuildRetained(
                    HdGeomSubsetSchema::BuildTypeDataSource(
                        HdGeomSubsetSchemaTokens->typePointSet),
                    HdRetainedTypedSampledDataSource<VtIntArray>::New(
                        invisiblePoints)),
                HdRetainedContainerDataSource::New(
                    HdVisibilitySchemaTokens->visibility,
                    HdVisibilitySchema::BuildRetained(
                        HdRetainedTypedSampledDataSource<bool>::New(false))),
            };

            names.push_back(invisiblePointsToken);
            values.push_back(HdOverlayContainerDataSource::New(
                        2, containers));
        }

        if (names.empty()) {
            return nullptr;
        } else {
            return HdRetainedContainerDataSource::New(
                names.size(), names.data(), values.data());
        }
    }

    Hd_MeshTopologyStoreSharedPtr _GetMeshTopologyStore()
    {
        Hd_MeshTopologyStoreSharedPtr mts =
            std::atomic_load(&_meshTopologyStore);
        if (mts) {
            return mts;
        }

        mts = std::make_shared<Hd_MeshTopologyStore>(_id, _sceneDelegate);
        std::atomic_store(&_meshTopologyStore, mts);

        return mts;
    }

    SdfPath _id;
    HdSceneDelegate *_sceneDelegate;
    Hd_MeshTopologyStoreSharedPtr _meshTopologyStore;
};

// ----------------------------------------------------------------------------

using Hd_BasisCurvesTopologyStoreSharedPtr =
    std::shared_ptr<class Hd_BasisCurvesTopologyStore>;
using HdBasisCurvesTopologySharedPtr = std::shared_ptr<HdBasisCurvesTopology>;

class Hd_BasisCurvesTopologyStore
{
public:
    Hd_BasisCurvesTopologyStore(
        const SdfPath &id, HdSceneDelegate *sceneDelegate)
    : _id(id)
    , _sceneDelegate(sceneDelegate)
    {
    }

    HdBasisCurvesTopologySharedPtr Get()
    {
        HdBasisCurvesTopologySharedPtr bct = std::atomic_load(
            &_basisCurvesTopology);
        if (bct) {
            return bct;
        }

        bct = std::make_shared<HdBasisCurvesTopology>(
            _sceneDelegate->GetBasisCurvesTopology(_id));
        std::atomic_store(&_basisCurvesTopology, bct);
        return bct;
    }

    void Invalidate()
    {
        HdBasisCurvesTopologySharedPtr nullbct;
        std::atomic_store(&_basisCurvesTopology, nullbct);
    }

    template <typename T>
    class MemberDataSource : public HdTypedSampledDataSource<T>
    {
    public:
        HD_DECLARE_DATASOURCE_ABSTRACT(MemberDataSource<T>);

        T GetTypedValue(HdSampledDataSource::Time shutterOffset) override = 0;

        VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
        {
            return VtValue(GetTypedValue(shutterOffset));
        }

        bool GetContributingSampleTimesForInterval( 
                HdSampledDataSource::Time startTime, 
                HdSampledDataSource::Time endTime,
                std::vector<HdSampledDataSource::Time> * outSampleTimes)
                    override
        {
            return false;
        }

        MemberDataSource(const Hd_BasisCurvesTopologyStoreSharedPtr &bcts)
        : _bcts(bcts)
        {
        }

    protected:
        Hd_BasisCurvesTopologyStoreSharedPtr _bcts;
    };

    #define DEFINE_BASISCURVES_TOPOLOGY_ACCESSOR_DATASOURCE(N, T, A) \
    class N : public MemberDataSource<T> \
    {                                                                         \
    public:                                                                   \
        HD_DECLARE_DATASOURCE(N);                                             \
                                                                              \
        N(const Hd_BasisCurvesTopologyStoreSharedPtr &bcts)                   \
        : MemberDataSource(bcts) {}                                           \
                                                                              \
        T GetTypedValue(HdSampledDataSource::Time shutterOffset) override     \
        {                                                                     \
            return _bcts->Get()->A();                                         \
        }                                                                     \
    };

    DEFINE_BASISCURVES_TOPOLOGY_ACCESSOR_DATASOURCE(
        CurveTypeDataSource, TfToken, GetCurveType);
    DEFINE_BASISCURVES_TOPOLOGY_ACCESSOR_DATASOURCE(
        CurveWrapDataSource, TfToken, GetCurveWrap);
    DEFINE_BASISCURVES_TOPOLOGY_ACCESSOR_DATASOURCE(
        CurveBasisDataSource, TfToken, GetCurveBasis);
    DEFINE_BASISCURVES_TOPOLOGY_ACCESSOR_DATASOURCE(
        CurveVertexCountsDataSource, VtArray<int>, GetCurveVertexCounts);
    DEFINE_BASISCURVES_TOPOLOGY_ACCESSOR_DATASOURCE(
        CurveIndicesDataSource, VtArray<int>, GetCurveIndices);

private:
    SdfPath _id;
    HdSceneDelegate *_sceneDelegate;
    HdBasisCurvesTopologySharedPtr _basisCurvesTopology;
};



class Hd_DataSourceBasisCurvesTopology : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceBasisCurvesTopology);
    Hd_DataSourceBasisCurvesTopology(const Hd_BasisCurvesTopologyStoreSharedPtr &bcts)
    : _bcts(bcts)
    {
    }

    bool Has(const TfToken &name) override
    {
        if (name == HdBasisCurvesTopologySchemaTokens->curveVertexCounts
                || name == HdBasisCurvesTopologySchemaTokens->curveIndices
                || name == HdBasisCurvesTopologySchemaTokens->basis
                || name == HdBasisCurvesTopologySchemaTokens->type
                || name == HdBasisCurvesTopologySchemaTokens->wrap) {
            return true;
        }
        return false;
    }

    TfTokenVector GetNames() override
    {
        return {
            HdBasisCurvesTopologySchemaTokens->curveVertexCounts,
            HdBasisCurvesTopologySchemaTokens->curveIndices,
            HdBasisCurvesTopologySchemaTokens->basis,
            HdBasisCurvesTopologySchemaTokens->type,
            HdBasisCurvesTopologySchemaTokens->wrap,
        };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdBasisCurvesTopologySchemaTokens->curveVertexCounts) {
            return Hd_BasisCurvesTopologyStore::
                CurveVertexCountsDataSource::New(_bcts);
        }
        if (name == HdBasisCurvesTopologySchemaTokens->curveIndices) {
            return Hd_BasisCurvesTopologyStore::
                CurveIndicesDataSource::New(_bcts);
        }
        if (name == HdBasisCurvesTopologySchemaTokens->basis) {
            return Hd_BasisCurvesTopologyStore::
                CurveBasisDataSource::New(_bcts);
        }
        if (name == HdBasisCurvesTopologySchemaTokens->type) {
            return Hd_BasisCurvesTopologyStore::
                CurveTypeDataSource::New(_bcts);
        }
        if (name == HdBasisCurvesTopologySchemaTokens->wrap) {
            return Hd_BasisCurvesTopologyStore::
                CurveWrapDataSource::New(_bcts);
        }
        return nullptr;
    }

private:
    Hd_BasisCurvesTopologyStoreSharedPtr _bcts;
};

class Hd_DataSourceBasisCurves : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceBasisCurves);

    Hd_DataSourceBasisCurves(const SdfPath &id, HdSceneDelegate *sceneDelegate)
    : _id(id)
    , _sceneDelegate(sceneDelegate)
    {
    }


    bool Has(const TfToken &name) override
    {
        if (name == HdBasisCurvesSchemaTokens->topology) {
            return true;
        }

        if (name == HdBasisCurvesSchemaTokens->geomSubsets) {
            return true;
        }

        return false;
    }

    TfTokenVector GetNames() override
    {
        return {
            HdBasisCurvesSchemaTokens->topology,
            HdBasisCurvesSchemaTokens->geomSubsets,
            
        };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdBasisCurvesSchemaTokens->topology) {
            return Hd_DataSourceBasisCurvesTopology::New(
                _GetBasisCurvesTopologyStore());
        }

        if (name == HdBasisCurvesSchemaTokens->geomSubsets) {
            return _BuildGeomSubsets();
        }

        return nullptr;
    }
private:

    HdDataSourceBaseHandle _BuildGeomSubsets()
    {
       // Build the geom subsets datasource.
        static const TfToken invisiblePointsToken("__invisiblePoints");
        static const TfToken invisibleCurvesToken("__invisibleCurves");

        std::vector<TfToken> names;
        std::vector<HdDataSourceBaseHandle> values;

        VtIntArray invisibleCurves =
            _GetBasisCurvesTopologyStore()->Get()->GetInvisibleCurves();

        if (!invisibleCurves.empty()) {
            HdContainerDataSourceHandle containers[2] = {
                HdGeomSubsetSchema::BuildRetained(
                        HdGeomSubsetSchema::BuildTypeDataSource(
                            HdGeomSubsetSchemaTokens->typeCurveSet),
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            invisibleCurves)),
                HdRetainedContainerDataSource::New(
                        HdVisibilitySchemaTokens->visibility,
                        HdVisibilitySchema::BuildRetained(
                            HdRetainedTypedSampledDataSource<bool>::New(false))),
            };

            names.push_back(invisibleCurvesToken);
            values.push_back(HdOverlayContainerDataSource::New(
                        2, containers));
        }

        VtIntArray invisiblePoints = 
            _GetBasisCurvesTopologyStore()->Get()->GetInvisiblePoints();

        if (!invisiblePoints.empty()) {
            HdContainerDataSourceHandle containers[2] = {
                HdGeomSubsetSchema::BuildRetained(
                        HdGeomSubsetSchema::BuildTypeDataSource(
                            HdGeomSubsetSchemaTokens->typePointSet),
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            invisiblePoints)),
                HdRetainedContainerDataSource::New(
                        HdVisibilitySchemaTokens->visibility,
                        HdVisibilitySchema::BuildRetained(
                            HdRetainedTypedSampledDataSource<bool>::New(false))),
            };

            names.push_back(invisiblePointsToken);
            values.push_back(HdOverlayContainerDataSource::New(
                        2, containers));
        }

        if (names.empty()) {
            return nullptr;
        } else {
            return HdRetainedContainerDataSource::New(
                names.size(), names.data(), values.data());
        }
    }


    Hd_BasisCurvesTopologyStoreSharedPtr _GetBasisCurvesTopologyStore()
    {
        Hd_BasisCurvesTopologyStoreSharedPtr bcts =
            std::atomic_load(&_basisCurvesTopologyStore);
        if (bcts) {
            return bcts;
        }

        bcts =
            std::make_shared<Hd_BasisCurvesTopologyStore>(_id, _sceneDelegate);
        std::atomic_store(&_basisCurvesTopologyStore, bcts);

        return bcts;
    }

    SdfPath _id;
    HdSceneDelegate *_sceneDelegate;
    Hd_BasisCurvesTopologyStoreSharedPtr _basisCurvesTopologyStore;

};

// ----------------------------------------------------------------------------

template <typename T>
class Hd_TypedDataSourceLegacyCameraParamValue : public HdTypedSampledDataSource<T>
{
public:
    HD_DECLARE_DATASOURCE(Hd_TypedDataSourceLegacyCameraParamValue<T>);

    Hd_TypedDataSourceLegacyCameraParamValue(
        const SdfPath &id,
        const TfToken &key,
        HdSceneDelegate *sceneDelegate)
    : _id(id)
    , _key(key)
    , _sceneDelegate(sceneDelegate)
    {
        TF_VERIFY(_sceneDelegate);
    }

    bool GetContributingSampleTimesForInterval( 
        HdSampledDataSource::Time startTime, 
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> * outSampleTimes)  override
    {
        return Hd_DataSourceLegacyPrimvarValue::New(
            _key, _id, _sceneDelegate)->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
    }

    T GetTypedValue(HdSampledDataSource::Time shutterOffset) override
    {
        VtValue v;
        if (shutterOffset == 0.0f) {
            v = _sceneDelegate->GetCameraParamValue(_id, _key);
        } else {
            v = Hd_DataSourceLegacyPrimvarValue::New(
                _key, _id, _sceneDelegate)->GetValue(shutterOffset);
        }

        if (v.IsHolding<T>()) {
            return v.UncheckedGet<T>();
        }

        return T();
    }

    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        if (shutterOffset == 0.0f) {
            return _sceneDelegate->GetCameraParamValue(_id, _key);
        }

        return VtValue(GetTypedValue(shutterOffset));
    }

private:
    SdfPath _id;
    TfToken _key;
    HdSceneDelegate *_sceneDelegate;
};


class Hd_DataSourceLegacyCameraParamValue : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceLegacyCameraParamValue);

    Hd_DataSourceLegacyCameraParamValue(
        const SdfPath &id,
        const TfToken &key,
        HdSceneDelegate *sceneDelegate)
    : _id(id)
    , _key(key)
    , _sceneDelegate(sceneDelegate)
    {
        TF_VERIFY(_sceneDelegate);
    }

    bool GetContributingSampleTimesForInterval( 
        HdSampledDataSource::Time startTime, 
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> * outSampleTimes)  override
    {
        return Hd_DataSourceLegacyPrimvarValue::New(
            _key, _id, _sceneDelegate)->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
    }

    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        if (shutterOffset == 0.0f) {
            return _sceneDelegate->GetCameraParamValue(_id, _key);
        }

        return Hd_DataSourceLegacyPrimvarValue::New(
                _key, _id, _sceneDelegate)->GetValue(shutterOffset);
    }

private:
    SdfPath _id;
    TfToken _key;
    HdSceneDelegate *_sceneDelegate;
};


class Hd_DataSourceCamera : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceCamera);

    Hd_DataSourceCamera(const SdfPath &id, HdSceneDelegate *sceneDelegate)
    : _id(id)
    , _sceneDelegate(sceneDelegate)
    {
        TF_VERIFY(_sceneDelegate);
    }

    bool Has(const TfToken &name) override
    {
        VtValue v = _sceneDelegate->GetCameraParamValue(_id, name);
        return !v.IsEmpty();
    }

    TfTokenVector GetNames() override
    {
        // HdSceneDelegate has no API for advertising what keys it will provide
        // an answer for in response to HdSceneDelegate::GetCameraParamValue.
        // Because HdContainerDataSource does (with this method), we take our
        // best guess by answering with the common ones defined by
        // HdCameraSchema.

        TfTokenVector results;
        results.push_back(HdCameraSchemaTokens->projection);
        results.push_back(HdCameraSchemaTokens->horizontalAperture);
        results.push_back(HdCameraSchemaTokens->verticalAperture);
        results.push_back(HdCameraSchemaTokens->horizontalApertureOffset);
        results.push_back(HdCameraSchemaTokens->verticalApertureOffset);
        results.push_back(HdCameraSchemaTokens->focalLength);
        results.push_back(HdCameraSchemaTokens->clippingRange);

        return results;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        // Smooth out some incompatibilities between scene delegate and
        // datasource schemas...
        if (name == HdCameraSchemaTokens->projection) {
            VtValue v = _sceneDelegate->GetCameraParamValue(_id, name);
        
            HdCamera::Projection proj = HdCamera::Perspective;
            if (v.IsHolding<HdCamera::Projection>()) {
                proj = v.UncheckedGet<HdCamera::Projection>();
            }
            return HdRetainedTypedSampledDataSource<TfToken>::New(
                proj == HdCamera::Perspective ?
                HdCameraSchemaTokens->perspective :
                HdCameraSchemaTokens->orthographic);
        } else if (name == HdCameraSchemaTokens->clippingRange) {
            VtValue v = _sceneDelegate->GetCameraParamValue(_id, name);
        
            GfRange1f range;
            if (v.IsHolding<GfRange1f>()) {
                range = v.UncheckedGet<GfRange1f>();
            }
            return HdRetainedTypedSampledDataSource<GfVec2f>::New(
                GfVec2f(range.GetMin(), range.GetMax()));
        } else if (name == HdCameraTokens->windowPolicy) {
            VtValue v = _sceneDelegate->GetCameraParamValue(_id, name);
        
            // XXX: this should probably be in the schema, and a token...
            CameraUtilConformWindowPolicy wp = CameraUtilDontConform;
            if (v.IsHolding<CameraUtilConformWindowPolicy>()) {
                wp = v.UncheckedGet<CameraUtilConformWindowPolicy>();
            }
            return HdRetainedTypedSampledDataSource<
                        CameraUtilConformWindowPolicy>::New(wp);
        } else if (name == HdCameraTokens->clipPlanes) {
            VtValue v = _sceneDelegate->GetCameraParamValue(_id, name);
        
            // XXX: this should probably be in the schema, and a vec4f array.
            std::vector<GfVec4d> cp;
            if (v.IsHolding<std::vector<GfVec4d>>()) {
                cp = v.UncheckedGet<std::vector<GfVec4d>>();
            }
            return HdRetainedTypedSampledDataSource<std::vector<GfVec4d>>::New(
                        cp);
        } else if (std::find(HdCameraSchemaTokens->allTokens.begin(),
                HdCameraSchemaTokens->allTokens.end(), name)
                    != HdCameraSchemaTokens->allTokens.end()) {
            // all remaining HdCameraSchema members are floats and should
            // be returned as a typed data source for schema conformance.
            return Hd_TypedDataSourceLegacyCameraParamValue<float>::New(
                _id, name, _sceneDelegate);
        } else {
            return Hd_DataSourceLegacyCameraParamValue::New(
                _id, name, _sceneDelegate);
        }
    }

private:
    SdfPath _id;
    HdSceneDelegate *_sceneDelegate;
};

// ----------------------------------------------------------------------------

class Hd_DataSourceLight : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceLight);

    Hd_DataSourceLight(const SdfPath &id, HdSceneDelegate *sceneDelegate)
    : _id(id)
    , _sceneDelegate(sceneDelegate)
    {
    }

    bool Has(const TfToken &name) override
    {
        VtValue v;
        if (_UseGet(name)) {
            v = _sceneDelegate->Get(_id, name);
        } else {
            v = _sceneDelegate->GetLightParamValue(_id, name);
        }
        return !v.IsEmpty();
    }

    TfTokenVector GetNames() override
    {
        // XXX: return the schema tokens when we have them.
        //      for now, return the values which are non-material-based. 
        TfTokenVector result = {
            HdTokens->filters,
            HdTokens->lightLink,
            HdTokens->shadowLink,
            HdTokens->lightFilterLink,
        };
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        VtValue v;
        if (_UseGet(name)) {
            v = _sceneDelegate->Get(_id, name);
        } else {
            v = _sceneDelegate->GetLightParamValue(_id, name);
        }

        // XXX: The simpleLight params all have problematic types. 'params' is
        // GlfSimpleLight, and 'shadowParams' is HdxShadowParams, neither of
        // which we can link to from here.  'shadowCollection' is an
        // HdRprimCollection, which we could technically add as a case to
        // HdCreateTypedRetaiendDataSource; but for now we're just passing all
        // of the simpleLight params through as retained VtValues.
        if (name == HdLightTokens->params) {
            return HdRetainedSampledDataSource::New(v);
        } else if (name == HdLightTokens->shadowParams) {
            return HdRetainedSampledDataSource::New(v);
        } else if (name == HdLightTokens->shadowCollection) {
            return HdRetainedSampledDataSource::New(v);
        } else {
            return HdCreateTypedRetainedDataSource(v);
        }
    }

private:
    SdfPath _id;
    HdSceneDelegate *_sceneDelegate;

    bool _UseGet(const TfToken &name) const {
        // Certain legacy attribute names use Get() instead of
        // GetLightParamValue(), which was the more modern implementation added
        // along with the UsdLux imaging adapter implementation

        if (name == HdLightTokens->params ||
            name == HdLightTokens->shadowParams ||
            name == HdLightTokens->shadowCollection) {
            return true;
        }

        return false;
    }
};

// ----------------------------------------------------------------------------

class Hd_DataSourceVolumeField : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceVolumeField);

    Hd_DataSourceVolumeField(const SdfPath &id, HdSceneDelegate *sceneDelegate)
    : _id(id), _sceneDelegate(sceneDelegate)
    {
        TF_VERIFY(_sceneDelegate);
    }

    bool Has(const TfToken &name) override
    {
        VtValue v = _sceneDelegate->Get(_id, name);
        return !v.IsEmpty();
    }

    TfTokenVector GetNames() override
    {
        // We don't actually know? but return the schema tokens.
        TfTokenVector results;
        results.push_back(HdVolumeFieldSchemaTokens->filePath);
        results.push_back(HdVolumeFieldSchemaTokens->fieldName);
        results.push_back(HdVolumeFieldSchemaTokens->fieldIndex);
        results.push_back(HdVolumeFieldSchemaTokens->fieldDataType);
        results.push_back(HdVolumeFieldSchemaTokens->vectorDataRoleHint);

        return results;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        VtValue v = _sceneDelegate->Get(_id, name);
        return HdCreateTypedRetainedDataSource(v);
    }

private:
    SdfPath _id;
    HdSceneDelegate *_sceneDelegate;
};

// ----------------------------------------------------------------------------

class Hd_InstanceCategoriesVectorDataSource : public HdVectorDataSource
{
public: 
    HD_DECLARE_DATASOURCE(Hd_InstanceCategoriesVectorDataSource);

    Hd_InstanceCategoriesVectorDataSource(
            const SdfPath &id, HdSceneDelegate *sceneDelegate)
    : _id(id), _sceneDelegate(sceneDelegate), _checked(false)
    {
        TF_VERIFY(_sceneDelegate);
    }

    size_t GetNumElements() override
    {
        if (!_checked) {
            _FillArray();
        }
        return _values.size();
    }

    HdDataSourceBaseHandle GetElement(size_t element) override
    {
        if (!_checked) {
            _FillArray();
        }
        if (element < _values.size()) {
            return _values[element];
        }
        return nullptr;
    }

private:
    void _FillArray()
    {
        // NOTE: in emulation, multiple threads are reading from the scene
        // index, but only one thread is reading from any specific prim at a
        // time, so we don't need to worry about concurrent access.
        if (_checked) {
            return;
        }
        std::vector<VtArray<TfToken>> values = 
            _sceneDelegate->GetInstanceCategories(_id);
        if (!values.empty()) {
            _values.reserve(values.size());
            for (const VtArray<TfToken> &value: values) {
                // TODO, deduplicate by address or value
                _values.push_back(HdCategoriesSchema::BuildRetained(
                    value.size(), const_cast<TfToken *>(value.data()),
                        0, nullptr));
            }
        }
        _checked = true;
    }

    SdfPath _id;
    HdSceneDelegate *_sceneDelegate;

    std::atomic_bool _checked;
    std::vector<HdDataSourceBaseHandle> _values;
};

// ----------------------------------------------------------------------------

class Hd_InstancerTopologyDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_InstancerTopologyDataSource);

    Hd_InstancerTopologyDataSource(
        const SdfPath &id, HdSceneDelegate *sceneDelegate)
    : _id(id), _sceneDelegate(sceneDelegate)
    {
        TF_VERIFY(_sceneDelegate);
        SdfPathVector protos = _sceneDelegate->GetInstancerPrototypes(_id);
        _protos.assign(protos.begin(), protos.end());
    }

    bool Has(const TfToken &name) override
    {
        if (name == HdInstancerTopologySchemaTokens->prototypes ||
            name == HdInstancerTopologySchemaTokens->instanceIndices) {
            return true;
        }
        return false;
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector results;
        results.push_back(HdInstancerTopologySchemaTokens->prototypes);
        results.push_back(HdInstancerTopologySchemaTokens->instanceIndices);
        return results;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdInstancerTopologySchemaTokens->prototypes) {
            return HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
                    _protos);
        } else if (name == HdInstancerTopologySchemaTokens->instanceIndices) {
            return Hd_InstanceIndicesDataSource::New(
                    _id, _sceneDelegate, _protos);
        } else {
            return nullptr;
        }
    }

private:
    class Hd_InstanceIndicesDataSource : public HdVectorDataSource
    {
    public:
        HD_DECLARE_DATASOURCE(Hd_InstanceIndicesDataSource);

        Hd_InstanceIndicesDataSource(
            const SdfPath &id, HdSceneDelegate *sceneDelegate,
            const VtArray<SdfPath> protos)
            : _id(id), _sceneDelegate(sceneDelegate), _protos(protos)
        {
            TF_VERIFY(_sceneDelegate);
        }

        size_t GetNumElements() override
        {
            return _protos.size();
        }

        HdDataSourceBaseHandle GetElement(size_t element) override
        {
            if (element >= _protos.size()) {
                return nullptr;
            }

            VtIntArray instanceIndices =
                _sceneDelegate->GetInstanceIndices(_id, _protos[element]);

            return HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    instanceIndices);
        }

    private:
        SdfPath _id;
        HdSceneDelegate *_sceneDelegate;
        const VtArray<SdfPath> _protos;
    };

    SdfPath _id;
    HdSceneDelegate *_sceneDelegate;
    VtArray<SdfPath> _protos;
};

// ----------------------------------------------------------------------------

class Hd_DisplayStyleDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DisplayStyleDataSource);

    Hd_DisplayStyleDataSource(
        HdSceneDelegate *sceneDelegate, const SdfPath &id)
    : _sceneDelegate(sceneDelegate), _id(id), _displayStyleRead(false)
    {
        TF_VERIFY(_sceneDelegate);
    }

    bool Has(const TfToken &name) override
    {
        if (name == HdLegacyDisplayStyleSchemaTokens->refineLevel ||
            name == HdLegacyDisplayStyleSchemaTokens->flatShadingEnabled ||
            name == HdLegacyDisplayStyleSchemaTokens->displacementEnabled ||
            name == HdLegacyDisplayStyleSchemaTokens->occludedSelectionShowsThrough ||
            name == HdLegacyDisplayStyleSchemaTokens->pointsShadingEnabled ||
            name == HdLegacyDisplayStyleSchemaTokens->materialIsFinal ||
            name == HdLegacyDisplayStyleSchemaTokens->shadingStyle ||
            name == HdLegacyDisplayStyleSchemaTokens->reprSelector ||
            name == HdLegacyDisplayStyleSchemaTokens->cullStyle) {
            return true;
        }
        return false;
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector results;
        results.push_back(HdLegacyDisplayStyleSchemaTokens->refineLevel);
        results.push_back(HdLegacyDisplayStyleSchemaTokens->flatShadingEnabled);
        results.push_back(HdLegacyDisplayStyleSchemaTokens->displacementEnabled);
        results.push_back(HdLegacyDisplayStyleSchemaTokens->occludedSelectionShowsThrough);
        results.push_back(HdLegacyDisplayStyleSchemaTokens->pointsShadingEnabled);
        results.push_back(HdLegacyDisplayStyleSchemaTokens->materialIsFinal);
        results.push_back(HdLegacyDisplayStyleSchemaTokens->shadingStyle);
        results.push_back(HdLegacyDisplayStyleSchemaTokens->reprSelector);
        results.push_back(HdLegacyDisplayStyleSchemaTokens->cullStyle);
        return results;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdLegacyDisplayStyleSchemaTokens->refineLevel) {
            if (!_displayStyleRead) {
                _displayStyle = _sceneDelegate->GetDisplayStyle(_id);
                _displayStyleRead = true;
            }
            return (_displayStyle.refineLevel != 0)
                ? HdRetainedTypedSampledDataSource<int>::New(
                        _displayStyle.refineLevel)
                : nullptr;
        } else if (name == HdLegacyDisplayStyleSchemaTokens->flatShadingEnabled) {
            if (!_displayStyleRead) {
                _displayStyle = _sceneDelegate->GetDisplayStyle(_id);
                _displayStyleRead = true;
            }
            return HdRetainedTypedSampledDataSource<bool>::New(
                    _displayStyle.flatShadingEnabled);
        } else if (name == HdLegacyDisplayStyleSchemaTokens->displacementEnabled) {
            if (!_displayStyleRead) {
                _displayStyle = _sceneDelegate->GetDisplayStyle(_id);
                _displayStyleRead = true;
            }
            return HdRetainedTypedSampledDataSource<bool>::New(
                    _displayStyle.displacementEnabled);
        } else if (name == HdLegacyDisplayStyleSchemaTokens->occludedSelectionShowsThrough) {
            if (!_displayStyleRead) {
                _displayStyle = _sceneDelegate->GetDisplayStyle(_id);
                _displayStyleRead = true;
            }
            return HdRetainedTypedSampledDataSource<bool>::New(
                    _displayStyle.occludedSelectionShowsThrough);
        } else if (name == HdLegacyDisplayStyleSchemaTokens->pointsShadingEnabled) {
            if (!_displayStyleRead) {
                _displayStyle = _sceneDelegate->GetDisplayStyle(_id);
                _displayStyleRead = true;
            }
            return HdRetainedTypedSampledDataSource<bool>::New(
                    _displayStyle.pointsShadingEnabled);
        } else if (name == HdLegacyDisplayStyleSchemaTokens->materialIsFinal) {
            if (!_displayStyleRead) {
                _displayStyle = _sceneDelegate->GetDisplayStyle(_id);
                _displayStyleRead = true;
            }
            return HdRetainedTypedSampledDataSource<bool>::New(
                    _displayStyle.materialIsFinal);
        } else if (name == HdLegacyDisplayStyleSchemaTokens->shadingStyle) {
            TfToken shadingStyle = _sceneDelegate->GetShadingStyle(_id)
                .GetWithDefault<TfToken>();
            if (shadingStyle.IsEmpty()) {
                return nullptr;
            }
            return HdRetainedTypedSampledDataSource<TfToken>::New(shadingStyle);
        } else if (name == HdLegacyDisplayStyleSchemaTokens->reprSelector) {
            HdReprSelector repr = _sceneDelegate->GetReprSelector(_id);
            HdTokenArrayDataSourceHandle reprSelectorDs = nullptr;
            bool empty = true;
            for (size_t i = 0; i < HdReprSelector::MAX_TOPOLOGY_REPRS; ++i) {
                if (!repr[i].IsEmpty()) {
                    empty = false;
                    break;
                }
            }
            if (!empty) {
                VtArray<TfToken> array(HdReprSelector::MAX_TOPOLOGY_REPRS);
                for (size_t i = 0; i < HdReprSelector::MAX_TOPOLOGY_REPRS; ++i) {
                    array[i] = repr[i];
                }
                reprSelectorDs =
                    HdRetainedTypedSampledDataSource<VtArray<TfToken>>::New(
                            array);
            }
            return reprSelectorDs;
        } else if (name == HdLegacyDisplayStyleSchemaTokens->cullStyle) {
            HdCullStyle cullStyle = _sceneDelegate->GetCullStyle(_id);
            if (cullStyle == HdCullStyleDontCare) {
                return nullptr;
            }
            TfToken cullStyleToken;
            switch(cullStyle) {
                case HdCullStyleNothing:
                    cullStyleToken = HdCullStyleTokens->nothing;
                    break;
                case HdCullStyleBack:
                    cullStyleToken = HdCullStyleTokens->back;
                    break;
                case HdCullStyleFront:
                    cullStyleToken = HdCullStyleTokens->front;
                    break;
                case HdCullStyleBackUnlessDoubleSided:
                    cullStyleToken = HdCullStyleTokens->backUnlessDoubleSided;
                    break;
                case HdCullStyleFrontUnlessDoubleSided:
                    cullStyleToken = HdCullStyleTokens->frontUnlessDoubleSided;
                    break;
                default:
                    cullStyleToken = HdCullStyleTokens->dontCare;
                    break;
            }
            return HdRetainedTypedSampledDataSource<TfToken>::New(cullStyleToken);
        } else {
            return nullptr;
        }
    }

private:
    HdSceneDelegate *_sceneDelegate;
    SdfPath _id;

    HdDisplayStyle _displayStyle;
    bool _displayStyleRead;
};

// ----------------------------------------------------------------------------

class Hd_GenericGetSampledDataSource : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_GenericGetSampledDataSource);

    Hd_GenericGetSampledDataSource(
        HdSceneDelegate *sceneDelegate, const SdfPath &id, const TfToken &key)
    : _sceneDelegate(sceneDelegate)
    , _id(id)
    , _key(key)
    {}

    VtValue GetValue(Time shutterOffset) override
    {
        return _sceneDelegate->Get(_id, _key);
    }

    // returning false indicates constant value for any time
    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time> *outSampleTimes) override
    {
        return false;
    }

private:
    HdSceneDelegate *_sceneDelegate;
    SdfPath _id;
    TfToken _key;
};

// ----------------------------------------------------------------------------

// Duplicated here because they are currently only defined in hdSt -- on which
// we cannot depend on but must be able to emulate
TF_DEFINE_PRIVATE_TOKENS(
    _drawTargetTokens,
    (camera)
    (collection)
    (drawTargetSet)
    (enable)
    (resolution)
    (aovBindings) 
    (depthPriority)
);


class Hd_LegacyDrawTargetContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_LegacyDrawTargetContainerDataSource);

    Hd_LegacyDrawTargetContainerDataSource(
        HdSceneDelegate *sceneDelegate, const SdfPath &id)
    : _sceneDelegate(sceneDelegate), _id(id) {}

    bool Has(const TfToken &name) override
    {
        return std::find(_drawTargetTokens->allTokens.begin(),
            _drawTargetTokens->allTokens.end(), name)
                != _drawTargetTokens->allTokens.end();
    }

    TfTokenVector GetNames() override
    {
        return _drawTargetTokens->allTokens;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        return HdSampledDataSourceHandle(
            Hd_GenericGetSampledDataSource::New(_sceneDelegate, _id, name));
    }
private:
    HdSceneDelegate *_sceneDelegate;
    SdfPath _id;
};

// ----------------------------------------------------------------------------

class Hd_DataSourceLegacyExtComputationPrimvarsContainer
    : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceLegacyExtComputationPrimvarsContainer);

    Hd_DataSourceLegacyExtComputationPrimvarsContainer(
        const SdfPath &primId,
        HdSceneDelegate *sceneDelegate)
    : _primId(primId)
    , _sceneDelegate(sceneDelegate)
    {
        TF_VERIFY(_sceneDelegate);
    }

    void AddDesc(const TfToken &name, const TfToken &interpolation,
        const TfToken &role, const SdfPath &sourceComputation,
        const TfToken &sourceComputationOutputName,
        const HdTupleType &valueType)
    {
        _entries[name] = {interpolation, role, sourceComputation,
                          sourceComputationOutputName, valueType};
    }

    bool Has(const TfToken &name) override
    {
        return _entries.find(name) != _entries.end();
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector result;
        result.reserve(_entries.size());
        for (const auto &pair : _entries) {
            result.push_back(pair.first);
        }
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        _EntryMap::const_iterator it = _entries.find(name);
        if (it == _entries.end()) {
            return nullptr;
        }

        return HdExtComputationPrimvarSchema::BuildRetained(
            HdExtComputationPrimvarSchema::BuildInterpolationDataSource(
                it->second.interpolation),
            HdExtComputationPrimvarSchema::BuildRoleDataSource(
                it->second.role),
            HdRetainedTypedSampledDataSource<SdfPath>::New(
                it->second.sourceComputation),
            HdRetainedTypedSampledDataSource<TfToken>::New(
                it->second.sourceComputationOutputName),
            HdRetainedTypedSampledDataSource<HdTupleType>::New(
                it->second.valueType));
    }

private:
    struct _Entry
    {
        TfToken interpolation;
        TfToken role;
        SdfPath sourceComputation;
        TfToken sourceComputationOutputName;
        HdTupleType valueType;
    };

    using _EntryMap =  TfDenseHashMap<TfToken, _Entry,
            TfToken::HashFunctor, std::equal_to<TfToken>, 32>;

    _EntryMap _entries;
    SdfPath _primId;
    HdSceneDelegate *_sceneDelegate;
};

HD_DECLARE_DATASOURCE_HANDLES(
    Hd_DataSourceLegacyExtComputationPrimvarsContainer);

// ----------------------------------------------------------------------------

class Hd_DataSourceLegacyExtComputationInput : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceLegacyExtComputationInput);

    Hd_DataSourceLegacyExtComputationInput(
        const TfToken &name,
        const SdfPath &id,
        HdSceneDelegate *sceneDelegate)
    : _name(name), _id(id), _sceneDelegate(sceneDelegate)
    {
        TF_VERIFY(_sceneDelegate);
    }

    VtValue GetValue(Time shutterOffset) override
    {
        if (shutterOffset == 0) {
            return _sceneDelegate->GetExtComputationInput(_id, _name);
        } else {
            _GetTimeSamples();
            return _timeSamples.Resample(shutterOffset);
        }
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time> *outSampleTimes) override
    {
        _GetTimeSamples();

        // XXX: Start and end times come from the sene delegate, so we can't
        // get samples outside of those provided. However, we can clamp
        // returned samples to be in the right range.
        if (outSampleTimes != nullptr) {
            for (const float &t : _timeSamples.times) {
                if (t >= startTime && t <= endTime) {
                    outSampleTimes->push_back(t);
                }
            }
        }

        return true;
    }

private:
    void _GetTimeSamples()
    {
        if (_timeSamples.count == 0) {
            _sceneDelegate->SampleExtComputationInput(
                    _id, _name, &_timeSamples);
        }
    }

    TfToken _name;
    SdfPath _id;
    HdTimeSampleArray<VtValue, 1> _timeSamples;
    HdSceneDelegate *_sceneDelegate;
};

// ----------------------------------------------------------------------------

class Hd_DataSourceLegacyExtComputationInputValues
    : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceLegacyExtComputationInputValues);

    Hd_DataSourceLegacyExtComputationInputValues(
            const SdfPath &id, HdSceneDelegate *sceneDelegate)
    : _id(id), _sceneDelegate(sceneDelegate) {}

    bool Has(const TfToken &name) override
    {
        TfTokenVector names = GetNames();
        return std::find(names.begin(), names.end(), name) != names.end();
    }

    TfTokenVector GetNames() override
    {
        return _sceneDelegate->GetExtComputationSceneInputNames(_id);
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        return Hd_DataSourceLegacyExtComputationInput::New(
            name, _id, _sceneDelegate);
    }

private:
    SdfPath _id;
    HdSceneDelegate *_sceneDelegate;
};

// ----------------------------------------------------------------------------

class Hd_DataSourceLegacyExtComputation : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceLegacyExtComputation);

    Hd_DataSourceLegacyExtComputation(
            const SdfPath &id, HdSceneDelegate *sceneDelegate)
    : _id(id), _sceneDelegate(sceneDelegate) {}

    bool Has(const TfToken &name) override
    {
        if (name == HdExtComputationSchemaTokens->inputValues ||
            name == HdExtComputationSchemaTokens->inputComputations ||
            name == HdExtComputationSchemaTokens->outputs ||
            name == HdExtComputationSchemaTokens->glslKernel ||
            name == HdExtComputationSchemaTokens->cpuCallback) {
            return true;
        }
        return false;
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector result;
        result.push_back(HdExtComputationSchemaTokens->inputValues);
        result.push_back(HdExtComputationSchemaTokens->inputComputations);
        result.push_back(HdExtComputationSchemaTokens->outputs);
        result.push_back(HdExtComputationSchemaTokens->glslKernel);
        result.push_back(HdExtComputationSchemaTokens->cpuCallback);
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdExtComputationSchemaTokens->inputValues) {
            return Hd_DataSourceLegacyExtComputationInputValues::New(
                    _id, _sceneDelegate);
        } else if (name == HdExtComputationSchemaTokens->inputComputations) {
            HdExtComputationInputDescriptorVector descs =
                _sceneDelegate->GetExtComputationInputDescriptors(_id);
            std::vector<HdDataSourceBaseHandle> out;
            out.reserve(descs.size());
            for (const auto& desc : descs) {
                out.push_back(
                    HdExtComputationInputComputationSchema::BuildRetained(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            desc.name),
                        HdRetainedTypedSampledDataSource<SdfPath>::New(
                            desc.sourceComputationId),
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            desc.sourceComputationOutputName)));
            }
            return HdRetainedSmallVectorDataSource::New(out.size(), out.data());
        } else if (name == HdExtComputationSchemaTokens->outputs) {
            HdExtComputationOutputDescriptorVector descs =
                _sceneDelegate->GetExtComputationOutputDescriptors(_id);
            std::vector<HdDataSourceBaseHandle> out;
            out.reserve(descs.size());
            for (const auto& desc : descs) {
                out.push_back(HdExtComputationOutputSchema::BuildRetained(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        desc.name),
                    HdRetainedTypedSampledDataSource<HdTupleType>::New(
                        desc.valueType)));
            }
            return HdRetainedSmallVectorDataSource::New(out.size(), out.data());
        } else if (name == HdExtComputationSchemaTokens->glslKernel) {
            std::string kernel = _sceneDelegate->GetExtComputationKernel(_id);
            return HdRetainedTypedSampledDataSource<std::string>::New(kernel);
        } else if (name == HdExtComputationSchemaTokens->cpuCallback) {
            return HdExtComputationCallbackDataSource::New(
                _id, _sceneDelegate);
        } else if (name == HdExtComputationSchemaTokens->dispatchCount) {
            size_t dispatchCount = 0;
            VtValue vDispatch = _sceneDelegate->GetExtComputationInput(
                _id, HdTokens->dispatchCount);
            if (vDispatch.IsHolding<size_t>()) {
                dispatchCount = vDispatch.UncheckedGet<size_t>();
            }
            return HdRetainedTypedSampledDataSource<size_t>::New(dispatchCount);
        } else if (name == HdExtComputationSchemaTokens->elementCount) {
            size_t elementCount = 0;
            VtValue vElement = _sceneDelegate->GetExtComputationInput(
                _id, HdTokens->elementCount);
            if (vElement.IsHolding<size_t>()) {
                elementCount = vElement.UncheckedGet<size_t>();
            }
            return HdRetainedTypedSampledDataSource<size_t>::New(elementCount);
        } else {
            return nullptr;
        }
    }

private:
    SdfPath _id;
    HdSceneDelegate *_sceneDelegate;
};

// ----------------------------------------------------------------------------

class Hd_DataSourceRenderBuffer : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_DataSourceRenderBuffer);

    Hd_DataSourceRenderBuffer(
        HdSceneDelegate *sceneDelegate, const SdfPath &id)
    : _sceneDelegate(sceneDelegate), _id(id)
    {
        TF_VERIFY(_sceneDelegate);
        rb = _sceneDelegate->GetRenderBufferDescriptor(_id);
    }

    bool Has(const TfToken &name) override
    {
        if (name == HdRenderBufferSchemaTokens->dimensions ||
            name == HdRenderBufferSchemaTokens->format ||
            name == HdRenderBufferSchemaTokens->multiSampled) {
            return true;
        }

        VtValue v = _sceneDelegate->Get(_id, name);
        return !v.IsEmpty();
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector v;
        v.push_back(HdRenderBufferSchemaTokens->dimensions);
        v.push_back(HdRenderBufferSchemaTokens->format);
        v.push_back(HdRenderBufferSchemaTokens->multiSampled);
        return v;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdRenderBufferSchemaTokens->dimensions) {
            return HdRetainedTypedSampledDataSource<GfVec3i>::New(
                    rb.dimensions);
        } else if (name == HdRenderBufferSchemaTokens->format) {
            return HdRetainedTypedSampledDataSource<HdFormat>::New(
                    rb.format);
        } else if (name == HdRenderBufferSchemaTokens->multiSampled) {
            return HdRetainedTypedSampledDataSource<bool>::New(
                    rb.multiSampled);
        } else {
            return HdSampledDataSourceHandle(
                Hd_GenericGetSampledDataSource::New(_sceneDelegate, _id, name));
        }
    }

private:
    HdSceneDelegate *_sceneDelegate;
    SdfPath _id;
    HdRenderBufferDescriptor rb;
};

// ----------------------------------------------------------------------------

TfToken _InterpolationAsToken(HdInterpolation interpolation)
{
    switch (interpolation) {
    case HdInterpolationConstant:
        return HdPrimvarSchemaTokens->constant;
    case HdInterpolationUniform:
        return HdPrimvarSchemaTokens->uniform;
    case HdInterpolationVarying:
        return HdPrimvarSchemaTokens->varying;
    case HdInterpolationVertex:
        return HdPrimvarSchemaTokens->vertex;
    case HdInterpolationFaceVarying:
        return HdPrimvarSchemaTokens->faceVarying;
    case HdInterpolationInstance:
        return HdPrimvarSchemaTokens->instance;

    default:
        return HdPrimvarSchemaTokens->constant;
    }
}

} //anonymous namespace

// ----------------------------------------------------------------------------

void
HdExtComputationCallbackDataSource::Invoke(HdExtComputationContext *context)
{
    _sceneDelegate->InvokeExtComputation(_id, context);
}

// ----------------------------------------------------------------------------

HdDataSourceLegacyPrim::HdDataSourceLegacyPrim(
    SdfPath id, 
    TfToken type, 
    HdSceneDelegate *sceneDelegate)
: _id(id),
  _type(type),
  _sceneDelegate(sceneDelegate),
  _primvarsBuilt(false),
  _extComputationPrimvarsBuilt(false)
{
    TF_VERIFY(_sceneDelegate);
}

void
HdDataSourceLegacyPrim::PrimDirtied(const HdDataSourceLocatorSet &locators)
{
    if (locators.Intersects(HdPrimvarsSchema::GetDefaultLocator())) {
        _primvarsBuilt.store(false);
        _extComputationPrimvarsBuilt = false;
        HdContainerDataSourceHandle null(nullptr);
        HdContainerDataSource::AtomicStore(_primvars, null);
        _extComputationPrimvars.reset();
    }

    if (locators.Intersects(HdInstancerTopologySchema::GetDefaultLocator())) {
        HdContainerDataSourceHandle null(nullptr);
        HdContainerDataSource::AtomicStore(_instancerTopology, null);
    }
}

const HdDataSourceLocatorSet&
HdDataSourceLegacyPrim::GetCachedLocators()
{
    static HdDataSourceLocatorSet locators = {
        HdPrimvarsSchema::GetDefaultLocator(),
        HdInstancerTopologySchema::GetDefaultLocator(),
    };

    return locators;
}

static
bool
_IsTypeLightLike(const TfToken &type)
{
    // Things for which HdSceneDelegate::GetLightParamValue is meaningful
    // for emulation
    if (HdPrimTypeIsLight(type) || type == HdPrimTypeTokens->lightFilter) {
        return true;
    }

    return false;
}

bool 
HdDataSourceLegacyPrim::Has(const TfToken &name)
{
    if (_type == HdPrimTypeTokens->mesh) {
        if (name == HdMeshSchemaTokens->mesh) {
            return true;
        }
    }

    if (_type == HdPrimTypeTokens->basisCurves) {
        if (name == HdPrimTypeTokens->basisCurves) {
            return true;
        }
    }

    if (HdPrimTypeIsGprim(_type)) {
        if (name == HdPrimvarsSchemaTokens->primvars ||
            name == HdExtComputationPrimvarsSchemaTokens->extComputationPrimvars ||
            name == HdMaterialBindingSchemaTokens->materialBinding ||
            name == HdLegacyDisplayStyleSchemaTokens->displayStyle || 
            name == HdCoordSysBindingSchemaTokens->coordSysBinding ||
            name == HdPurposeSchemaTokens->purpose ||
            name == HdVisibilitySchemaTokens->visibility ||
            name == HdInstancedBySchemaTokens->instancedBy ||
            name == HdDataSharingSchemaTokens->dataSharing ||
            name == HdCategoriesSchemaTokens->categories ||
            name == HdXformSchemaTokens->xform ||
            name == HdExtentSchemaTokens->extent) {
            return true;
        }
    }
    
    if (HdPrimTypeIsLight(_type)) {
        if (name == HdMaterialSchemaTokens->material ||
            name == HdXformSchemaTokens->xform ||
            name == HdPrimvarsSchemaTokens->primvars) {
            return true;
        }
    }

    if (_IsTypeLightLike(_type)) {
        if (name == HdLightSchemaTokens->light ||
            name == HdMaterialSchemaTokens->material) {
            return true;
        }
    }

    if (_type == HdPrimTypeTokens->material) {
        if (name == HdMaterialSchemaTokens->material) {
            return true;
        }
    }

    if (_type == HdPrimTypeTokens->instancer) {
        if (name == HdXformSchemaTokens->xform ||
            name == HdInstancedBySchemaTokens->instancedBy ||
            name == HdDataSharingSchemaTokens->dataSharing ||
            name == HdInstancerTopologySchemaTokens->instancerTopology ||
            name == HdPrimvarsSchemaTokens->primvars ||
            name == HdInstanceCategoriesSchemaTokens->instanceCategories) {
            return true;
        }
    }

    if (_type == HdPrimTypeTokens->camera) {
        if (name == HdCameraSchemaTokens->camera ||
            name == HdXformSchemaTokens->xform) {
            return true;
        }
    }

    if (_type == HdPrimTypeTokens->renderBuffer) {
        if (name == HdRenderBufferSchemaTokens->renderBuffer) {
            return true;
        }
    }

    if (HdLegacyPrimTypeIsVolumeField(_type)) {
        if (name == HdVolumeFieldSchemaTokens->volumeField) {
            return true;
        }
    }

    if (_type == HdPrimTypeTokens->volume) {
        if (name == HdVolumeFieldBindingSchemaTokens->volumeFieldBinding) {
            return true;
        }
    }

    if (_type == HdPrimTypeTokens->extComputation) {
        if (name == HdExtComputationSchemaTokens->extComputation) {
            return true;
        }
    }

    if (_type == HdPrimTypeTokens->coordSys) {
        if (name == HdXformSchemaTokens->xform) {
            return true;
        }
    }

    if (_type == HdPrimTypeTokens->drawTarget) {
        if (name == HdPrimTypeTokens->drawTarget) {
            return true;
        }
    }

    if (_type == _tokens->prmanParams) {
        if (name == _tokens->prmanParams) {
            return true;
        }
    }

    if (name == HdSceneIndexEmulationTokens->sceneDelegate) {
        return true;
    }

    return false;
}

TfTokenVector 
HdDataSourceLegacyPrim::GetNames()
{
    TfTokenVector result;

    if (_type == HdPrimTypeTokens->mesh) {
        result.push_back(HdMeshSchemaTokens->mesh);
    }

    if (_type == HdPrimTypeTokens->basisCurves) {
        result.push_back(HdBasisCurvesSchemaTokens->basisCurves);
    }

    if (HdPrimTypeIsGprim(_type)) {
        result.push_back(HdPrimvarsSchemaTokens->primvars);
        result.push_back(HdExtComputationPrimvarsSchemaTokens->extComputationPrimvars);
        result.push_back(HdMaterialBindingSchemaTokens->materialBinding);
        result.push_back(HdLegacyDisplayStyleSchemaTokens->displayStyle); 
        result.push_back(HdCoordSysBindingSchemaTokens->coordSysBinding);
        result.push_back(HdPurposeSchemaTokens->purpose);
        result.push_back(HdVisibilitySchemaTokens->visibility);
        result.push_back(HdInstancedBySchemaTokens->instancedBy);
        result.push_back(HdDataSharingSchemaTokens->dataSharing);
        result.push_back(HdCategoriesSchemaTokens->categories);
        result.push_back(HdXformSchemaTokens->xform);
        result.push_back(HdExtentSchemaTokens->extent);
    }
    
    if (HdPrimTypeIsLight(_type)) {
        result.push_back(HdMaterialSchemaTokens->material);
        result.push_back(HdXformSchemaTokens->xform);
        result.push_back(HdPrimvarsSchemaTokens->primvars);
    }

    if (_IsTypeLightLike(_type)) {
        result.push_back(HdLightSchemaTokens->light);
        result.push_back(HdMaterialSchemaTokens->material);
    }

    if (_type == HdPrimTypeTokens->material) {
        result.push_back(HdMaterialSchemaTokens->material);
    }

    if (_type == HdPrimTypeTokens->instancer) {
        result.push_back(HdXformSchemaTokens->xform);
        result.push_back(HdInstancedBySchemaTokens->instancedBy);
        result.push_back(HdDataSharingSchemaTokens->dataSharing);
        result.push_back(HdInstancerTopologySchemaTokens->instancerTopology);
        result.push_back(HdPrimvarsSchemaTokens->primvars);
        result.push_back(HdInstanceCategoriesSchemaTokens->instanceCategories);
    }

    if (_type == HdPrimTypeTokens->camera) {
        result.push_back(HdCameraSchemaTokens->camera);
        result.push_back(HdXformSchemaTokens->xform);
    }

    if (_type == HdPrimTypeTokens->renderBuffer) {
        result.push_back(HdRenderBufferSchemaTokens->renderBuffer);
    }

    if (HdLegacyPrimTypeIsVolumeField(_type)) {
        result.push_back(HdVolumeFieldSchemaTokens->volumeField);
    }

    if (_type == HdPrimTypeTokens->volume) {
        result.push_back(HdVolumeFieldBindingSchemaTokens->volumeFieldBinding);
    }

    if (_type == HdPrimTypeTokens->extComputation) {
        result.push_back(HdExtComputationSchemaTokens->extComputation);
    }

    if (_type == HdPrimTypeTokens->coordSys) {
        result.push_back(HdXformSchemaTokens->xform);
    }

    if (_type == HdPrimTypeTokens->drawTarget) {
        result.push_back(HdPrimTypeTokens->drawTarget);
    }

    if (_type == _tokens->prmanParams) {
        result.push_back(_tokens->prmanParams);
    }

    result.push_back(HdSceneIndexEmulationTokens->sceneDelegate);

    return result;
}

static bool
_ConvertHdMaterialNetworkToHdDataSources(
    const HdMaterialNetworkMap &hdNetworkMap,
    HdContainerDataSourceHandle *result)
{
    HD_TRACE_FUNCTION();

    TfTokenVector terminalsNames;
    std::vector<HdDataSourceBaseHandle> terminalsValues;
    std::vector<TfToken> nodeNames;
    std::vector<HdDataSourceBaseHandle> nodeValues;

    for (auto const &iter: hdNetworkMap.map) {
        const TfToken &terminalName = iter.first;
        const HdMaterialNetwork &hdNetwork = iter.second;

        if (hdNetwork.nodes.empty()) {
            continue;
        }

        terminalsNames.push_back(terminalName);

        // Transfer over individual nodes.
        // Note that the same nodes may be shared by multiple terminals.
        // We simply overwrite them here.
        for (const HdMaterialNode &node : hdNetwork.nodes) {
            std::vector<TfToken> paramsNames;
            std::vector<HdDataSourceBaseHandle> paramsValues;

            for (const auto &p : node.parameters) {
                paramsNames.push_back(p.first);
                paramsValues.push_back(
                    HdRetainedTypedSampledDataSource<VtValue>::New(p.second)
                );
            }

            // Accumulate array connections to the same input
            TfDenseHashMap<TfToken,
                TfSmallVector<HdDataSourceBaseHandle, 8>, TfToken::HashFunctor> 
                    connectionsMap;

            TfSmallVector<TfToken, 8> cNames;
            TfSmallVector<HdDataSourceBaseHandle, 8> cValues;

            for (const HdMaterialRelationship &rel : hdNetwork.relationships) {
                if (rel.outputId == node.path) {                    
                    TfToken outputPath = rel.inputId.GetToken(); 
                    TfToken outputName = TfToken(rel.inputName.GetString());

                    HdDataSourceBaseHandle c = 
                        HdMaterialConnectionSchema::BuildRetained(
                            HdRetainedTypedSampledDataSource<TfToken>::New(
                                outputPath),
                            HdRetainedTypedSampledDataSource<TfToken>::New(
                                outputName));

                    connectionsMap[
                        TfToken(rel.outputName.GetString())].push_back(c);
                }
            }

            cNames.reserve(connectionsMap.size());
            cValues.reserve(connectionsMap.size());

            // NOTE: not const because HdRetainedSmallVectorDataSource needs
            //       a non-const HdDataSourceBaseHandle*
            for (auto &entryPair : connectionsMap) {
                cNames.push_back(entryPair.first);
                cValues.push_back(
                    HdRetainedSmallVectorDataSource::New(
                        entryPair.second.size(), entryPair.second.data()));
            }

            nodeNames.push_back(node.path.GetToken());
            nodeValues.push_back(
                HdMaterialNodeSchema::BuildRetained(
                    HdRetainedContainerDataSource::New(
                        paramsNames.size(), 
                        paramsNames.data(),
                        paramsValues.data()),
                    HdRetainedContainerDataSource::New(
                        cNames.size(), 
                        cNames.data(),
                        cValues.data()),
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        node.identifier)));
        }

        terminalsValues.push_back(
            HdMaterialConnectionSchema::BuildRetained(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    hdNetwork.nodes.back().path.GetToken()),
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    terminalsNames.back()))
                );
    }

    HdContainerDataSourceHandle nodesDefaultContext = 
        HdRetainedContainerDataSource::New(
            nodeNames.size(), 
            nodeNames.data(),
            nodeValues.data());

    HdContainerDataSourceHandle terminalsDefaultContext = 
        HdRetainedContainerDataSource::New(
            terminalsNames.size(), 
            terminalsNames.data(),
            terminalsValues.data());

    // Create the material network, potentially one per network selector
    HdDataSourceBaseHandle network = HdMaterialNetworkSchema::BuildRetained(
        nodesDefaultContext,
        terminalsDefaultContext);
        
    TfToken defaultContext = HdMaterialSchemaTokens->universalRenderContext;
    *result = HdMaterialSchema::BuildRetained(
        1, 
        &defaultContext, 
        &network);
    
    return true;
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetPrimvarsDataSource()
{
    if (_primvarsBuilt.load()) {
        return HdContainerDataSource::AtomicLoad(_primvars);
    }

    TRACE_FUNCTION();

    Hd_DataSourceLegacyPrimvarsContainerHandle primvarsDs;

    for (size_t interpolation = HdInterpolationConstant;
        interpolation < HdInterpolationCount; ++interpolation) {

        HdPrimvarDescriptorVector v = _sceneDelegate->GetPrimvarDescriptors(
            _id, 
            (HdInterpolation)interpolation);

        TfToken interpolationToken = _InterpolationAsToken(
            (HdInterpolation)interpolation);

        for (const auto &primvarDesc : v) {
            if (!primvarsDs) {
                primvarsDs = Hd_DataSourceLegacyPrimvarsContainer::New(
                    _id, _sceneDelegate);
            }
            primvarsDs->AddDesc(
                primvarDesc.name, interpolationToken, primvarDesc.role,
                primvarDesc.indexed);
        }
    }

    HdContainerDataSourceHandle ds = primvarsDs;
    HdContainerDataSource::AtomicStore(_primvars, ds);
    _primvarsBuilt.store(true);

    return primvarsDs;
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetExtComputationPrimvarsDataSource()
{
    if (_extComputationPrimvarsBuilt) {
        return _extComputationPrimvars;
    }

    Hd_DataSourceLegacyExtComputationPrimvarsContainerHandle primvarsDs;

    for (size_t interpolation = HdInterpolationConstant;
         interpolation < HdInterpolationCount; ++interpolation) {
        HdExtComputationPrimvarDescriptorVector v =
            _sceneDelegate->GetExtComputationPrimvarDescriptors(
                _id, (HdInterpolation)interpolation);

        TfToken interpolationToken = _InterpolationAsToken(
            (HdInterpolation)interpolation);

        for (const auto &primvarDesc : v) {
            if (!primvarsDs) {
                primvarsDs =
                    Hd_DataSourceLegacyExtComputationPrimvarsContainer::New(
                        _id, _sceneDelegate);
            }
            primvarsDs->AddDesc(
                primvarDesc.name, interpolationToken, primvarDesc.role,
                primvarDesc.sourceComputationId,
                primvarDesc.sourceComputationOutputName,
                primvarDesc.valueType);
        }
    }

    _extComputationPrimvars = primvarsDs;
    _extComputationPrimvarsBuilt = true;
    return _extComputationPrimvars;
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetMaterialBindingDataSource()
{
    SdfPath path = _sceneDelegate->GetMaterialId(_id);
    if (path.IsEmpty()) {
        return nullptr;
    }
    HdDataSourceBaseHandle bindingPath = 
        HdRetainedTypedSampledDataSource<SdfPath>::New(path);

    TfToken t = HdMaterialBindingSchemaTokens->allPurpose;
    HdContainerDataSourceHandle binding = 
        HdMaterialBindingSchema::BuildRetained(1, &t, &bindingPath);
    return binding;
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetXformDataSource()
{
    HdContainerDataSourceHandle t = 
        HdXformSchema::BuildRetained(
            Hd_DataSourceLegacyMatrixValue::New(_type, _id, _sceneDelegate),
            // Mark this transform as fully composed, since scene delegate
            // transforms are always fully composed.
            HdRetainedTypedSampledDataSource<bool>::New(true)
        );
    return t;
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetMaterialDataSource()
{
    VtValue materialContainer = _sceneDelegate->GetMaterialResource(_id);

    if (!materialContainer.IsHolding<HdMaterialNetworkMap>()) {
        return nullptr;
    }

    HdMaterialNetworkMap hdNetworkMap = 
        materialContainer.UncheckedGet<HdMaterialNetworkMap>();
    HdContainerDataSourceHandle materialDS = nullptr;    
    if (!_ConvertHdMaterialNetworkToHdDataSources(
        hdNetworkMap,
        &materialDS) ) {
        return nullptr;
    }
    return materialDS;
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetDisplayStyleDataSource()
{
    return Hd_DisplayStyleDataSource::New(_sceneDelegate, _id);
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetInstancedByDataSource()
{
    SdfPath instancerId = _sceneDelegate->GetInstancerId(_id);
    if (instancerId.IsEmpty()) {
        return nullptr;
    }
    return HdInstancedBySchema::BuildRetained(
        HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
            VtArray<SdfPath>({instancerId})));
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetDataSharingDataSource()
{
    SdfPath sharingId = _sceneDelegate->GetDataSharingId(_id);
    if (sharingId.IsEmpty()) {
        return nullptr;
    }
    return HdDataSharingSchema::BuildRetained(
        HdRetainedTypedSampledDataSource<SdfPath>::New(sharingId));
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetInstancerTopologyDataSource()
{
    TRACE_FUNCTION();

    HdContainerDataSourceHandle instancerTopology =
        HdContainerDataSource::AtomicLoad(_instancerTopology);

    if (instancerTopology) {
        return instancerTopology;
    }

    instancerTopology =
        Hd_InstancerTopologyDataSource::New(_id, _sceneDelegate);

    HdContainerDataSource::AtomicStore(
        _instancerTopology, instancerTopology);

    return instancerTopology;
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetVolumeFieldBindingDataSource()
{
    HdVolumeFieldDescriptorVector volumeFields =
        _sceneDelegate->GetVolumeFieldDescriptors(_id);

    if (volumeFields.empty()) {
        return nullptr;
    }

    std::vector<TfToken> names;
    std::vector<HdDataSourceBaseHandle> paths;
    for (HdVolumeFieldDescriptor const &desc : volumeFields) {
        names.push_back(desc.fieldName);
        paths.push_back(HdRetainedTypedSampledDataSource<SdfPath>::New(
                desc.fieldId));
    }
    return HdVolumeFieldBindingSchema::BuildRetained(
        names.size(), names.data(), paths.data());
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetCoordSysBindingDataSource()
{
    HdIdVectorSharedPtr coordSysBindings =
        _sceneDelegate->GetCoordSysBindings(_id);

    if (coordSysBindings == nullptr || coordSysBindings->empty()) {
        return nullptr;
    }

    std::vector<TfToken> names;
    std::vector<HdDataSourceBaseHandle> paths;
    for (SdfPath const &path : *coordSysBindings) {
        // Note: the scene delegate API just binds prims to unnamed
        // coord sys objects.  These coord sys objects have paths of the
        // form /path/to/object.coordSys:foo, where "foo" is the name the
        // shader gets to access.  We pull these names out to store in the
        // schema.
        names.push_back(TfToken(SdfPath::StripNamespace(path.GetName())));
        paths.push_back(HdRetainedTypedSampledDataSource<SdfPath>::New(
            path));
    }
    return HdCoordSysBindingSchema::BuildRetained(
            names.size(), names.data(), paths.data());
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetVisibilityDataSource()
{
    bool vis = _sceneDelegate->GetVisible(_id);
    if (vis) {
        static const HdContainerDataSourceHandle visOn = 
            HdVisibilitySchema::BuildRetained(
                HdRetainedTypedSampledDataSource<bool>::New(true));
        return visOn;
    } else {
        static const HdContainerDataSourceHandle visOff = 
            HdVisibilitySchema::BuildRetained(
                HdRetainedTypedSampledDataSource<bool>::New(false));
        return visOff;
    }
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetPurposeDataSource()
{
    TfToken purpose = _sceneDelegate->GetRenderTag(_id);
    return HdPurposeSchema::BuildRetained(
        HdRetainedTypedSampledDataSource<TfToken>::New(purpose));
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetExtentDataSource()
{
    GfRange3d extent = _sceneDelegate->GetExtent(_id);
    return HdExtentSchema::BuildRetained(
        HdRetainedTypedSampledDataSource<GfVec3d>::New(extent.GetMin()),
        HdRetainedTypedSampledDataSource<GfVec3d>::New(extent.GetMax()));
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetCategoriesDataSource()
{
    VtArray<TfToken> categories = _sceneDelegate->GetCategories(_id);
    if (!categories.empty()) {
        return HdCategoriesSchema::BuildRetained(
            categories.size(), categories.data(), 0, nullptr);
    }
    return nullptr;
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetInstanceCategoriesDataSource()
{
    return HdInstanceCategoriesSchema::BuildRetained(
        HdVectorDataSourceHandle(
            Hd_InstanceCategoriesVectorDataSource::New(_id, _sceneDelegate))
    );
}

HdDataSourceBaseHandle
HdDataSourceLegacyPrim::_GetPrmanParamsDataSource()
{
    VtValue namesValue = _sceneDelegate->Get(_id, _tokens->prmanParamsNames);
    if (!namesValue.IsHolding<TfTokenVector>()) {
        return nullptr;
    }

    TfTokenVector dictKeys;
    std::vector<HdDataSourceBaseHandle> dicts;

    for (const TfToken &dictName : namesValue.UncheckedGet<TfTokenVector>()) {
        VtValue dictValue = _sceneDelegate->Get(_id, dictName);

        if (!dictValue.IsHolding<std::map<TfToken, VtValue>>()) {
            continue;
        }

        std::map<TfToken, VtValue> dict =
            dictValue.UncheckedGet<std::map<TfToken, VtValue>>();

        if (dict.empty()) {
            continue;
        }

        TfTokenVector valueKeys;
        std::vector<HdDataSourceBaseHandle> values;
        for (const auto &valuePair : dict) {
            valueKeys.push_back(valuePair.first);
            values.push_back(
                HdRetainedSampledDataSource::New(valuePair.second));
        }

        dictKeys.push_back(dictName);
        dicts.push_back(HdRetainedContainerDataSource::New(
            valueKeys.size(), valueKeys.data(), values.data()));
    }


    return HdRetainedContainerDataSource::New(
            dictKeys.size(), dictKeys.data(), dicts.data());
}

HdDataSourceBaseHandle 
HdDataSourceLegacyPrim::Get(const TfToken &name)
{
    if (name == HdMeshSchemaTokens->mesh) {
        if (_type == HdPrimTypeTokens->mesh) {
            return Hd_DataSourceMesh::New(_id, _sceneDelegate);
        }
        
    } else if (name == HdBasisCurvesSchemaTokens->basisCurves) {
        if (_type == HdPrimTypeTokens->basisCurves) {
            return Hd_DataSourceBasisCurves::New(_id, _sceneDelegate);
        }
    } else if (name == HdPrimvarsSchemaTokens->primvars) {
        return _GetPrimvarsDataSource();
    } else if (
        name == HdExtComputationPrimvarsSchemaTokens->extComputationPrimvars) {
        return _GetExtComputationPrimvarsDataSource();
    } else if (name == HdMaterialBindingSchemaTokens->materialBinding) {
        return _GetMaterialBindingDataSource();
    } else if (name == HdXformSchemaTokens->xform) {
       return _GetXformDataSource();
    } else if (name == HdMaterialSchemaTokens->material) {
        return _GetMaterialDataSource();
    } else if (name == HdLegacyDisplayStyleSchemaTokens->displayStyle) {
        return _GetDisplayStyleDataSource();
    } else if (name == HdSceneIndexEmulationTokens->sceneDelegate) {
        return HdRetainedTypedSampledDataSource<HdSceneDelegate*>::New(
            _sceneDelegate);
    } else if (name == HdInstancedBySchemaTokens->instancedBy) {
        return _GetInstancedByDataSource();
    } else if (name == HdDataSharingSchemaTokens->dataSharing) {
        return _GetDataSharingDataSource();
    } else if (name == HdInstancerTopologySchemaTokens->instancerTopology) {
        return _GetInstancerTopologyDataSource();
    } else if (name == HdVolumeFieldBindingSchemaTokens->volumeFieldBinding) {
        return _GetVolumeFieldBindingDataSource();
    } else if (name == HdCoordSysBindingSchemaTokens->coordSysBinding) {
        return _GetCoordSysBindingDataSource();
    } else if (name == HdVisibilitySchemaTokens->visibility) {
        return _GetVisibilityDataSource();
    } else if (name == HdPurposeSchemaTokens->purpose) {
        return _GetPurposeDataSource();
    } else if (name == HdExtentSchemaTokens->extent) {
        return _GetExtentDataSource();
    } else if (name == HdCameraSchemaTokens->camera) {
        return Hd_DataSourceCamera::New(_id, _sceneDelegate);
    } else if (name == HdLightSchemaTokens->light) {
        return Hd_DataSourceLight::New(_id, _sceneDelegate);
    } else if (name == HdCategoriesSchemaTokens->categories) {
        return _GetCategoriesDataSource();
    } else if (name == HdInstanceCategoriesSchemaTokens->instanceCategories) {
        return _GetInstanceCategoriesDataSource();
    } else if (name == HdRenderBufferSchemaTokens->renderBuffer) {
        return Hd_DataSourceRenderBuffer::New(_sceneDelegate, _id);
    } else if (name == HdVolumeFieldSchemaTokens->volumeField) {
        return Hd_DataSourceVolumeField::New(_id, _sceneDelegate);
    } else if (name == HdPrimTypeTokens->drawTarget) {
        return HdContainerDataSourceHandle(
            Hd_LegacyDrawTargetContainerDataSource::New(_sceneDelegate, _id));
    } else if (name == HdExtComputationSchemaTokens->extComputation) {
        return Hd_DataSourceLegacyExtComputation::New(_id, _sceneDelegate);
    } else if (name == _tokens->prmanParams) {
        if (_type == _tokens->prmanParams) {
            return _GetPrmanParamsDataSource();
        }
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
