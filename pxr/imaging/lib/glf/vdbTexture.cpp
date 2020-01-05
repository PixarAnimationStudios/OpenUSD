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
/// \file vdbTexture.cpp
// 

#include "pxr/imaging/glf/vdbTexture.h"
#ifdef PXR_OPENVDB_SUPPORT_ENABLED
#include "pxr/imaging/glf/vdbTextureData.h"
#else
#include "pxr/imaging/glf/baseTextureData.h"
#endif
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    using Type = GlfVdbTexture;
    TfType t = TfType::Define<Type, TfType::Bases<GlfBaseTexture> >();
    t.SetFactory< GlfTextureFactory<GlfVdbTexture> >();
}

GlfVdbTextureRefPtr 
GlfVdbTexture::New(TfToken const &filePath)
{
    return TfCreateRefPtr(new GlfVdbTexture(filePath));
}

GlfVdbTextureRefPtr 
GlfVdbTexture::New(std::string const &filePath)
{
    return New(TfToken(filePath));
}

int
GlfVdbTexture::GetNumDimensions() const
{
    return 3;
}

GlfVdbTexture::GlfVdbTexture(TfToken const &filePath)
    : GlfBaseTexture()
    , _filePath(filePath)
{
}

VtDictionary
GlfVdbTexture::GetTextureInfo(bool forceLoad)
{
    VtDictionary info = GlfBaseTexture::GetTextureInfo(forceLoad);

    info["imageFilePath"] = _filePath;

    return info;
}

bool
GlfVdbTexture::IsMinFilterSupported(GLenum filter)
{
    return true;
}

void
GlfVdbTexture::_ReadTexture()
{

#ifdef PXR_OPENVDB_SUPPORT_ENABLED

    GlfVdbTextureDataRefPtr const texData =
        GlfVdbTextureData::New(_filePath, GetMemoryRequested());

    if (texData) {
        texData->Read(0, _GenerateMipmap());
        _boundingBox = texData->GetBoundingBox();
    }

#else

    GlfBaseTextureDataRefPtr texData;

#endif

    _UpdateTexture(texData);
    _CreateTexture(texData, _GenerateMipmap());
    _SetLoaded();
}

bool
GlfVdbTexture::_GenerateMipmap() const
{
    return true;
}

const GfBBox3d &
GlfVdbTexture::GetBoundingBox()
{
    _ReadTextureIfNotLoaded();
    
    return _boundingBox;
}

PXR_NAMESPACE_CLOSE_SCOPE

