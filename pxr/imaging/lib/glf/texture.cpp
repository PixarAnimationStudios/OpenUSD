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
/// \file texture.cpp
#include "pxr/imaging/glf/texture.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

#include <climits>

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<GlfTexture>();
}

TF_DEFINE_PUBLIC_TOKENS(GlfTextureTokens, GLF_TEXTURE_TOKENS);

static size_t _TextureMemoryAllocated=0;
static size_t _TextureContentsID=0;

static size_t
_GetNewContentsID()
{
    return ++_TextureContentsID;
}

GlfTexture::GlfTexture( )
    : _memoryUsed(0)
    , _memoryRequested(INT_MAX)
    , _contentsID(_GetNewContentsID())
{
}

GlfTexture::~GlfTexture( )
{
    _TextureMemoryAllocated-=_memoryUsed;
}

size_t
GlfTexture::GetMemoryRequested( ) const
{
    return _memoryRequested;
}

void
GlfTexture::SetMemoryRequested(size_t targetMemory)
{
    if (_memoryRequested != targetMemory) {
        _memoryRequested = targetMemory;
        _OnSetMemoryRequested(targetMemory);
    }
}

void
GlfTexture::_OnSetMemoryRequested(size_t targetMemory)
{
    // do nothing in base class
}

size_t 
GlfTexture::GetMemoryUsed( ) const 
{
    return _memoryUsed;
}

void
GlfTexture::_SetMemoryUsed( size_t s ) 
{
    _TextureMemoryAllocated += s - _memoryUsed;

    _memoryUsed = s;        
}

bool
GlfTexture::IsMinFilterSupported(GLenum filter)
{
    return true;
}

bool
GlfTexture::IsMagFilterSupported(GLenum filter)
{
    return true;
}

size_t 
GlfTexture::GetTextureMemoryAllocated()
{
    return _TextureMemoryAllocated;
}

size_t
GlfTexture::GetContentsID() const
{
    return _contentsID;
}

void
GlfTexture::_UpdateContentsID()
{
    _contentsID = _GetNewContentsID();
}
