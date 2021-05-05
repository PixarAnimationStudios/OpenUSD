//
// Copyright 2019 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/imaging/hdx/colorizeSelectionTask.h"

#include "pxr/imaging/hd/renderBuffer.h"

#include "pxr/imaging/hdx/fullscreenShader.h"
#include "pxr/imaging/hdx/hgiConversions.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hgi/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((outlineFrag, "OutlineFragment"))
    (colorIn)
    (enableOutline)
    (radius)
    (texelSize)    
);

HdxColorizeSelectionTask::HdxColorizeSelectionTask(
    HdSceneDelegate* delegate,
    SdfPath const& id)
  : HdxTask(id)
  , _params()
  , _lastVersion(-1)
  , _hasSelection(false)
  , _selectionOffsets()
  , _primId(nullptr)
  , _instanceId(nullptr)
  , _elementId(nullptr)
  , _outputBuffer(nullptr)
  , _outputBufferSize(0)
  , _converged(false)
  , _compositor()
  , _pipelineCreated(false)
{
}

HdxColorizeSelectionTask::~HdxColorizeSelectionTask()
{
    delete[] _outputBuffer;

    if (_texture) {
        _GetHgi()->DestroyTexture(&_texture);
    }
}

bool
HdxColorizeSelectionTask::IsConverged() const
{
    return _converged;
}

void
HdxColorizeSelectionTask::_Sync(HdSceneDelegate* delegate,
                                HdTaskContext* ctx,
                                HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!_compositor) {
        _compositor = std::make_unique<HdxFullscreenShader>(
            _GetHgi(), "ColorizeSelection");
    }

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        _GetTaskParams(delegate, &_params);
    }
    *dirtyBits = HdChangeTracker::Clean;

    HdxSelectionTrackerSharedPtr sel;
    if (_GetTaskContextData(ctx, HdxTokens->selectionState, &sel)) {
        sel->UpdateSelection(&(delegate->GetRenderIndex()));
    }
}

void
HdxColorizeSelectionTask::Prepare(HdTaskContext* ctx,
                                  HdRenderIndex *renderIndex)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _primId = static_cast<HdRenderBuffer*>(
        renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer,
                              _params.primIdBufferPath));
    _instanceId = static_cast<HdRenderBuffer*>(
        renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer,
                              _params.instanceIdBufferPath));
    _elementId = static_cast<HdRenderBuffer*>(
        renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer,
                              _params.elementIdBufferPath));

    HdxSelectionTrackerSharedPtr sel;
    _GetTaskContextData(ctx, HdxTokens->selectionState, &sel);

    if (sel && sel->GetVersion() != _lastVersion) {
        _lastVersion = sel->GetVersion();
        _hasSelection =
            sel->GetSelectionOffsetBuffer(renderIndex, _params.enableSelection,
                                          &_selectionOffsets);
    }
}

