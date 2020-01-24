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
#include "pxr/imaging/hdSt/renderBuffer.h"
#include "pxr/imaging/hdSt/hgiConversions.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hgi/blitEncoder.h"
#include "pxr/imaging/hgi/blitEncoderOps.h"
#include "pxr/imaging/hgi/immediateCommandBuffer.h"
#include "pxr/imaging/hgi/texture.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStRenderBuffer::HdStRenderBuffer(Hgi* hgi, SdfPath const& id)
    : HdRenderBuffer(id)
    , _hgi(hgi)
    , _dimensions(GfVec3i(0,0,1))
    , _format(HdFormatInvalid)
    , _usage(HgiTextureUsageBitsColorTarget)
    , _multiSampled(false)
    , _texture(nullptr)
    , _textureMS(nullptr)
    , _mappers(0)
    , _mappedBuffer()
{
}

HdStRenderBuffer::~HdStRenderBuffer()
{
}

bool
HdStRenderBuffer::Allocate(
    GfVec3i const& dimensions,
    HdFormat format,
    bool multiSampled)
{
    if (dimensions[0]<1 || dimensions[1]<1 || dimensions[2]<1) {
        TF_WARN("Invalid render buffer dimensions for: %s", GetId().GetText());
        return false;
    }

    _Deallocate();

    _dimensions = dimensions;
    _format = format;
    _multiSampled = multiSampled;

    // XXX HdFormat does not have a depth format and neither HdAovDescriptor
    // nor HdRenderBufferDescriptor have 'Purpose / Usage' flags that tell us
    // the usage is for depth. Temp hack: do a string-compare the path which
    // is build out of HdAovTokens.
    const TfToken& bufferName = GetId().GetNameToken();
    bool _isDepthBuffer = 
        TfStringEndsWith(bufferName.GetString(), HdAovTokens->depth);

    _usage = _isDepthBuffer ? HgiTextureUsageBitsDepthTarget : 
                              HgiTextureUsageBitsColorTarget;

    // Allocate new GPU resource
    HgiTextureDesc texDesc;
    texDesc.dimensions = _dimensions;
    texDesc.format = HdStHgiConversions::GetHgiFormat(_format);
    texDesc.usage = _usage;
    texDesc.sampleCount = HgiSampleCount1;
    _texture = _hgi->CreateTexture(texDesc);

    // Allocate multi-sample texture (optional)
    if (_multiSampled) {
        texDesc.sampleCount = HgiSampleCount4;
        _textureMS = _hgi->CreateTexture(texDesc);
    }

    return true;
}

void
HdStRenderBuffer::_Deallocate()
{
    // If the buffer is mapped while we're doing this, there's not a great
    // recovery path...
    TF_VERIFY(!IsMapped());

    _dimensions = GfVec3i(0,0,1);
    _format = HdFormatInvalid;
    _multiSampled = false;
    _usage = HgiTextureUsageBitsColorTarget;
    _mappers.store(0);

    // Deallocate GPU resource that backs this render buffer
    _hgi->DestroyTexture(&_texture);
    if (_textureMS) {
        _hgi->DestroyTexture(&_textureMS);
    }
}

void* 
HdStRenderBuffer::Map()
{
    _mappers.fetch_add(1);

    size_t formatByteSize = HdDataSizeOfFormat(_format);
    size_t dataByteSize = _dimensions[0] * 
                          _dimensions[1] * 
                          _dimensions[2] *
                          formatByteSize;

    _mappedBuffer.resize(dataByteSize);

    if (dataByteSize > 0) {
        HgiCopyResourceOp copyOp;
        copyOp.format = HdStHgiConversions::GetHgiFormat(_format);
        copyOp.usage = _usage;
        copyOp.dimensions = _dimensions;
        copyOp.sourceByteOffset = GfVec3i(0);
        copyOp.cpuDestinationBuffer = _mappedBuffer.data();
        copyOp.destinationByteOffset = GfVec3i(0);
        copyOp.destinationBufferByteSize = dataByteSize;
        copyOp.gpuSourceTexture = _texture;

        // Use blit encoder to record resource copy commands.
        HgiImmediateCommandBuffer& icb = _hgi->GetImmediateCommandBuffer();
        HgiBlitEncoderUniquePtr blitEncoder = icb.CreateBlitEncoder();

        blitEncoder->CopyTextureGpuToCpu(copyOp);
        blitEncoder->EndEncoding();
    }

    return _mappedBuffer.data();
}

void
HdStRenderBuffer::Unmap()
{
    // XXX We could consider clearing _mappedBuffer here to free RAM.
    //     For now we assume that Map() will be called frequently so we prefer
    //     to avoid the cost of clearing the buffer over memory savings.
    // _mappedBuffer.clear();
    _mappers.fetch_sub(1);
}

void
HdStRenderBuffer::Resolve()
{
    if (!_multiSampled) {
        return;
    }

    GfVec4i region(0,0, _dimensions[0], _dimensions[1]);

    HgiResolveImageOp resolveOp;
    resolveOp.usage = _usage;
    resolveOp.source = _textureMS;
    resolveOp.destination = _texture;
    resolveOp.sourceRegion = region;
    resolveOp.destinationRegion = region;

    // Use blit encoder to record resource copy commands.
    HgiImmediateCommandBuffer& icb = _hgi->GetImmediateCommandBuffer();
    HgiBlitEncoderUniquePtr blitEncoder = icb.CreateBlitEncoder();

    blitEncoder->ResolveImage(resolveOp);
    blitEncoder->EndEncoding();
}

HgiTextureHandle 
HdStRenderBuffer::GetHgiTextureHandle(bool multiSampled) const 
{
    if (multiSampled && _multiSampled) {
        return _textureMS;
    } else {
        return _texture;
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
