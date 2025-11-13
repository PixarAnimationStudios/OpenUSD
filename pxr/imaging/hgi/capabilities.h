//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_CAPABILITIES_H
#define PXR_IMAGING_HGI_CAPABILITIES_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"

#include <array>
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

    /// The max texture dimension for 1D, 2D and 3D, in their respective
    /// element. In other words, when checking *all* dimensions for a N-D
    /// texture, use the N'th element as the allowed maximum.
    HGI_API
    const std::array<uint32_t, 3>& GetMaxTextureDimension() const {
        return _maxTextureDimension;
    }

protected:
    HgiCapabilities()
        : _maxUniformBlockSize(0)
        , _maxShaderStorageBlockSize(0)
        , _uniformBufferOffsetAlignment(0)
        , _maxClipDistances(0)
        , _pageSizeAlignment(1)
        , _maxTextureDimension{}
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
    std::array<uint32_t, 3> _maxTextureDimension;

private:
    HgiCapabilities & operator=(const HgiCapabilities&) = delete;
    HgiCapabilities(const HgiCapabilities&) = delete;

    HgiDeviceCapabilities _flags;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
