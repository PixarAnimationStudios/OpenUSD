//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIMETAL_DEVICE_FILTER_H
#define PXR_IMAGING_HGIMETAL_DEVICE_FILTER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgi/deviceFilter.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hgiMetal/api.h"
#include "pxr/imaging/hgiMetal/capabilities.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiMetalDeviceFilter
///
/// If all devices are filtered out, then HgiMetal will default to
/// MTLCreateSystemDefaultDevice().
class HgiMetalDeviceFilter : public HgiDeviceFilter
{
public:
    virtual ~HgiMetalDeviceFilter() = default;

     const TfToken& GetTargetHgiName() const override final
    {
        return HgiTokens->Metal;
    }

    virtual bool FilterDevice(const HgiCapabilities& capabilities) override
    {
        return true;
    }

    /// Overload for more detailed device filtering.
    virtual bool FilterDevice(const HgiMetalCapabilities& capabilities,
        id<MTLDevice> device) = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
