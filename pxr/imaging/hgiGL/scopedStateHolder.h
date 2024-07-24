//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIGL_STATE_H
#define PXR_IMAGING_HGIGL_STATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"

#include <cstdint>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HgiGLScopedStateHolder
///
/// OpenGL state guard object.
///
/// We've historically allowed applications to change global OpenGL. 
/// Consecutive code then relies on certain global state having been set.
/// This results in difficult to manage and inefficient OpenGL code.
///
/// For Hgi transition, we defensively capture state in this object and restore
/// it to the previous state to keep our applications function as before.
/// The end goal is to not need this object at all and make sure all opengl
/// state is only changed via HgiPipeline objects.
///
class HgiGL_ScopedStateHolder final
{
public:
    HGIGL_API
    HgiGL_ScopedStateHolder();

    HGIGL_API
    ~HgiGL_ScopedStateHolder();

private:
    HgiGL_ScopedStateHolder& operator=(const HgiGL_ScopedStateHolder&) = delete;
    HgiGL_ScopedStateHolder(const HgiGL_ScopedStateHolder&) = delete;

    int32_t _restoreRenderBuffer;
    int32_t _restoreVao;

    bool _restoreDepthTest;
    bool _restoreDepthWriteMask;
    int32_t _restoreDepthFunc;

    bool _restoreDepthBias;
    float _restoreDepthBiasConstantFactor;
    float _restoreDepthBiasSlopeFactor;

    bool _restoreStencilTest;
    int32_t _restoreStencilCompareFn[2];
    int32_t _restoreStencilReferenceValue[2];
    int32_t _restoreStencilFail[2];
    int32_t _restoreStencilReadMask[2];
    int32_t _restoreStencilPass[2];
    int32_t _restoreStencilDepthFail[2];
    int32_t _restoreStencilDepthPass[2];
    int32_t _restoreStencilWriteMask[2];

    int32_t _restoreViewport[4];
    bool _restoreBlendEnabled;
    int32_t _restoreColorOp;
    int32_t _restoreAlphaOp;
    int32_t _restoreColorSrcFnOp;
    int32_t _restoreAlphaSrcFnOp;
    int32_t _restoreColorDstFnOp;
    int32_t _restoreAlphaDstFnOp;
    float _restoreBlendColor[4];
    bool _restoreAlphaToCoverage;
    bool _restoreSampleAlphaToOne;
    float _lineWidth;
    bool _cullFace;
    int32_t _cullMode;
    int32_t _frontFace;
    bool _rasterizerDiscard;
    bool _restoreDepthClamp;
    float _depthRange[2];
    bool _restoreFramebufferSRGB;
    bool _restoreConservativeRaster;
    std::vector<bool> _restoreClipDistances;
    bool _restoreMultiSample;
    bool _restorePointSmooth;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
