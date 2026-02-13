//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/xformResolver.h"

#include "pxr/usdImaging/usdSkelImaging/bindingSchema.h"
#include "pxr/usdImaging/usdSkelImaging/dataSourceUtils.h"

#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/usd/usdSkel/tokens.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

/// First matrix from an HdMatrixArrayDataSource.
class _FirstMatrixDataSource : public HdMatrixDataSource
{
public:
    HD_DECLARE_DATASOURCE(_FirstMatrixDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    GfMatrix4d GetTypedValue(const Time shutterOffset) override {
        TRACE_FUNCTION();

        const VtArray<GfMatrix4d> matrices =
            _inputSrc->GetTypedValue(shutterOffset);
        if (matrices.empty()) {
            static const GfMatrix4d id(1.0);
            return id;
        }
        return matrices[0];
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override {
        return _inputSrc->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    _FirstMatrixDataSource(HdMatrixArrayDataSourceHandle inputSrc)
     : _inputSrc(std::move(inputSrc))
    {
    }

    HdMatrixArrayDataSourceHandle const _inputSrc;
};

class _MatrixProductDataSource : public HdMatrixDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MatrixProductDataSource);


    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    GfMatrix4d GetTypedValue(const Time shutterOffset) override {
        TRACE_FUNCTION();

        if (_inputSrcs.empty()) {
            static const GfMatrix4d id(1.0);
            return id;
        }
        GfMatrix4d result = _inputSrcs[0]->GetTypedValue(shutterOffset);

        for (size_t i = 1; i < _inputSrcs.size(); ++i) {
            result = result * _inputSrcs[i]->GetTypedValue(shutterOffset);
        }

        return result;
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override {

        TRACE_FUNCTION();

        const std::vector<HdSampledDataSourceHandle> h(
            _inputSrcs.begin(), _inputSrcs.end());

        return HdGetMergedContributingSampleTimesForInterval(
            h.size(), h.data(),
            startTime, endTime, outSampleTimes);
    }

private:
    _MatrixProductDataSource(std::vector<HdMatrixDataSourceHandle> inputSrcs)
     : _inputSrcs(std::move(inputSrcs))
    {
    }

    const std::vector<HdMatrixDataSourceHandle> _inputSrcs;
};

// Follows the instancedBy schema to find instancers.
// If the prototype has no skel root (as in the UsdSkelImagingBindingSchema),
// we stop.
//
VtArray<SdfPath>
_ComputeInstancerPaths(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    HdContainerDataSourceHandle primSource)
{
    TRACE_FUNCTION();
    
    VtArray<SdfPath> result;

    while(true) {
        const auto instancedBySchema =
            HdInstancedBySchema::GetFromParent(primSource);
        if (!instancedBySchema) {
            break;
        }
        const HdPathArrayDataSourceHandle prototypeRootsDs =
            instancedBySchema.GetPrototypeRoots();
        if (!prototypeRootsDs) {
            break;
        }
        const VtArray<SdfPath> prototypeRoots =
            prototypeRootsDs->GetTypedValue(0.0f);
        if (prototypeRoots.empty()) {
            break;
        }
        const SdfPath &prototypeRoot = prototypeRoots[0];
        const HdBoolDataSourceHandle hasSkelRootDs =
            UsdSkelImagingBindingSchema::GetFromParent(
                    sceneIndex->GetPrim(prototypeRoot).dataSource)
                .GetHasSkelRoot();
        if (!(hasSkelRootDs && hasSkelRootDs->GetTypedValue(0.0f))) {
            break;
        }
        const HdPathArrayDataSourceHandle pathsDs =
            instancedBySchema.GetPaths();
        if (!pathsDs) {
            break;
        }
        const VtArray<SdfPath> paths = pathsDs->GetTypedValue(0.0f);
        if (paths.empty()) {
            break;
        }
        const SdfPath &instancerPath = paths[0];
        primSource = sceneIndex->GetPrim(instancerPath).dataSource;
        result.push_back(instancerPath);
    }

    return result;
}

}

const HdDataSourceLocator &
UsdSkelImagingDataSourceXformResolver::GetInstancedByLocator()
{
    return HdInstancedBySchema::GetDefaultLocator();
}

const HdDataSourceLocator &
UsdSkelImagingDataSourceXformResolver::GetXformLocator()
{
    return HdXformSchema::GetDefaultLocator();
}

const HdDataSourceLocator &
UsdSkelImagingDataSourceXformResolver::GetInstanceXformLocator()
{
    static const HdDataSourceLocator locator(
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(HdInstancerTokens->instanceTransforms));
    return locator;
}

const HdDataSourceLocator &
UsdSkelImagingDataSourceXformResolver::GetInstanceAnimationSourceLocator()
{
    static const HdDataSourceLocator locator(
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(UsdSkelTokens->skelAnimationSource));
    return locator;
}

UsdSkelImagingDataSourceXformResolver::UsdSkelImagingDataSourceXformResolver(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    HdContainerDataSourceHandle const &primSource)
 : _sceneIndex(sceneIndex)
 , _primSource(primSource)
 , _instancerPaths(_ComputeInstancerPaths(sceneIndex, primSource))
{
}

HdMatrixDataSourceHandle
UsdSkelImagingDataSourceXformResolver::GetPrimLocalToCommonSpace() const
{
    TRACE_FUNCTION();

    // Transform on prim itself.
    HdMatrixDataSourceHandle const primXformSrc =
        HdXformSchema::GetFromParent(_primSource).GetMatrix();

    if (_instancerPaths.empty()) {
        return primXformSrc;
    }

    std::vector<HdMatrixDataSourceHandle> xformSrcs;
    if (primXformSrc) {
        xformSrcs.push_back(primXformSrc);
    }

    for (const SdfPath &instancerPath : _instancerPaths) {
        HdContainerDataSourceHandle const primDs =
            _sceneIndex->GetPrim(instancerPath).dataSource;
        if (auto const instanceXformSrc = HdMatrixArrayDataSource::Cast(
                HdPrimvarsSchema::GetFromParent(primDs)
                    .GetPrimvar(HdInstancerTokens->instanceTransforms)
                    .GetPrimvarValue())) {
            // Add instance transform.
            xformSrcs.push_back(
                _FirstMatrixDataSource::New(
                    instanceXformSrc));
        }
        if (HdMatrixDataSourceHandle const xformSrc =
                HdXformSchema::GetFromParent(primDs).GetMatrix()) {
            // Add instancer's transform.
            xformSrcs.push_back(xformSrc);
        }
    }

    switch(xformSrcs.size()) {
    case 0: return nullptr;
    case 1: return xformSrcs[0];
    default: return _MatrixProductDataSource::New(std::move(xformSrcs));
    };
}

VtArray<SdfPath>
UsdSkelImagingDataSourceXformResolver::GetInstanceAnimationSource() const
{
    // For nested instances we traverse the chain up and return the first 
    // aggregated animation source instance primvar.
    for (const SdfPath& instancerPath: _instancerPaths) {
        const HdPrimvarsSchema& primvars = HdPrimvarsSchema::GetFromParent(
            _sceneIndex->GetPrim(instancerPath).dataSource);
        const VtArray<SdfPath>& instanceAnimSource = 
            UsdSkelImagingGetTypedValue(HdPathArrayDataSource::Cast(
                primvars.GetPrimvar(UsdSkelTokens->skelAnimationSource)
                    .GetPrimvarValue()));
        if (!instanceAnimSource.empty()) {
            return instanceAnimSource;
        }
    }
    return {};
}


PXR_NAMESPACE_CLOSE_SCOPE
