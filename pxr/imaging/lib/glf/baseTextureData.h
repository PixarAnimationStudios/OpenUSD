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
#ifndef GLF_BASETEXTUREDATA_H
#define GLF_BASETEXTUREDATA_H

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/image.h"
#include "pxr/imaging/glf/utils.h"
#include "pxr/imaging/garch/gl.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_AND_REF_PTRS(GlfBaseTextureData);

class GlfBaseTextureData : public TfRefBase,
                           public TfWeakBase
{
public:
    GLF_API
    virtual ~GlfBaseTextureData();

    GLF_API
    GlfBaseTextureData() = default;

    // Disallow copies
    GlfBaseTextureData(const GlfBaseTextureData&) = delete;
    GlfBaseTextureData& operator=(const GlfBaseTextureData&) = delete;

    struct WrapInfo {
        WrapInfo() :
          hasWrapModeS(false), hasWrapModeT(false),
          wrapModeS(GL_REPEAT), wrapModeT(GL_REPEAT) {};

        bool    hasWrapModeS;
        bool    hasWrapModeT;
        GLenum  wrapModeS;
        GLenum  wrapModeT;
    };

    virtual int ResizedWidth(int mipLevel = 0) const = 0;

    virtual int ResizedHeight(int mipLevel = 0) const = 0;

    virtual GLenum GLInternalFormat() const = 0;

    virtual GLenum GLFormat() const = 0;

    virtual GLenum GLType() const = 0;

    virtual size_t TargetMemory() const = 0;

    virtual WrapInfo GetWrapInfo() const = 0;

    virtual size_t ComputeBytesUsed() const = 0;

    virtual size_t ComputeBytesUsedByMip(int mipLevel = 0) const = 0;

    virtual bool Read(int degradeLevel, 
                      bool generateMipmap, 
                      GlfImage::ImageOriginLocation originLocation = 
                                                 GlfImage::OriginUpperLeft) = 0;
    
    virtual bool HasRawBuffer(int mipLevel = 0) const = 0;

    virtual unsigned char * GetRawBuffer(int mipLevel = 0) const = 0;   

    virtual int GetNumMipLevels() const = 0;

    virtual bool IsCompressed() const {
        return GlfIsCompressedFormat(GLFormat());
    }

protected:
    // Map image format and type and encoding to GL format.
    GLF_API
    static GLenum _GLInternalFormatFromImageData(
        GLenum format, GLenum type, bool isSRGB);

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // GLF_BASETEXTURE_DATA