void
HdxColorizeSelectionTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!_HasTaskContextData(ctx, HdAovTokens->color)) {
        _converged = true;
        return;
    }

    // The color aov has the rendered results and we wish to apply the selection
    // colorization on top of it.
    HgiTextureHandle aovTexture;
    _GetTaskContextData(ctx, HdAovTokens->color, &aovTexture);

    // instance ID and element ID are optional inputs, but if we don't have
    // a prim ID buffer, skip doing anything.
    if (!_primId) {
        _converged = true;
        return;
    }

    // If there's nothing in the selection buffer, return.
    if (!_hasSelection) {
        _converged = true;
        return;
    }

    _primId->Resolve();
    _converged = _primId->IsConverged();
    size_t size = _primId->GetWidth() * _primId->GetHeight();

    if (_instanceId) {
        _instanceId->Resolve();
        size_t iidSize = _instanceId->GetWidth() * _instanceId->GetHeight();
        if (iidSize != size) {
            TF_WARN("Instance Id buffer %s has different dimensions "
                    "than Prim Id buffer %s",
                    _params.instanceIdBufferPath.GetText(),
                    _params.primIdBufferPath.GetText());
            return;
        }
        _converged = _converged && _instanceId->IsConverged();
    }
    if (_elementId) {
        _elementId->Resolve();
        size_t eidSize = _elementId->GetWidth() * _elementId->GetHeight();
        if (eidSize != size) {
            TF_WARN("Element Id buffer %s has different dimensions "
                    "than Prim Id buffer %s",
                    _params.elementIdBufferPath.GetText(),
                    _params.primIdBufferPath.GetText());
            return;
        }
        _converged = _converged && _elementId->IsConverged();
    }

    // Allocate the scratch space, if needed.
    if (!_outputBuffer || _outputBufferSize != size) {
        delete[] _outputBuffer;
        _outputBuffer = new uint8_t[size * 4];
        _outputBufferSize = size;
    }

    // Colorize!
    _ColorizeSelection();

    // Blit!
    //Make set program take
    
    HgiShaderFunctionDesc fragDesc;
    fragDesc.debugName = _tokens->outlineFrag.GetString();
    fragDesc.shaderStage = HgiShaderStageFragment;
    HgiShaderFunctionAddStageInput(
        &fragDesc, "uvOut", "vec2");
    HgiShaderFunctionAddTexture(
        &fragDesc, "colorIn");
    
    HgiShaderFunctionAddConstantParam(
        &fragDesc, "texelSize", "vec2");
    HgiShaderFunctionAddConstantParam(
        &fragDesc, "enableOutline", "int");
    HgiShaderFunctionAddConstantParam(
        &fragDesc, "radius", "int");
    
    HgiShaderFunctionAddStageOutput(
        &fragDesc, "hd_FragColor", "vec4", "color");
    
    _compositor->SetProgram(HdxPackageOutlineShader(), _tokens->outlineFrag, fragDesc);

    _CreateTexture(
        _primId->GetWidth(), 
        _primId->GetHeight(),
        HdFormatUNorm8Vec4, 
        _outputBuffer);

    _compositor->BindTextures({_tokens->colorIn}, {_texture});

    if (_UpdateParameterBuffer()) {
        const size_t byteSize = sizeof(_ParameterBuffer);
        _compositor->SetShaderConstants(byteSize, &_parameterData);
    }

    // Blend the selection color on top.  ApplySelectionColor uses the
    // calculation:
    //   src.rgb = mix(src.rgb, selection.rgb, selection.a);
    //   src.a = src.a;
    // ... per mode.
    //
    // Since we only get one blend, we pre-multiply alpha into the selection
    // color, and the selection alpha is the residual value used to scale the
    // scene color. This gives us the blend func:
    // GL_ONE, GL_SRC_ALPHA, GL_ZERO, GL_ONE.
    if (!_pipelineCreated) {
        HgiDepthStencilState depthState;
        depthState.depthTestEnabled = false;
        depthState.depthWriteEnabled = false;
        depthState.stencilTestEnabled = false;

        _compositor->SetDepthState(depthState);
        _pipelineCreated = true;

        _compositor->SetBlendState(
            /*enable blending*/true,
            HgiBlendFactorOne,
            HgiBlendFactorSrcAlpha,
            HgiBlendOpAdd,
            HgiBlendFactorZero,
            HgiBlendFactorOne,
            HgiBlendOpAdd);
    }

    _compositor->Draw(aovTexture, /*no depth*/HgiTextureHandle());
}

GfVec4f
HdxColorizeSelectionTask::_GetColorForMode(int mode) const
{
    if (mode == 0) {
        return _params.selectionColor;
    } else if (mode == 1) {
        return _params.locateColor;
    } else {
        return GfVec4f(0);
    }
}

