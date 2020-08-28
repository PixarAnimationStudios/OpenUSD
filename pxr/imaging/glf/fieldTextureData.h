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
#ifndef PXR_IMAGING_GLF_FIELD_TEXTURE_DATA_H
#define PXR_IMAGING_GLF_FIELD_TEXTURE_DATA_H

/// \file glf/fieldTextureData.h

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/image.h"
#include "pxr/imaging/glf/baseTextureData.h"

#include "pxr/base/gf/bbox3d.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(GlfFieldTextureData);

/// \class GlfFieldTextureData
///
/// An interface class for reading volume files having a
/// transformation.
///
class GlfFieldTextureData : public GlfBaseTextureData
{
public:
    using Base = GlfBaseTextureData;

    /// Bounding box describing how 3d texture maps into
    /// world space.
    ///
    virtual const GfBBox3d &GetBoundingBox() const = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
