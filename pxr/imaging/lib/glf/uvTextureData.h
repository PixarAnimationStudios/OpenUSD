//
// Copyright 2016 Pixar
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
#ifndef GLF_UVTEXTURE_DATA_H
#define GLF_UVTEXTURE_DATA_H

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/baseTextureData.h"

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


typedef std::shared_ptr<class GlfImage> GlfImageSharedPtr;

TF_DECLARE_WEAK_AND_REF_PTRS(GlfUVTextureData);

class GlfUVTextureData : public GlfBaseTextureData {
public:
    struct Params {
        Params() 
            : targetMemory(0)
            , cropTop(0)
            , cropBottom(0)
            , cropLeft(0)
            , cropRight(0)
        { }

        bool operator==(const Params& rhs) const
        {
            return (targetMemory == rhs.targetMemory &&
                    cropTop == rhs.cropTop           && 
                    cropBottom == rhs.cropBottom     && 
                    cropLeft == rhs.cropLeft         && 
                    cropRight == rhs.cropRight);
        }

        bool operator!=(const Params& rhs) const
        {
            return !(*this == rhs);
        }

        size_t targetMemory;
        unsigned int cropTop, cropBottom, cropLeft, cropRight;
    };

    GLF_API
    static GlfUVTextureDataRefPtr
    New(std::string const &filePath,
        size_t targetMemory,
        unsigned int cropTop,
        unsigned int cropBottom,
        unsigned int cropLeft,
        unsigned int cropRight);

    GLF_API
    static GlfUVTextureDataRefPtr
    New(std::string const &filePath, Params const &params);

    const Params& GetParams() const { return _params; }

    // GlfBaseTextureData overrides
    GLF_API
    virtual int ResizedWidth(int mipLevel = 0) const;

    GLF_API
    virtual int ResizedHeight(int mipLevel = 0) const;

    virtual GLenum GLInternalFormat() const {
        return _glInternalFormat;
    };

    virtual GLenum GLFormat() const {
        return _glFormat;
    };

    virtual GLenum GLType() const {
        return _glType;
    };

    virtual size_t TargetMemory() const {
        return _targetMemory;
    };

    virtual WrapInfo GetWrapInfo() const {
        return _wrapInfo;
    };

    GLF_API
    virtual size_t ComputeBytesUsed() const;

    GLF_API
    virtual size_t ComputeBytesUsedByMip(int mipLevel = 0) const;

    GLF_API
    virtual bool HasRawBuffer(int mipLevel = 0) const;

    GLF_API
    virtual unsigned char * GetRawBuffer(int mipLevel = 0) const;

    GLF_API
    virtual bool Read(int degradeLevel, bool generateMipmap);

    GLF_API
    virtual int GetNumMipLevels() const;

private:
    // A structure that keeps the mips loaded from disk in the format
    // that the gpu needs.
    struct Mip {
        Mip() 
            : size(0), offset(0), width(0), height(0)
        { }

        size_t size;
        size_t offset;
        int width;
        int height;
    };

    // A structure keeping a down-sampled image input and floats indicating the
    // downsample rate (e.g., if the resolution changed from 2048x1024 to
    // 512x256, scaleX=0.25 and scaleY=0.25).
    struct _DegradedImageInput {
        _DegradedImageInput(double scaleX, double scaleY, 
            GlfImageSharedPtr image) : scaleX(scaleX), scaleY(scaleY)
        { 
            images.push_back(image);
        }

        _DegradedImageInput(double scaleX, double scaleY)
            : scaleX(scaleX), scaleY(scaleY)
        { }

        double         scaleX;
        double         scaleY;
        std::vector<GlfImageSharedPtr> images;
    };

    // Reads an image using GlfImage. If possible and requested, it will
    // load a down-sampled version (when mipmapped .tex file) of the image.
    // If targetMemory is > 0, it will iterate through the down-sampled version
    // until the estimated required GPU memory is smaller than targetMemory.
    // Otherwise, it will use the given degradeLevel.
    // When estimating the required GPU memory, it will take into account that
    // the GPU might generate MipMaps.
    _DegradedImageInput _ReadDegradedImageInput(bool generateMipmap,
                                                size_t targetMemory,
                                                size_t degradeLevel);

    // Helper to read degraded image chains, given a starting mip and an 
    // ending mip it will fill the image chain.
    _DegradedImageInput _GetDegradedImageInputChain(double scaleX, 
                                                    double scaleY, 
                                                    int startMip, 
                                                    int lastMip);

    // Given a GlfImage it will return the number of mip levels that 
    // are actually valid to be loaded to the GPU. For instance, it will
    // drop textures with non valid OpenGL pyramids.
    int _GetNumMipLevelsValid(const GlfImageSharedPtr image) const;

    GlfUVTextureData(std::string const &filePath, Params const &params);
    virtual ~GlfUVTextureData();
        
    const std::string _filePath;
    const Params      _params;

    size_t _targetMemory;

    int _nativeWidth, _nativeHeight;
    int _resizedWidth, _resizedHeight;
    int _bytesPerPixel;

    GLenum  _glInternalFormat, _glFormat, _glType;

    WrapInfo _wrapInfo;

    size_t _size;

    std::unique_ptr<unsigned char[]> _rawBuffer;
    std::vector<Mip> _rawBufferMips;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // GLF_UVTEXTURE_DATA_H
