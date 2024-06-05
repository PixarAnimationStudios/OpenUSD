//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/renderBuffer.h"

#include "pxr/imaging/hdSt/dynamicUvTextureObject.h"
#include "pxr/imaging/hdSt/hgiConversions.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/subtextureIdentifier.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

static 
HgiTextureUsage _GetTextureUsage(HdFormat format, TfToken const &name)
{
    if (HdAovHasDepthSemantic(name)) {
        return HgiTextureUsageBitsDepthTarget;
    } else if (HdAovHasDepthStencilSemantic(name)) {
        return HgiTextureUsageBitsDepthTarget |
               HgiTextureUsageBitsStencilTarget;
    }

    // We are assuming at some point in a render buffer's lifetime it could be
    // used to read from, so provide that ability to the render buffer. This is 
    // especially useful when for the HgiVulkan back-end.
    return HgiTextureUsageBitsColorTarget | HgiTextureUsageBitsShaderRead;
}

HdStRenderBuffer::HdStRenderBuffer(
            HdStResourceRegistry * const resourceRegistry, SdfPath const& id)
    : HdRenderBuffer(id)
    , _resourceRegistry(resourceRegistry)
    , _format(HdFormatInvalid)
    , _msaaSampleCount(4)
    , _mappers(0)
    , _mappedBuffer()
{
}

HdStRenderBuffer::~HdStRenderBuffer() = default;

void
HdStRenderBuffer::Sync(HdSceneDelegate *sceneDelegate,
                       HdRenderParam *renderParam,
                       HdDirtyBits *dirtyBits)
{
    // Invoke base class processing for the DirtyDescriptor bit after pulling
    // the MSAA sample count, which is authored for consumption by Storm alone.
    if (*dirtyBits & DirtyDescription) {
        VtValue val = sceneDelegate->Get(GetId(),
                                HdStRenderBufferTokens->stormMsaaSampleCount);
        if (val.IsHolding<uint32_t>()) {
            _msaaSampleCount = val.UncheckedGet<uint32_t>();
        }
    }

    HdRenderBuffer::Sync(sceneDelegate, renderParam, dirtyBits);
}

HdStTextureIdentifier
HdStRenderBuffer::GetTextureIdentifier(const bool multiSampled)
{
    // The texture identifier has to be unique across different render
    // delegates sharing the same resource registry.
    //
    // Thus, we cannot just use the path of the render buffer. Adding
    // "this" pointer to ensure uniqueness.
    //
    std::string idStr = GetId().GetString();
    if (multiSampled) {
        idStr += " [MSAA]";
    }
    idStr += TfStringPrintf("[%p] ", this);


    return HdStTextureIdentifier(
        TfToken(idStr),
        // Tag as texture not being loaded from an asset by
        // texture registry but populated by us.
        std::make_unique<HdStDynamicUvSubtextureIdentifier>());
}

// Debug name for texture.
static
std::string
_GetDebugName(const HdStDynamicUvTextureObjectSharedPtr &textureObject)
{
    return textureObject->GetTextureIdentifier().GetFilePath().GetString();
}

static
void
_CreateTexture(
    HdStDynamicUvTextureObjectSharedPtr const &textureObject,
    const HgiTextureDesc &desc)
{
    HgiTextureHandle const texture = textureObject->GetTexture();
    if (texture && texture->GetDescriptor() == desc) {
        return;
    }

    textureObject->CreateTexture(desc);
}

