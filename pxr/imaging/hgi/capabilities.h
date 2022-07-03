//
// Copyright 2021 Pixar
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
#ifndef PXR_IMAGING_HGI_CAPABILITIES_H
#define PXR_IMAGING_HGI_CAPABILITIES_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiCapabilities
///
/// Reports the capabilities of the Hgi device.
///
class HgiCapabilities
{
public:
    HGI_API
    virtual ~HgiCapabilities() = 0;

    bool IsSet(HgiDeviceCapabilities mask) const {
        return (_flags & mask) != 0;
    }

    HGI_API
    virtual int GetAPIVersion() const = 0;
    
    HGI_API
    virtual int GetShaderVersion() const = 0;

    HGI_API
    size_t GetMaxUniformBlockSize() const {
        return _maxUniformBlockSize;
    }

    HGI_API
    size_t GetMaxShaderStorageBlockSize() const {
        return _maxShaderStorageBlockSize;
    }

    HGI_API
    size_t GetUniformBufferOffsetAlignment() const {
        return _uniformBufferOffsetAlignment;
    }

    HGI_API
    size_t GetMaxClipDistances() const {
        return _maxClipDistances;
    }

    HGI_API
    size_t GetPageSizeAlignment() const {
        return _pageSizeAlignment;
    }

protected:
    HgiCapabilities()
        : _maxUniformBlockSize(0)
        , _maxShaderStorageBlockSize(0)
        , _uniformBufferOffsetAlignment(0)
        , _maxClipDistances(0)
        , _pageSizeAlignment(1)
        , _flags(0)
    {}

    void _SetFlag(HgiDeviceCapabilities mask, bool value) {
        if (value) {
            _flags |= mask;
        } else {
            _flags &= ~mask;
        }
    }

    size_t _maxUniformBlockSize;
    size_t _maxShaderStorageBlockSize;
    size_t _uniformBufferOffsetAlignment;
    size_t _maxClipDistances;
    size_t _pageSizeAlignment;

private:
    HgiCapabilities & operator=(const HgiCapabilities&) = delete;
    HgiCapabilities(const HgiCapabilities&) = delete;

    HgiDeviceCapabilities _flags;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
