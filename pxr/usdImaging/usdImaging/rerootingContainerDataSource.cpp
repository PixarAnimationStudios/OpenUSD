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
        return srcPath.ReplacePrefix(_srcPrefix, _dstPrefix);
    }

private:
    _RerootingPathDataSource(
        HdPathDataSourceHandle inputDataSource,
        const SdfPath &srcPrefix,
        const SdfPath &dstPrefix)
      : _inputDataSource(std::move(inputDataSource))
      , _srcPrefix(srcPrefix)
      , _dstPrefix(dstPrefix)
    {
    }

    HdPathDataSourceHandle const _inputDataSource;
    const SdfPath _srcPrefix;
    const SdfPath _dstPrefix;
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

        const size_t n = result.size();

        if (n == 0) {
            return result;
        }

        size_t i = 0;

        // If _srcPrefix is absolute root path, we know that we
        // need to translate every path.
        if (!_srcPrefix.IsAbsoluteRootPath()) {
            // Find the first element where we need to change the path.
            //
            // Use const & so that paths[i] does not trigger VtArray
            // to make a copy.
            const VtArray<SdfPath> &paths = result.AsConst();
            while (!paths[i].HasPrefix(_srcPrefix)) {
                ++i;
                if (i == n) {
                    // No need to modify result if no path needed
                    // to be changed.
                    return result;
                }
            }
        }

        // Starting with the first element where the path matched the
        // prefix, process it and all following elements.
        for (; i < n; i++) {
            SdfPath &path = result[i];
            path = path.ReplacePrefix(_srcPrefix, _dstPrefix);
        }

        return result;
    }

private:
    _RerootingPathArrayDataSource(
        HdPathArrayDataSourceHandle inputDataSource,
        const SdfPath &srcPrefix,
        const SdfPath &dstPrefix)
      : _inputDataSource(std::move(inputDataSource))
      , _srcPrefix(srcPrefix)
      , _dstPrefix(dstPrefix)
    {
    }

    HdPathArrayDataSourceHandle const _inputDataSource;
    const SdfPath _srcPrefix;
    const SdfPath _dstPrefix;
};

// ----------------------------------------------------------------------------

HdDataSourceBaseHandle
_RerootingCreateDataSource(
    HdDataSourceBaseHandle const &inputDataSource,
    const SdfPath &srcPrefix,
    const SdfPath &dstPrefix);

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
            _srcPrefix,
            _dstPrefix);
    }

private:
    _RerootingVectorDataSource(
        HdVectorDataSourceHandle inputDataSource,
        const SdfPath &srcPrefix,
        const SdfPath &dstPrefix)
     : _inputDataSource(std::move(inputDataSource))
     , _srcPrefix(srcPrefix)
     , _dstPrefix(dstPrefix)
    {
    }

    HdVectorDataSourceHandle const _inputDataSource;
    const SdfPath _srcPrefix;
    const SdfPath _dstPrefix;
};

// ----------------------------------------------------------------------------

HdDataSourceBaseHandle
_RerootingCreateDataSource(
    HdDataSourceBaseHandle const &inputDataSource,
    const SdfPath &srcPrefix,
    const SdfPath &dstPrefix)
{
    if (!inputDataSource) {
        return nullptr;
    }

    if (auto containerDs = HdContainerDataSource::Cast(inputDataSource)) {
        return UsdImagingRerootingContainerDataSource::New(
            std::move(containerDs), srcPrefix, dstPrefix);
    }

    if (auto vectorDs = HdVectorDataSource::Cast(inputDataSource)) {
        return _RerootingVectorDataSource::New(
            std::move(vectorDs), srcPrefix, dstPrefix);
    }

    if (auto pathDataSource =
            HdTypedSampledDataSource<SdfPath>::Cast(inputDataSource)) {
        return _RerootingPathDataSource::New(
            std::move(pathDataSource), srcPrefix, dstPrefix);
    }

    if (auto pathArrayDataSource =
            HdTypedSampledDataSource<VtArray<SdfPath>>::Cast(inputDataSource)) {
        return _RerootingPathArrayDataSource::New(
            std::move(pathArrayDataSource),srcPrefix, dstPrefix);
    }

    return inputDataSource;
}

} // anonymous namespace

UsdImagingRerootingContainerDataSource::UsdImagingRerootingContainerDataSource(
    HdContainerDataSourceHandle inputDataSource,
    const SdfPath &srcPrefix,
    const SdfPath &dstPrefix)
 : _inputDataSource(std::move(inputDataSource))
 , _srcPrefix(srcPrefix)
 , _dstPrefix(dstPrefix)
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
        _srcPrefix,
        _dstPrefix);
}



PXR_NAMESPACE_CLOSE_SCOPE
