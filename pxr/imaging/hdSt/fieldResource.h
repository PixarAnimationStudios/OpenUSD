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
#ifndef PXR_IMAGING_HD_ST_FIELD_RESOURCE_H
#define PXR_IMAGING_HD_ST_FIELD_RESOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/textureResource.h"

#include "pxr/imaging/glf/vdbTexture.h"
#include "pxr/imaging/glf/textureHandle.h"

#include "pxr/base/gf/bbox3d.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdStFieldResource> HdStFieldResourceSharedPtr;

///
/// HdStFieldResource
///
/// Stores a 3d texture associated with a field. 
///
/// It wraps an HdStSimpleTextureResource and exists for two reasons:
/// 1. It also extracts the bounding box from the GlfVdbTexture.
/// 2. It constructs the OpenGL sampler and texture handle (if bindless)
///    upon creation and not lazily when GetTexels...() is called.
///    This has to do with the fact that the client of HdStFieldResource is
///    HdStVolume::Sync which is run concurrently with other rprim Sync's.
///
class HdStFieldResource : public HdStTextureResource
{
public:
    HdStFieldResource(const GlfTextureHandleRefPtr &);

    HDST_API
    ~HdStFieldResource() override;

    HDST_API HdTextureType GetTextureType() const override;
    HDST_API size_t GetMemoryUsed() override;

    HDST_API GLuint GetTexelsTextureId() override;
    HDST_API GLuint GetTexelsSamplerId() override;
    HDST_API uint64_t GetTexelsTextureHandle() override;
    HDST_API GLuint GetLayoutTextureId() override;
    HDST_API uint64_t GetLayoutTextureHandle() override;

    /// Returns the transform of the grid in the OpenVDB file as well as the
    /// bounding box of the samples in the corresponding OpenVDB tree.
    ///
    /// This pair of information is encoded as GfBBox3d.
    HDST_API const GfBBox3d &GetBoundingBox() {
        return _boundingBox;
    }

private:
    HdStSimpleTextureResource _simpleTextureResource;

    const GLuint _textureId;
    const GLuint _samplerId;
    const uint64_t _glTextureHandle;

    const GfBBox3d _boundingBox;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_FIELD_RESOURCE_H
