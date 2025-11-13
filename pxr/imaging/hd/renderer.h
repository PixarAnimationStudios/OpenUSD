//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_RENDERER_H
#define PXR_IMAGING_HD_RENDERER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdLegacyRenderControlInterface;

///
/// \class HdRenderer
///
/// A Hydra renderer. Typically constructed through a
/// HdRendererPlugin given a scene index. In general,
/// a subclass of HdRenderer has a constructor taking a scene index
/// and implements HdSceneIndexObserver behavior.
///
/// It is the Hydra 2.0 replacement of the HdRenderDelegate.
///
class HdRenderer
{
public:
    /// Stub class.
    /// TODO: Add API here to replace HdLegacyRenderControlInterface.
    virtual ~HdRenderer() = 0;

    /// Transitory Hydra-1.0-like API.
    virtual HdLegacyRenderControlInterface * GetLegacyRenderControl();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
