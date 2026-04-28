//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIGL_DEVICE_FILTER_H
#define PXR_IMAGING_HGIGL_DEVICE_FILTER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgi/deviceFilter.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgiGL/capabilities.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiGLDeviceFilter
///
/// HgiGL doesn't support device filtering. Context creation is handled by the
/// application. FilterDevice() is called but the return value is ignored.
class HgiGLDeviceFilter : public HgiDeviceFilter
{
public:
    virtual ~HgiGLDeviceFilter() = default;

    const TfToken& GetTargetHgiName() const override final
    {
        return HgiTokens->OpenGL;
    }

    bool FilterDevice(const HgiCapabilities& capabilities) final
    {
        return true;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
