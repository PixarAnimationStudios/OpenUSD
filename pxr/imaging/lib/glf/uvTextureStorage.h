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
/// \file glf/uvTextureStorage.h

#ifndef GLF_UVTEXTURESTORAGE_H
#define GLF_UVTEXTURESTORAGE_H

#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/baseTexture.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/vt/value.h"

TF_DECLARE_WEAK_AND_REF_PTRS(GlfUVTextureStorage);

/// 
/// \class GlfUVTextureStorage uvTextureStorage.h "pxr/imaging/glf/uvTextureStorage.h"
/// \brief Represents a texture object in Glf initialized from a VtValue
///
/// A GlfUVTextureStorage is currently initialized from a float/double, GfVec3d, or GfVec4d.
///

class GlfUVTextureStorage : public GlfBaseTexture {
public:
    /// Creates a new texture instance based on input storageData
    /// \p width, and \p height specify the size
    ///
    GLF_API static GlfUVTextureStorageRefPtr New(
        unsigned int width,
        unsigned int height, 
        const VtValue &storageData);

protected:
    GlfUVTextureStorage(
        unsigned int width,
        unsigned int height, 
        const VtValue &storageData);

    virtual void _OnSetMemoryRequested(size_t targetMemory);
    virtual bool _GenerateMipmap() const;
    
private:
    unsigned int _width, _height; 
    VtValue _storageData; 


};

#endif // GLF_UVTEXTURESTORAGE_H
