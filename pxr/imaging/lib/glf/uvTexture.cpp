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
/// \file UVTexture.cpp
// 

#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/image.h"
#include "pxr/imaging/glf/arrayTexture.h"
#include "pxr/imaging/glf/uvTexture.h"
#include "pxr/imaging/glf/uvTextureData.h"
#include "pxr/imaging/glf/utils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


// Custom factory to handle UVTexture and ArrayTexture for same types.
class Glf_UVTextureFactory : public GlfTextureFactoryBase {
public:
    virtual GlfTextureRefPtr New(const TfToken& texturePath,
                                 GlfImage::ImageOriginLocation originLocation =
                                                GlfImage::OriginUpperLeft) const
    {
        return GlfUVTexture::New(texturePath,
                                 /*cropTop*/ 0,
                                 /*cropBottom*/ 0,
                                 /*cropLeft*/ 0,
                                 /*cropRight*/ 0,
                                 originLocation);
    }

    virtual GlfTextureRefPtr New(const TfTokenVector& texturePaths,
                                 GlfImage::ImageOriginLocation originLocation = 
                                                GlfImage::OriginUpperLeft) const
    {
        return GlfArrayTexture::New(texturePaths, 
                                    texturePaths.size(), 
                                    /*cropTop*/ 0,
                                    /*cropBottom*/ 0,
                                    /*cropLeft*/ 0,
                                    /*cropRight*/ 0,
                                    originLocation);
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    typedef GlfUVTexture Type;
    TfType t = TfType::Define<Type, TfType::Bases<GlfBaseTexture> >();
    t.SetFactory< Glf_UVTextureFactory >();
}

GlfUVTextureRefPtr 
GlfUVTexture::New(
    TfToken const &imageFilePath,
    unsigned int cropTop,
    unsigned int cropBottom,
    unsigned int cropLeft,
    unsigned int cropRight,
    GlfImage::ImageOriginLocation originLocation)
{
    return TfCreateRefPtr(new GlfUVTexture(
            imageFilePath, cropTop, 
            cropBottom, cropLeft, cropRight,
            originLocation));
}

GlfUVTextureRefPtr 
GlfUVTexture::New(
    std::string const &imageFilePath,
    unsigned int cropTop,
    unsigned int cropBottom,
    unsigned int cropLeft,
    unsigned int cropRight,
    GlfImage::ImageOriginLocation originLocation)
{
    return TfCreateRefPtr(new GlfUVTexture(
            TfToken(imageFilePath), cropTop, 
            cropBottom, cropLeft, cropRight,
            originLocation));
}

bool 
GlfUVTexture::IsSupportedImageFile(TfToken const &imageFilePath)
{
    return IsSupportedImageFile(imageFilePath.GetString());
}

bool 
GlfUVTexture::IsSupportedImageFile(std::string const &imageFilePath)
{
    return GlfImage::IsSupportedImageFile(imageFilePath);
}

GlfUVTexture::GlfUVTexture(
    TfToken const &imageFilePath,
    unsigned int cropTop,
    unsigned int cropBottom,
    unsigned int cropLeft,
    unsigned int cropRight,
    GlfImage::ImageOriginLocation originLocation)
    : GlfBaseTexture(originLocation)
    , _imageFilePath(imageFilePath)
    , _cropTop(cropTop)
    , _cropBottom(cropBottom)
    , _cropLeft(cropLeft)
    , _cropRight(cropRight)
{
    /* nothing */
}

VtDictionary
GlfUVTexture::GetTextureInfo(bool forceLoad)
{
    VtDictionary info = GlfBaseTexture::GetTextureInfo(forceLoad);

    info["imageFilePath"] = _imageFilePath;

    return info;
}

bool
GlfUVTexture::IsMinFilterSupported(GLenum filter)
{
    return true;
}

void
GlfUVTexture::_ReadTexture()
{
    GlfUVTextureDataRefPtr texData =
        GlfUVTextureData::New(_GetImageFilePath(), GetMemoryRequested(),
                              _GetCropTop(), _GetCropBottom(),
                              _GetCropLeft(), _GetCropRight());
    if (texData) {
        texData->Read(0, _GenerateMipmap(), GetOriginLocation());
    }
    _UpdateTexture(texData);
    _CreateTexture(texData, _GenerateMipmap());
    _SetLoaded();
}

bool
GlfUVTexture::_GenerateMipmap() const
{
    return true;
}

const TfToken&
GlfUVTexture::_GetImageFilePath() const
{
    return _imageFilePath;
}

PXR_NAMESPACE_CLOSE_SCOPE

