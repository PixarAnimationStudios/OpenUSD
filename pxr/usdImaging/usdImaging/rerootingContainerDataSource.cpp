//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/rerootingContainerDataSource.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

class _RerootingPathDataSource : public HdPathDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RerootingPathDataSource)

    VtValue GetValue(const Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        const Time startTime,
        const Time endTime,
        std::vector<Time> * const outSampleTimes) override
    {
        if (!_inputDataSource) {
            return false;
        }

        return _inputDataSource->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
    }

    SdfPath GetTypedValue(const Time shutterOffset) override
    {
        if (!_inputDataSource) {
            return SdfPath();
        }

        const SdfPath srcPath = _inputDataSource->GetTypedValue(shutterOffset);

        SdfPath targetPath = _mapFn.MapSourceToTarget(srcPath);

        // If the path is outside the mapping, this data source
        // implicitly leaves the path alone; it does not enforce
        // encapsulation in the way that USD composition arcs do.
        return targetPath.IsEmpty() ? srcPath : targetPath;
    }

private:
    _RerootingPathDataSource(
        HdPathDataSourceHandle inputDataSource,
        PcpMapFunction const& mapFn)
      : _inputDataSource(std::move(inputDataSource))
      , _mapFn(mapFn)
    {
    }

    const HdPathDataSourceHandle _inputDataSource;
    const PcpMapFunction _mapFn;
};

// ----------------------------------------------------------------------------

class _RerootingPathArrayDataSource : public HdPathArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RerootingPathArrayDataSource)

    VtValue GetValue(const Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        const Time startTime,
        const Time endTime,
        std::vector<Time>*  const outSampleTimes) override
    {
        if (!_inputDataSource) {
            return false;
        }

        return _inputDataSource->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

    VtArray<SdfPath> GetTypedValue(const Time shutterOffset) override
    {
        if (!_inputDataSource) {
            return {};
        }

        VtArray<SdfPath> result
            = _inputDataSource->GetTypedValue(shutterOffset);

        for (SdfPath &path: result) {
            SdfPath targetPath = _mapFn.MapSourceToTarget(path);
            if (!targetPath.IsEmpty()) {
                path = targetPath;
            }
        }

        return result;
    }

private:
    _RerootingPathArrayDataSource(
        HdPathArrayDataSourceHandle inputDataSource,
        PcpMapFunction const& mapFn)
      : _inputDataSource(std::move(inputDataSource))
      , _mapFn(mapFn)
    {
    }

    const HdPathArrayDataSourceHandle _inputDataSource;
    const PcpMapFunction _mapFn;
};

// ----------------------------------------------------------------------------

HdDataSourceBaseHandle
_RerootingCreateDataSource(
    HdDataSourceBaseHandle const &inputDataSource,
    PcpMapFunction const& mapFn);

class _RerootingVectorDataSource : public HdVectorDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RerootingVectorDataSource)

    size_t GetNumElements() {
        return _inputDataSource->GetNumElements();
    }

    HdDataSourceBaseHandle GetElement(const size_t element) {
        return _RerootingCreateDataSource(
            _inputDataSource->GetElement(element),
            _mapFn);
    }

private:
    _RerootingVectorDataSource(
        HdVectorDataSourceHandle inputDataSource,
        PcpMapFunction const& mapFn)
     : _inputDataSource(std::move(inputDataSource))
     , _mapFn(mapFn)
    {
    }

    const HdVectorDataSourceHandle _inputDataSource;
    const PcpMapFunction _mapFn;
};

// ----------------------------------------------------------------------------

HdDataSourceBaseHandle
_RerootingCreateDataSource(
    HdDataSourceBaseHandle const &inputDataSource,
    PcpMapFunction const& mapFn)
{
    if (!inputDataSource) {
        return nullptr;
    }

    if (auto containerDs = HdContainerDataSource::Cast(inputDataSource)) {
        return UsdImagingRerootingContainerDataSource::New(
            std::move(containerDs), mapFn);
    }

    if (auto vectorDs = HdVectorDataSource::Cast(inputDataSource)) {
        return _RerootingVectorDataSource::New(
            std::move(vectorDs), mapFn);
    }

    if (auto pathDataSource =
            HdTypedSampledDataSource<SdfPath>::Cast(inputDataSource)) {
        return _RerootingPathDataSource::New(
            std::move(pathDataSource), mapFn);
    }

    if (auto pathArrayDataSource =
            HdTypedSampledDataSource<VtArray<SdfPath>>::Cast(inputDataSource)) {
        return _RerootingPathArrayDataSource::New(
            std::move(pathArrayDataSource),mapFn);
    }

    return inputDataSource;
}

} // anonymous namespace

UsdImagingRerootingContainerDataSource::UsdImagingRerootingContainerDataSource(
    HdContainerDataSourceHandle inputDataSource,
    PcpMapFunction const& mapFn)
 : _inputDataSource(std::move(inputDataSource))
 , _mapFn(mapFn)
{
}

UsdImagingRerootingContainerDataSource::UsdImagingRerootingContainerDataSource(
    HdContainerDataSourceHandle inputDataSource,
    const SdfPath &srcPrefix,
    const SdfPath &dstPrefix)
 : _inputDataSource(std::move(inputDataSource))
 , _mapFn( PcpMapFunction::Create(
            {{srcPrefix, dstPrefix}}, SdfLayerOffset()))
{
}

UsdImagingRerootingContainerDataSource::
~UsdImagingRerootingContainerDataSource() = default;

TfTokenVector
UsdImagingRerootingContainerDataSource::GetNames()
{
    if (!_inputDataSource) {
        return {};
    }

    return _inputDataSource->GetNames();
}

HdDataSourceBaseHandle
UsdImagingRerootingContainerDataSource::Get(const TfToken &name)
{
    if (!_inputDataSource) {
        return nullptr;
    }

    return _RerootingCreateDataSource(
        _inputDataSource->Get(name),
        _mapFn);
}

PXR_NAMESPACE_CLOSE_SCOPE
