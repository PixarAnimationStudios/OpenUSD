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
#ifndef PXR_IMAGING_GLF_FIELD_TEXTURE_H
#define PXR_IMAGING_GLF_FIELD_TEXTURE_H

/// \file glf/fieldTexture.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/baseTexture.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(GlfFieldTexture);

/// \class GlfFieldTexture
///
/// Represents an abstract 3-dimensional texture read from a volume field
///
class GlfFieldTexture : public GlfBaseTexture {
public:
    /// Returns the transform and bouding box of the field.
    ///
    /// This pair of information is encoded as GfBBox3d.
    GLF_API
    virtual const GfBBox3d &GetBoundingBox();

    int GetNumDimensions() const override;

protected:

    GLF_API
    virtual void _ReadTexture() = 0;

    GfBBox3d _boundingBox;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_GLF_FIELD_TEXTURE_H
