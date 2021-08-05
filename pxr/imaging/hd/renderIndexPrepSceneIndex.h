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
#ifndef PXR_IMAGING_HD_RENDER_INDEX_PREP_SCENE_INDEX_H
#define PXR_IMAGING_HD_RENDER_INDEX_PREP_SCENE_INDEX_H

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/primDataSourceOverlayCache.h"
#include "pxr/imaging/hd/primvarsSchema.h"

#include "pxr/imaging/hd/sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

// ----------------------------------------------------------------------------

TF_DECLARE_REF_PTRS(HdRenderIndexPrepSceneIndex);

// builds and caches HdPrimvarDescriptorVectors due to repeated access from
// current render delegates
class HdRenderIndexPrepSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:

    static HdRenderIndexPrepSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr inputScene) {
        return TfCreateRefPtr(new HdRenderIndexPrepSceneIndex(inputScene));
    }

    ~HdRenderIndexPrepSceneIndex() override;

    // satisfying HdSceneIndexBase
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    TfTokenVector GetChildPrimNames(const SdfPath &primPath) const override;

protected:
    HdRenderIndexPrepSceneIndex(HdSceneIndexBaseRefPtr inputScene);

    // satisfying HdSingleInputFilteringSceneIndexBase
    void _PrimsAdded(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsRemoved(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _PrimsDirtied(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    class _OverlayCache : public HdPrimDataSourceOverlayCache
    {
    public:
        static std::shared_ptr<_OverlayCache> New() {
            return std::shared_ptr<_OverlayCache>(new _OverlayCache());
        }

    protected:
        _OverlayCache() : HdPrimDataSourceOverlayCache(false) {}

        TfTokenVector _GetOverlayNames(
            HdContainerDataSourceHandle inputDataSource) const override;
        HdDataSourceBaseHandle _ComputeOverlayDataSource(
            const TfToken &name,
            HdContainerDataSourceHandle inputDataSource,
            HdContainerDataSourceHandle parentOverlayDataSource) const override;
        HdDataSourceLocatorSet _GetOverlayDependencies(
            const TfToken &name) const override;

        HdDataSourceBaseHandle _ComputePrimvarDescriptors(
            HdContainerDataSourceHandle inputDataSource) const;
        HdDataSourceBaseHandle _ComputeExtComputationPrimvarDescriptors(
            HdContainerDataSourceHandle inputDataSource) const;
    };

    using _OverlayCacheSharedPtr = std::shared_ptr<_OverlayCache>;
    _OverlayCacheSharedPtr _cache;
};

// ----------------------------------------------------------------------------

#define HDPRIMVARDESCRIPTORSSCHEMA_TOKENS       \
    (__primvarDescriptors)                      \
    (__extComputationPrimvarDescriptors)        \

TF_DECLARE_PUBLIC_TOKENS(HdPrimvarDescriptorsSchemaTokens,
        HDPRIMVARDESCRIPTORSSCHEMA_TOKENS);

template <typename T>
class _HdBasePrimvarDescriptorsSchema : public HdSchema
{
public:
    _HdBasePrimvarDescriptorsSchema(HdContainerDataSourceHandle container)
    : HdSchema(container) {}

    using Type = VtArray<T>;
    using DataSourceType = HdTypedSampledDataSource<Type>;

    typename DataSourceType::Handle GetConstantPrimvarDescriptors() {
        return _GetTypedDataSource<DataSourceType>(
            HdPrimvarSchemaTokens->constant);
    }
    typename DataSourceType::Handle GetUniformPrimvarDescriptors() {
        return _GetTypedDataSource<DataSourceType>(
            HdPrimvarSchemaTokens->uniform);
    }
    typename DataSourceType::Handle GetVaryingPrimvarDescriptors() {
        return _GetTypedDataSource<DataSourceType>(
            HdPrimvarSchemaTokens->varying);
    }
    typename DataSourceType::Handle GetVertexPrimvarDescriptors() {
        return _GetTypedDataSource<DataSourceType>(
            HdPrimvarSchemaTokens->vertex);
    }
    typename DataSourceType::Handle GetFaceVaryingPrimvarDescriptors() {
        return _GetTypedDataSource<DataSourceType>(
            HdPrimvarSchemaTokens->faceVarying);
    }
    typename DataSourceType::Handle GetInstancePrimvarDescriptors() {
        return _GetTypedDataSource<DataSourceType>(
            HdPrimvarSchemaTokens->instance);
    }
    typename DataSourceType::Handle GetPrimvarDescriptorsForInterpolation(
            HdInterpolation interpolation) {
        switch (interpolation)
        {
            case HdInterpolationConstant:
                return GetConstantPrimvarDescriptors();
            case HdInterpolationUniform:
                return GetUniformPrimvarDescriptors();
            case HdInterpolationVarying:
                return GetVaryingPrimvarDescriptors();
            case HdInterpolationVertex:
                return GetVertexPrimvarDescriptors();
            case HdInterpolationFaceVarying:
                return GetFaceVaryingPrimvarDescriptors();
            case HdInterpolationInstance:
                return GetInstancePrimvarDescriptors();
            default:
                return nullptr;
        }
    }
};

class HdPrimvarDescriptorsSchema
    : public _HdBasePrimvarDescriptorsSchema<HdPrimvarDescriptor>
{
public:
    HdPrimvarDescriptorsSchema(HdContainerDataSourceHandle container)
    : _HdBasePrimvarDescriptorsSchema(container) {}

    static HdPrimvarDescriptorsSchema GetFromParent(
            HdContainerDataSourceHandle fromParentContainer) {
        return HdPrimvarDescriptorsSchema(
            fromParentContainer
            ? HdContainerDataSource::Cast(fromParentContainer->Get(
                    HdPrimvarDescriptorsSchemaTokens->__primvarDescriptors))
            : nullptr);
    }
};

class HdExtComputationPrimvarDescriptorsSchema
    : public _HdBasePrimvarDescriptorsSchema<HdExtComputationPrimvarDescriptor>
{
public:
    HdExtComputationPrimvarDescriptorsSchema(
        HdContainerDataSourceHandle container)
    : _HdBasePrimvarDescriptorsSchema(container) {}

    static HdExtComputationPrimvarDescriptorsSchema GetFromParent(
            HdContainerDataSourceHandle fromParentContainer) {
        return HdExtComputationPrimvarDescriptorsSchema(
            fromParentContainer
            ? HdContainerDataSource::Cast(fromParentContainer->Get(
                HdPrimvarDescriptorsSchemaTokens->__extComputationPrimvarDescriptors))
            : nullptr);
    }
};

// ----------------------------------------------------------------------------

PXR_NAMESPACE_CLOSE_SCOPE

#endif

