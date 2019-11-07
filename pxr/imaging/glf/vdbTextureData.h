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
#ifndef PXR_IMAGING_GLF_VDB_TEXTURE_DATA_H
#define PXR_IMAGING_GLF_VDB_TEXTURE_DATA_H

/// \file glf/vdbTextureData.h

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/image.h"
#include "pxr/imaging/glf/baseTextureData.h"

#include "pxr/base/gf/bbox3d.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(GlfVdbTextureData);

class GlfVdbTextureData_DenseGridHolderBase;

/// \class GlfVdbTextureData
///
/// Implements GlfBaseTextureData to load an OpenVDB file
///
class GlfVdbTextureData : public GlfBaseTextureData {
public:
    GLF_API
    static GlfVdbTextureDataRefPtr
    New(std::string const &filePath, size_t targetMemory);

    /// See GlfVdbTexture for details
    const GfBBox3d &GetBoundingBox() const { return _boundingBox; }

    GLF_API
    int NumDimensions() const override;

    GLF_API
    int ResizedWidth(int mipLevel = 0) const override;

    GLF_API
    int ResizedHeight(int mipLevel = 0) const override;

    GLF_API
    int ResizedDepth(int mipLevel = 0) const override;

    GLenum GLInternalFormat() const override;

    GLenum GLFormat() const override;

    GLenum GLType() const override;

    size_t TargetMemory() const override;

    WrapInfo GetWrapInfo() const override;

    size_t ComputeBytesUsed() const override;

    size_t ComputeBytesUsedByMip(int mipLevel = 0) const override;

    bool Read(int degradeLevel, 
              bool generateMipmap,
              GlfImage::ImageOriginLocation
                  originLocation = GlfImage::OriginUpperLeft) override;
    
    bool HasRawBuffer(int mipLevel = 0) const override;

    unsigned char * GetRawBuffer(int mipLevel = 0) const override;

    int GetNumMipLevels() const override;

private:
    GlfVdbTextureData(std::string const &filePath, size_t targetMemory);
    ~GlfVdbTextureData() override;

    const std::string _filePath;

    size_t _targetMemory;

    int _nativeWidth, _nativeHeight, _nativeDepth;
    int _bytesPerPixel;

    GLenum  _glInternalFormat, _glFormat, _glType;

    WrapInfo _wrapInfo;

    size_t _size;

    GfBBox3d _boundingBox;

    std::unique_ptr<GlfVdbTextureData_DenseGridHolderBase> _denseGrid;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
