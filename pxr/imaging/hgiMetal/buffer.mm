//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <Metal/Metal.h>

#include "pxr/base/arch/defines.h"

#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/capabilities.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/buffer.h"


PXR_NAMESPACE_OPEN_SCOPE

HgiMetalBuffer::HgiMetalBuffer(HgiMetal *hgi, HgiBufferDesc const & desc)
    : HgiBuffer(desc)
    , _bufferId(nil)
{

    if (desc.byteSize == 0) {
        TF_CODING_ERROR("Buffers must have a non-zero length");
    }

    MTLResourceOptions options = MTLResourceCPUCacheModeDefaultCache |
        hgi->GetCapabilities()->defaultStorageMode;
    
    if (desc.initialData) {
        _bufferId = [hgi->GetPrimaryDevice() newBufferWithBytes:desc.initialData
                                                         length:desc.byteSize
                                                        options:options];
    }
    else {
        _bufferId = [hgi->GetPrimaryDevice() newBufferWithLength:desc.byteSize
                                                         options:options];
    }
    
    _descriptor.initialData = nullptr;

    HGIMETAL_DEBUG_LABEL(_bufferId, _descriptor.debugName.c_str());
}

HgiMetalBuffer::~HgiMetalBuffer()
{
    if (_bufferId != nil) {
        [_bufferId release];
        _bufferId = nil;
    }
}

size_t
HgiMetalBuffer::GetByteSizeOfResource() const
{
    return _descriptor.byteSize;
}

uint64_t
HgiMetalBuffer::GetRawResource() const
{
    return (uint64_t) _bufferId;
}

void*
HgiMetalBuffer::GetCPUStagingAddress()
{
    if (_bufferId) {
        return [_bufferId contents];
    }

    return nullptr;
}
PXR_NAMESPACE_CLOSE_SCOPE