bool
HdStRenderBuffer::Allocate(
    GfVec3i const& dimensions,
    const HdFormat format,
    const bool multiSampled)
{
    _format = format;

    if (_format == HdFormatInvalid) {
        _textureObject = nullptr;
        _textureMSAAObject = nullptr;
        return false;
    }

    if (!_textureObject) {
        // Allocate texture object if necessary.
        _textureObject =
            std::dynamic_pointer_cast<HdStDynamicUvTextureObject>(
                _resourceRegistry->AllocateTextureObject(
                    GetTextureIdentifier(/*multiSampled = */ false),
                    HdStTextureType::Uv));
        if (!_textureObject) {
            TF_CODING_ERROR("Expected HdStDynamicUvTextureObject");
            return false;
        }
    }
    
    if (multiSampled) {
        if (!_textureMSAAObject) {
            // Allocate texture object if necessary
            _textureMSAAObject =
                std::dynamic_pointer_cast<HdStDynamicUvTextureObject>(
                    _resourceRegistry->AllocateTextureObject(
                        GetTextureIdentifier(/*multiSampled = */ true),
                        HdStTextureType::Uv));
            if (!_textureMSAAObject) {
                TF_CODING_ERROR("Expected HdStDynamicUvTextureObject");
                return false;
            }
        }
    } else {
        // De-allocate texture object
        _textureMSAAObject = nullptr;
    }
            
    HgiTextureDesc texDesc;
    texDesc.debugName = _GetDebugName(_textureObject);
    texDesc.dimensions = dimensions;
    texDesc.type = (dimensions[2] > 1) ? HgiTextureType3D : HgiTextureType2D;
    texDesc.format = HdStHgiConversions::GetHgiFormat(format);
    texDesc.usage = _GetTextureUsage(format, GetId().GetNameToken());
    texDesc.sampleCount = HgiSampleCount1;

    // Allocate actual GPU resource
    _CreateTexture(_textureObject, texDesc);

    if (multiSampled) {
        texDesc.debugName = _GetDebugName(_textureMSAAObject);
        texDesc.sampleCount = HgiSampleCount(_msaaSampleCount);

        // Allocate actual GPU resource
        _CreateTexture(_textureMSAAObject, texDesc);
    }

    return true;
}

void
HdStRenderBuffer::_Deallocate()
{
    _textureObject = nullptr;
    _textureMSAAObject = nullptr;
}

void*
HdStRenderBuffer::Map()
{
    _mappers.fetch_add(1);

    if (!_textureObject) {
        return nullptr;
    }

    HgiTextureHandle const texture = _textureObject->GetTexture();

    if (!TF_VERIFY(_resourceRegistry)) {
        return nullptr;
    }

    Hgi * const hgi = _resourceRegistry->GetHgi();
    if (!TF_VERIFY(hgi)) {
        return nullptr;
    }

    size_t size = 0;
    _mappedBuffer = HdStTextureUtils::HgiTextureReadback(hgi, texture, &size);

    return _mappedBuffer.get();
}

void
HdStRenderBuffer::Unmap()
{
    // XXX We could consider clearing _mappedBuffer here to free RAM.
    //     For now we assume that Map() will be called frequently so we prefer
    //     to avoid the cost of clearing the buffer over memory savings.
    // _mappedBuffer.reset(nullptr);

    _mappers.fetch_sub(1);
}

void
HdStRenderBuffer::Resolve()
{
    // Textures are resolved at the end of a render pass via the graphicsCmds
    // by supplying the resolve textures to the graphicsCmds descriptor.
}

static
VtValue
_GetResource(const HdStDynamicUvTextureObjectSharedPtr &textureObject)
{
    if (textureObject) {
        return VtValue(textureObject->GetTexture());
    } else {
        return VtValue();
    }
}

VtValue
HdStRenderBuffer::GetResource(const bool multiSampled) const
{
    if (multiSampled) {
        return _GetResource(_textureMSAAObject);
    } else {
        return _GetResource(_textureObject);
    }
}

bool
HdStRenderBuffer::IsMultiSampled() const
{
    return bool(_textureMSAAObject);
}

uint32_t
HdStRenderBuffer::GetMSAASampleCount() const
{
    return _msaaSampleCount;
}

static
const GfVec3i &
_GetDimensions(HdStDynamicUvTextureObjectSharedPtr const &textureObject)
{
    static const GfVec3i invalidDimensions(0,0,0);

    if (!textureObject) {
        return invalidDimensions;
    }
    HgiTextureHandle const texture = textureObject->GetTexture();
    if (!texture) {
        return invalidDimensions;
    }
    return texture->GetDescriptor().dimensions;
}

unsigned int
HdStRenderBuffer::GetWidth() const
{
    return _GetDimensions(_textureObject)[0];
}

unsigned int
HdStRenderBuffer::GetHeight() const
{
    return _GetDimensions(_textureObject)[1];
}

unsigned int
HdStRenderBuffer::GetDepth() const
{
    return _GetDimensions(_textureObject)[2];
}

PXR_NAMESPACE_CLOSE_SCOPE
