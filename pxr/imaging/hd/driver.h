//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_DRIVER_H
#define PXR_IMAGING_HD_DRIVER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

/// HdDriver represents a device object, commonly a render device, that is owned
/// by the application and passed to HdRenderIndex. The RenderIndex passes it to
/// the render delegate and rendering tasks.
/// The application manages the lifetime (destruction) of HdDriver and must 
/// ensure it remains valid while Hydra is running.
class HdDriver {
public:
    TfToken name;
    VtValue driver;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
