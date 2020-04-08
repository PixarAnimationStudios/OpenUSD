//
// Copyright 2020 Pixar
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

#include "pxr/imaging/hdSt/textureObject.h"

#include "pxr/imaging/hdSt/textureObjectRegistry.h"
#include "pxr/imaging/hdSt/subtextureIdentifier.h"
#include "pxr/imaging/hdSt/textureIdentifier.h"

#include "pxr/imaging/glf/uvTextureData.h"

#include "pxr/imaging/hgi/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
// HdStTextureObject

HdStTextureObject::HdStTextureObject(
    const HdStTextureIdentifier &textureId,
    HdSt_TextureObjectRegistry * const textureObjectRegistry)
  : _textureObjectRegistry(textureObjectRegistry)
  , _textureId(textureId)
  , _targetMemory(0)
{
}

void
HdStTextureObject::SetTargetMemory(const size_t targetMemory)
{
    if (_targetMemory == targetMemory) {
        return;
    }
    _targetMemory = targetMemory;
    _textureObjectRegistry->MarkTextureObjectDirty(shared_from_this());
}

Hgi *
HdStTextureObject::_GetHgi() const
{
    if (!TF_VERIFY(_textureObjectRegistry)) {
        return nullptr;
    }

    Hgi * const hgi = _textureObjectRegistry->GetHgi();
    TF_VERIFY(hgi);

    return hgi;
}

HdStTextureObject::~HdStTextureObject() = default;

///////////////////////////////////////////////////////////////////////////////
// Helpers

static
std::string
_GetDebugName(const HdStTextureIdentifier &textureId)
{
    return textureId.GetFilePath().GetString();
}

// Should this be in HgiGL?
static
HgiFormat
_GetFormat(const GLenum format, const GLenum type)
{
    switch(format) {
    case GL_RED:
        switch(type) {
        case GL_FLOAT:
            return HgiFormatFloat32;
        case GL_UNSIGNED_BYTE:
            return HgiFormatUNorm8;
        default:
            break;
        }
        break;
    case GL_RG:
        switch(type) {
        case GL_FLOAT:
            return HgiFormatFloat32Vec2;
        case GL_UNSIGNED_BYTE:
            return HgiFormatUNorm8Vec2;
        default:
            break;
        }
        break;
    case GL_RGB:
        switch(type) {
        case GL_FLOAT:
            return HgiFormatFloat32Vec3;
        case GL_UNSIGNED_BYTE:
            return HgiFormatUNorm8Vec3;
        default:
            break;
        }
        break;
    case GL_RGBA:
        switch(type) {
        case GL_FLOAT:
            return HgiFormatFloat32Vec4;
        case GL_UNSIGNED_BYTE:
            return HgiFormatUNorm8Vec4;
        default:
            break;
        }
        break;
    default:
        break;
    }

    TF_CODING_ERROR("Unsupported texture format %d %d",
                    format, type);

    return HgiFormatInvalid;
}

static
HgiTextureType
_GetTextureType(int numDimensions)
{
    switch(numDimensions) {
    case 2:
        return HgiTextureType2D;
    case 3:
        return HgiTextureType3D;
    default:
        TF_CODING_ERROR("Unsupported number of dimensions");
        return HgiTextureType2D;
    }
}


static
HgiTextureDesc
_GetTextureDesc(GlfBaseTextureDataRefPtr const &cpuData,
                const std::string &debugName)
{
    HgiTextureDesc textureDesc;
    textureDesc.debugName = debugName;
    textureDesc.format = _GetFormat(cpuData->GLFormat(), cpuData->GLType());
    textureDesc.pixelsByteSize = HgiDataSizeOfFormat(textureDesc.format);

    const GfVec3i dimensions(cpuData->ResizedWidth(),
                             cpuData->ResizedHeight(),
                             cpuData->ResizedDepth());

    if (dimensions[0] * dimensions[1] * dimensions[2] > 0) {
        textureDesc.dimensions = dimensions;
        textureDesc.initialData = cpuData->GetRawBuffer();
    } else {
        textureDesc.dimensions = GfVec3i(1,1,1);

        static char zeros[256];
        textureDesc.initialData = zeros;
    }

    textureDesc.type = _GetTextureType(cpuData->NumDimensions());
    
    return textureDesc;
}

///////////////////////////////////////////////////////////////////////////////
// Uv texture

HdStUvTextureObject::HdStUvTextureObject(
    const HdStTextureIdentifier &textureId,
    HdSt_TextureObjectRegistry * const textureObjectRegistry)
  : HdStTextureObject(textureId, textureObjectRegistry)
{
}

HdStUvTextureObject::~HdStUvTextureObject()
{
    if (Hgi * hgi = _GetHgi()) {
        hgi->DestroyTexture(&_gpuTexture);
    }
}

void
HdStUvTextureObject::_Load()
{
    _cpuData = GlfUVTextureData::New(
        GetTextureIdentifier().GetFilePath(),
        GetTargetMemory(),
        /* borders */ 0, 0, 0, 0);

    if (_cpuData) {
        _cpuData->Read(0, false);
    }
}

void
HdStUvTextureObject::_Commit()
{
    Hgi * const hgi = _GetHgi();
    if (!hgi) {
        return;
    }

    // Free previously allocated texture
    hgi->DestroyTexture(&_gpuTexture);

    if (!_cpuData) {
        return;
    }

    const HgiTextureDesc textureDesc =
        _GetTextureDesc(
            _cpuData,
            _GetDebugName(GetTextureIdentifier()));

    if (textureDesc.type != HgiTextureType2D) {
        TF_CODING_ERROR("Wrong texture type for uv");
    }

    // Upload to GPU
    _gpuTexture = hgi->CreateTexture(textureDesc);

    // Free CPU memory after transfer to GPU
    _cpuData = TfNullPtr;
}

HdTextureType
HdStUvTextureObject::GetTextureType() const
{
    return HdTextureType::Uv;
}

PXR_NAMESPACE_CLOSE_SCOPE
