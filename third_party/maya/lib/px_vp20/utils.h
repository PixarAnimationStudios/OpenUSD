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
#ifndef PXVP20_UTILS_H
#define PXVP20_UTILS_H

/// \file px_vp20/utils.h

#include "pxr/pxr.h"
#include "px_vp20/api.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

#include <maya/M3dView.h>
#include <maya/MBoundingBox.h>
#include <maya/MDrawContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MMatrix.h>
#include <maya/MSelectionContext.h>

#include <ostream>


PXR_NAMESPACE_OPEN_SCOPE


class px_vp20Utils
{
    public:
        /// Take VP2.0 lighting information and import it into opengl lights
        PX_VP20_API
        static bool setupLightingGL(const MHWRender::MDrawContext& context);
        PX_VP20_API
        static void unsetLightingGL(const MHWRender::MDrawContext& context);

        /// Translate a Maya MDrawContext into a GlfSimpleLightingContext.
        PX_VP20_API
        static GlfSimpleLightingContextRefPtr GetLightingContextFromDrawContext(
                const MHWRender::MDrawContext& context);

        /// Tries to get the viewport for the given draw context.
        ///
        /// Returns true if the viewport was found, in which case it is
        /// returned in the \p view parameter.
        /// Returns false if there's not a 3D viewport (e.g. we're drawing into
        /// a render view).
        PX_VP20_API
        static bool GetViewFromDrawContext(
                const MHWRender::MDrawContext& context,
                M3dView& view);

        /// Renders the given bounding box in the given \p color via OpenGL.
        PX_VP20_API
        static bool RenderBoundingBox(
                const MBoundingBox& bounds,
                const GfVec4f& color,
                const MMatrix& worldViewMat,
                const MMatrix& projectionMat);

        /// Helper to draw multiple wireframe boxes, where \p cubeXforms is a
        /// list of transforms to apply to the unit cube centered around the
        /// origin.  Those transforms will all be concatenated with the 
        /// \p worldViewMat and \p projectionMat.
        PX_VP20_API
        static bool RenderWireCubes(
                const std::vector<GfMatrix4f>& cubeXforms,
                const GfVec4f& color,
                const GfMatrix4d& worldViewMat,
                const GfMatrix4d& projectionMat);

        /// Gets the view and projection matrices based on a particular
        /// selection in the given draw context.
        PX_VP20_API
        static bool GetSelectionMatrices(
                const MHWRender::MSelectionInfo& selectionInfo,
                const MHWRender::MDrawContext& context,
                GfMatrix4d& viewMatrix,
                GfMatrix4d& projectionMatrix);

        /// Outputs a human-readable form of the given \p displayStyle to
        /// \p stream for debugging.
        ///
        /// \p displayStyle should be a bitwise combination of
        /// MHWRender::MFrameContext::DisplayStyle values.
        PX_VP20_API
        static void OutputDisplayStyleToStream(
                const unsigned int displayStyle,
                std::ostream& stream);

        /// Outputs a human-readable form of the given \p displayStatus to
        /// \p stream for debugging.
        PX_VP20_API
        static void OutputDisplayStatusToStream(
                const MHWRender::DisplayStatus displayStatus,
                std::ostream& stream);

    private:
        px_vp20Utils() = delete;
        ~px_vp20Utils() = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