void
HdxColorizeSelectionTask::_ColorizeSelection()
{
    int32_t *piddata = reinterpret_cast<int32_t*>(_primId->Map());
    if (!piddata) {
        // Skip the colorizing if we can't look up prim ID
        return;
    }
    int32_t *iiddata = reinterpret_cast<int32_t*>(_instanceId->Map());
    int32_t *eiddata = reinterpret_cast<int32_t*>(_elementId->Map());

    for (size_t i = 0; i < _outputBufferSize; ++i) {
        GfVec4f output = GfVec4f(0,0,0,1);

        int primId = piddata ? piddata[i] : -1;
        int instanceId = iiddata ? iiddata[i] : -1;
        int elementId = eiddata ? eiddata[i] : -1;

        for (int mode = 0; mode < _selectionOffsets[0]; ++mode) {
            if (primId == -1) {
                continue;
            }

            int modeOffset = _selectionOffsets[mode+1];
            if (modeOffset == 0) {
                continue;
            }

            int smin = _selectionOffsets[modeOffset];
            int smax = _selectionOffsets[modeOffset+1];

            if (primId >= smin && primId < smax) {
                int offset = modeOffset + 2 + primId - smin;
                int selectionData = _selectionOffsets[offset];
                bool sel = bool(selectionData & 0x1);
                int nextOffset = selectionData >> 1;

                if (nextOffset != 0 && !sel) {
                    int subprimType = _selectionOffsets[nextOffset];
                    if (subprimType == 3 /* INSTANCE */) {
                        int imin = _selectionOffsets[nextOffset+1];
                        int imax = _selectionOffsets[nextOffset+2];
                        if (instanceId >= imin && instanceId < imax) {
                            offset = nextOffset + 3 + instanceId - imin;
                            selectionData = _selectionOffsets[offset];
                            sel = sel || bool(selectionData & 0x1);
                            nextOffset = selectionData >> 1;
                        }
                    }
                    subprimType = _selectionOffsets[nextOffset];
                    if (subprimType == 0 /* ELEMENT */) {
                        int emin = _selectionOffsets[nextOffset+1];
                        int emax = _selectionOffsets[nextOffset+2];
                        if (elementId >= emin && elementId < emax) {
                            offset = nextOffset + 3 + elementId - emin;
                            selectionData = _selectionOffsets[offset];
                            sel = sel || bool(selectionData & 0x1);
                            nextOffset = selectionData >> 1;
                        }
                    }
                }

                if (sel) {
                    // dst.rgb = mix(dst.rgb, selection.rgb, selection.a)
                    // dst.a = mix(dst.a, 0, selection.a);
                    GfVec4f modeColor = _GetColorForMode(mode);
                    output[0] = modeColor[3] * modeColor[0] +
                               (1 - modeColor[3]) * output[0];
                    output[1] = modeColor[3] * modeColor[1] +
                               (1 - modeColor[3]) * output[1];
                    output[2] = modeColor[3] * modeColor[2] +
                               (1 - modeColor[3]) * output[2];
                    output[3] = (1 - modeColor[3]) * output[3];
                }
            }
        }
        _outputBuffer[i*4 + 0] = uint8_t(output[0] * 255.0f);
        _outputBuffer[i*4 + 1] = uint8_t(output[1] * 255.0f);
        _outputBuffer[i*4 + 2] = uint8_t(output[2] * 255.0f);
        _outputBuffer[i*4 + 3] = uint8_t(output[3] * 255.0f);
    }

    _primId->Unmap();
    if (iiddata) {
        _instanceId->Unmap();
    }
    if (eiddata) {
        _elementId->Unmap();
    }
}

bool
HdxColorizeSelectionTask::_UpdateParameterBuffer()
{
    _ParameterBuffer pb;

    if(_primId->GetWidth() > 0 && _primId->GetHeight() > 0) {
        pb.texelSize[0] = 1.0f / _primId->GetWidth();
        pb.texelSize[1] = 1.0f / _primId->GetHeight();
    }

    pb.enableOutline = (int)_params.enableOutline;
    pb.radius = (int)_params.outlineRadius;

    // All data is still the same, no need to update the compositor
    if (pb == _parameterData) {
        return false;
    }

    _parameterData = pb;
    return true;
}

void
HdxColorizeSelectionTask::_CreateTexture(
    int width, 
    int height,
    HdFormat format,
    void *data)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Destroy the old texture (if any) if we received new pixels.
    if (_texture) {
        _GetHgi()->DestroyTexture(&_texture);
    }

    // Texture was removed, exit.
    if (width == 0 || height == 0 || data == nullptr) {
        return;
    }

    const size_t pixelByteSize = HdDataSizeOfFormat(format);

    HgiTextureDesc texDesc;
    texDesc.debugName = "HdxColorizeSelectionTask texture";
    texDesc.dimensions = GfVec3i(width, height, 1);
    texDesc.format = HdxHgiConversions::GetHgiFormat(format);
    texDesc.initialData = data;
    texDesc.layerCount = 1;
    texDesc.mipLevels = 1;
    texDesc.pixelsByteSize = width * height * pixelByteSize;
    texDesc.sampleCount = HgiSampleCount1;
    texDesc.usage = HgiTextureUsageBitsShaderRead;
    _texture = _hgi->CreateTexture(texDesc);
}

// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out,
                         const HdxColorizeSelectionTaskParams& pv)
{
    out << "ColorizeSelectionTask Params: (...) "
        << pv.enableSelection << " "
        << pv.selectionColor << " "
        << pv.locateColor << " "
        << pv.primIdBufferPath << " "
        << pv.instanceIdBufferPath << " "
        << pv.elementIdBufferPath;
    return out;
}

bool operator==(const HdxColorizeSelectionTaskParams& lhs,
                const HdxColorizeSelectionTaskParams& rhs)
{
    return lhs.enableSelection      == rhs.enableSelection      &&
           lhs.selectionColor       == rhs.selectionColor       &&
           lhs.locateColor          == rhs.locateColor          &&
           lhs.primIdBufferPath     == rhs.primIdBufferPath     &&
           lhs.instanceIdBufferPath == rhs.instanceIdBufferPath &&
           lhs.elementIdBufferPath  == rhs.elementIdBufferPath;
}

bool operator!=(const HdxColorizeSelectionTaskParams& lhs,
                const HdxColorizeSelectionTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
