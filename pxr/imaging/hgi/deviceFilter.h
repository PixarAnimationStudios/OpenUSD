//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_DEVICE_FILTER_H
#define PXR_IMAGING_HGI_DEVICE_FILTER_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hgi/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiCapabilities;

///
/// \class HgiDeviceFilter
///
/// Implement this interface and pass an instance to
/// `Hgi::CreatePlatformDefaultHgi()` or `Hgi::CreateNamedHgi()` to hook into
/// the device selection process. Note that an implementations might choose to
/// ignore the device filter if it can't be supported (for example only one
/// device is ever available). The specific Hgi backends might offer a more
/// advanced subinterfaces that allow more control not only over device
/// filtering, but also creation.
///
/// Have a look at the subinterface documentation for more information on
/// limitations and additional features.
class HgiDeviceFilter
{
public:
    virtual ~HgiDeviceFilter() = default;

    /// If this filter should only be used for a certain Hgi implementation,
    /// then this function should return the name of that Hgi (for example
    /// HgiTokens->Vulkan). This is optional, by default this returns the empty
    /// token, which means "all Hgi implementations".
    virtual const TfToken& GetTargetHgiName() const
    {
        static const TfToken anyHgi{};
        return anyHgi;
    }

    /// Called by the Hgi backend with the computed capabilities for the device.
    /// Return true to accept this device, or false to reject it and try
    /// another. Note that if no other device is available, then the
    /// implementation might ignore the results and proceed anyway, or it might
    /// cause Hgi creation will fail.
    virtual bool FilterDevice(const HgiCapabilities& capabilities) = 0;
};

/// A list of HgiDeviceFilters. Only one filter per target Hgi is expected.
using HgiDeviceFilters = std::vector<HgiDeviceFilter*>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
