//
// Copyright 2020 benmalartre
//
// Unlicensed
//

#ifndef PXR_IMAGING_PLUGIN_LOFI_RENDER_PARAM_H
#define PXR_IMAGING_PLUGIN_LOFI_RENDER_PARAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderParam.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class LoFiRenderParam
///
class LoFiRenderParam final : public HdRenderParam {
public:
    LoFiRenderParam(){};

    virtual ~LoFiRenderParam() = default;

private:

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_RENDER_PARAM_H
