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
#ifndef PXR_IMAGING_HD_ST_VOLUME_H
#define PXR_IMAGING_HD_ST_VOLUME_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/volume.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStDrawItem;

/// Represents a Volume Prim.
///
class HdStVolume final : public HdVolume {
public:
    HDST_API
    HdStVolume(SdfPath const& id);
    HDST_API
    ~HdStVolume() override;

    HDST_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    HDST_API
    void Sync(HdSceneDelegate* delegate,
              HdRenderParam*   renderParam,
              HdDirtyBits*     dirtyBits,
              TfToken const  &reprToken) override;

    HDST_API
    void Finalize(HdRenderParam *renderParam) override;

    /// Default step size used for raymarching
    static const float defaultStepSize;

    /// Default step size used for raymarching for lighting computation
    static const float defaultStepSizeLighting;

    /// Default memory limit for a field texture (in Mb) if not
    /// overridden by field prim with textureMemory.
    static const float defaultMaxTextureMemoryPerField;

protected:
    void _InitRepr(TfToken const &reprToken,
                   HdDirtyBits* dirtyBits) override;

    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    void _UpdateRepr(HdSceneDelegate *sceneDelegate,
                     HdRenderParam *renderParam,
                     TfToken const &reprToken,
                     HdDirtyBits *dirtyBitsState);

private:
    void _UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                         HdRenderParam *renderParam,
                         HdStDrawItem *drawItem,
                         HdDirtyBits *dirtyBits);

    HdReprSharedPtr _volumeRepr;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_VOLUME_H
