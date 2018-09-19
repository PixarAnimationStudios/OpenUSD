//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYAGL_HD_IMAGING_SHAPE_UI_H
#define PXRUSDMAYAGL_HD_IMAGING_SHAPE_UI_H

/// \file pxrUsdMayaGL/hdImagingShapeUI.h

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"

// XXX: On Linux, some Maya headers (notably M3dView.h) end up indirectly
//      including X11/Xlib.h, which #define's "Bool" as int. This can cause
//      compilation issues if sdf/types.h is included afterwards, so to fix
//      this, we ensure that it gets included first.
#include "pxr/usd/sdf/types.h"

#include <maya/M3dView.h>
#include <maya/MDrawInfo.h>
#include <maya/MDrawRequest.h>
#include <maya/MDrawRequestQueue.h>
#include <maya/MPxSurfaceShapeUI.h>


PXR_NAMESPACE_OPEN_SCOPE


/// Class for drawing the pxrHdImagingShape node in the legacy viewport
///
/// In most cases, there will only be a single instance of the
/// pxrHdImagingShape node in the scene, so this class will be the thing that
/// invokes the batch renderer to draw all Hydra-imaged Maya nodes.
///
/// Note that it does not support selection, so the individual nodes are still
/// responsible for managing that.
class PxrMayaHdImagingShapeUI : public MPxSurfaceShapeUI
{
    public:

        PXRUSDMAYAGL_API
        static void* creator();

        PXRUSDMAYAGL_API
        void getDrawRequests(
                const MDrawInfo& drawInfo,
                bool objectAndActiveOnly,
                MDrawRequestQueue& requests) override;

        PXRUSDMAYAGL_API
        void draw(const MDrawRequest& request, M3dView& view) const override;

    private:

        PxrMayaHdImagingShapeUI();
        ~PxrMayaHdImagingShapeUI() override;

        PxrMayaHdImagingShapeUI(const PxrMayaHdImagingShapeUI&) = delete;
        PxrMayaHdImagingShapeUI& operator=(
                const PxrMayaHdImagingShapeUI&) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
