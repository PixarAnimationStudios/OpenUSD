//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_RENDER_DELEGATE_ADAPTER_RENDERER_H
#define PXR_IMAGING_HD_RENDER_DELEGATE_ADAPTER_RENDERER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/pluginRenderDelegateUniqueHandle.h"
#include "pxr/imaging/hd/renderer.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;
class HdEngine;
class HdDriver;
TF_DECLARE_REF_PTRS(HdSceneIndexBase);

///
/// \class HdRenderDelegateAdapterRenderer
///
/// A Hydra renderer that populates a Hydra 1.0 render delegate from a
/// (typically terminal) scene index using "back-end" emulation.
///
class HdRenderDelegateAdapterRenderer : public HdRenderer
{
public:
    HD_API
    HdRenderDelegateAdapterRenderer(
        HdPluginRenderDelegateUniqueHandle renderDelegate,
        HdSceneIndexBaseRefPtr const &terminalSceneIndex,
        HdContainerDataSourceHandle const &rendererCreateArgs);

    HD_API
    ~HdRenderDelegateAdapterRenderer() override;

    HdLegacyRenderControlInterface * GetLegacyRenderControl() override;
    
private:
    // Keep HdDriver's alive while HdRenderIndex/HdRenderDelegate has raw
    // pointers to HdDriver.
    const std::vector<HdDriver> _drivers;
    HdPluginRenderDelegateUniqueHandle const _renderDelegate;
    std::unique_ptr<HdRenderIndex> const _renderIndex;
    std::unique_ptr<HdEngine> const _engine;

    class _LegacyRenderControl;
    std::unique_ptr<_LegacyRenderControl> const _legacyRenderControl;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
