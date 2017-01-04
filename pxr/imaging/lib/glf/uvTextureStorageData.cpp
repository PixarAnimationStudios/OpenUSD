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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/uvTextureStorageData.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4d.h" 
#include "pxr/base/tf/diagnostic.h"

#include <vector>

TF_DECLARE_WEAK_AND_REF_PTRS(GlfUVTextureStorageData);

GlfUVTextureStorageDataRefPtr
GlfUVTextureStorageData::New(
    unsigned int width,
    unsigned int height, 
    const VtValue &storageData)
{
    return TfCreateRefPtr(new GlfUVTextureStorageData(
        width, height, storageData));
}

GlfUVTextureStorageData::~GlfUVTextureStorageData()
{
    if (_rawBuffer) {
        delete [] _rawBuffer;
        _rawBuffer = nullptr; 
    }
}

size_t GlfUVTextureStorageData::ComputeBytesUsed() const
{
    if (_rawBuffer) {
        return _resizedWidth * _resizedHeight * _bytesPerPixel;
    } else {
        return 0;
    }
}

bool GlfUVTextureStorageData::HasRawBuffer(int mipLevel) const
{
    return (_rawBuffer != nullptr);
}

unsigned char * GlfUVTextureStorageData::GetRawBuffer(int mipLevel) const
{
    return _rawBuffer;
}

bool GlfUVTextureStorageData::Read(int degradeLevel, bool generateMipmap) {
    _targetMemory = _size;
    std::vector<float> storageArray;
    if (_storageData.IsHolding<float>()) {
        storageArray.resize(1); 
        storageArray[0] = _storageData.Get<float>();
        _glInternalFormat = GL_RED;
        _glFormat = GL_RED; 
    } else if (_storageData.IsHolding<double>()) { 
        storageArray.resize(1); 
        storageArray[0] = _storageData.Get<double>();
        _glInternalFormat = GL_RED;
        _glFormat = GL_RED; 
    } else if (_storageData.IsHolding<GfVec3d>()) {
        GfVec3d storageVec3d = _storageData.Get<GfVec3d>();
        storageArray.resize(3); 
        storageArray[0] = storageVec3d[0]; 
        storageArray[1] = storageVec3d[1];
        storageArray[2] = storageVec3d[2]; 
        _glInternalFormat = GL_RGB;
        _glFormat = GL_RGB; 
    } else if (_storageData.IsHolding<GfVec4d>()) {
        GfVec4d storageVec4d = _storageData.Get<GfVec4d>();
        storageArray.resize(4); 
        storageArray[0] = storageVec4d[0]; 
        storageArray[1] = storageVec4d[1];
        storageArray[2] = storageVec4d[2]; 
        storageArray[3] = storageVec4d[3];
        _glInternalFormat = GL_RGBA;
        _glFormat = GL_RGBA; 
    } else {
        TF_CODING_ERROR("Unsupported texture storage data type");
        return false; 
    }
    _bytesPerPixel = GlfGetNumElements(_glFormat) * 
                         GlfGetElementSize(_glType);

    _size = _resizedWidth * _resizedHeight * _bytesPerPixel;

    if (_rawBuffer) {
        delete [] _rawBuffer;
        _rawBuffer = nullptr;
    }    

    _rawBuffer = new unsigned char[_size];

    // Transfer value from Storage Data into raw buffer
    for (int y=0; y < _resizedHeight; ++y) {
        for (int x=0; x < _resizedWidth; ++x) {
            int destIndex = (y * _resizedWidth + x) * _bytesPerPixel; 
            memcpy(&_rawBuffer[destIndex], &storageArray[0], _bytesPerPixel); 
        }
    }

    return true; 
}

int GlfUVTextureStorageData::GetNumMipLevels() const 
{
    if (_rawBuffer) return 1;
    return 0;
}

bool GlfUVTextureStorageData::IsCompressed() const {
    return false; 
}

GlfUVTextureStorageData::GlfUVTextureStorageData(
    unsigned int width,
    unsigned int height, 
    const VtValue &storageData)
    : _targetMemory(0)
    , _resizedWidth(width)
    , _resizedHeight(height)
    , _storageData(storageData)
    , _glInternalFormat(GL_RGB)
    , _glFormat(GL_RGB)
    , _glType(GL_FLOAT)
    , _size(0)
    , _rawBuffer(nullptr)
{
    /* nothing */
}
