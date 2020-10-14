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
#ifndef PXR_IMAGING_GLF_FIELD3D_TEXTURE_DATA_BASE_H
#define PXR_IMAGING_GLF_FIELD3D_TEXTURE_DATA_BASE_H

/// \file glf/field3DTextureDataBase.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/fieldTextureData.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(GlfField3DTextureDataBase);

/// \class GlfField3DTextureDataBase
///
/// Interface class to load a Field3D file. Clients can use
/// it to load a Field3D file if there is a plugin providing
/// a subclass implementing loading Field3D files.
///
class GlfField3DTextureDataBase : public GlfFieldTextureData
{
public:
    /// Load Field3D file
    ///
    /// fieldName corresponds to the layer/attribute name in the Field3D file
    /// fieldIndex corresponds to the partition index
    /// fieldPurpose corresponds to the partition name/grouping
    ///
    GLF_API
    static GlfField3DTextureDataBaseRefPtr New(
        std::string const &filePath,
        std::string const &fieldName,
        int fieldIndex,
        std::string const &fieldPurpose,
        size_t targetMemory);
};

/// \class GlfField3DTextureDataFactoryBase
///
/// A base class to make GlfField3DTextureDataBase objects.
/// The Field3D loader plugin has to subclass from it as well.
///
class GlfField3DTextureDataFactoryBase : public TfType::FactoryBase
{
protected:
    friend class GlfField3DTextureDataBase;

    virtual GlfField3DTextureDataBaseRefPtr _New(
        std::string const &filePath,
        std::string const &fieldName,
        int fieldIndex,
        std::string const &fieldPurpose,
        size_t targetMemory) = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
