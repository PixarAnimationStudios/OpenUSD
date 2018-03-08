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
#ifndef PXRUSDMAYAGL_SHAPE_ADAPTER_H
#define PXRUSDMAYAGL_SHAPE_ADAPTER_H

/// \file pxrUsdMayaGL/shapeAdapter.h

#include "pxr/pxr.h"

#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/renderParams.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/usd/sdf/path.h"

// XXX: On Linux, some Maya headers (notably M3dView.h) end up indirectly
//      including X11/Xlib.h, which #define's "Bool" as int. This can cause
//      compilation issues if sdf/types.h is included afterwards, so to fix
//      this, we ensure that it gets included first.
#include "pxr/usd/sdf/types.h"

#include <maya/M3dView.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MPxSurfaceShape.h>

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


/// Abstract base class for objects that manage translation of Maya shape node
/// data and viewport state for imaging with Hydra.
class PxrMayaHdShapeAdapter
{
    public:

        /// Update the shape adapter's state from the given \c MPxSurfaceShape
        /// and the legacy viewport display state.
        PXRUSDMAYAGL_API
        virtual bool Sync(
                MPxSurfaceShape* surfaceShape,
                const M3dView::DisplayStyle legacyDisplayStyle,
                const M3dView::DisplayStatus legacyDisplayStatus);

        /// Update the shape adapter's state from the given \c MPxSurfaceShape
        /// and the Viewport 2.0 display state.
        PXRUSDMAYAGL_API
        virtual bool Sync(
                MPxSurfaceShape* surfaceShape,
                const unsigned int displayStyle,
                const MHWRender::DisplayStatus displayStatus);

        /// Update the shape adapter's visibility state from the display status
        /// of its shape.
        ///
        /// When a Maya shape is made invisible, it may no longer be included
        /// in the "prepare" phase of a viewport render (i.e. we get no
        /// getDrawRequests() or prepareForDraw() callbacks for that shape).
        /// This method can be called on demand to ensure that the shape
        /// adapter is updated with the current visibility state of the shape.
        ///
        /// Returns true if the visibility state was changed, or false
        /// otherwise.
        PXRUSDMAYAGL_API
        virtual bool UpdateVisibility();

        /// Get a set of render params from the shape adapter's current state.
        ///
        /// Sets \p drawShape and \p drawBoundingBox depending on whether shape
        /// and/or bounding box rendering is indicated from the state.
        PXRUSDMAYAGL_API
        virtual PxrMayaHdRenderParams GetRenderParams(
                bool* drawShape,
                bool* drawBoundingBox) const;

        PXRUSDMAYAGL_API
        virtual const HdRprimCollection& GetRprimCollection() const;

        PXRUSDMAYAGL_API
        virtual const GfMatrix4d& GetRootXform() const;

        PXRUSDMAYAGL_API
        virtual void SetRootXform(const GfMatrix4d& transform);

        PXRUSDMAYAGL_API
        virtual const SdfPath& GetDelegateID() const;

        /// Get whether this shape adapter is for use with Viewport 2.0.
        ///
        /// The shape adapter gets its viewport renderer affiliation from the
        /// version of Sync() that is used to populate it.
        ///
        /// Returns true if the shape adapter should be used for batched
        /// drawing/selection in Viewport 2.0, or false if it should be used
        /// in the legacy viewport.
        PXRUSDMAYAGL_API
        virtual bool IsViewport2() const;

    protected:

        /// Update the shape adapter's state from the given \c MPxSurfaceShape
        /// and display state.
        ///
        /// This method should be called by both public versions of Sync() and
        /// should perform shape data updates that are common to both the
        /// legacy viewport and Viewport 2.0. The legacy viewport Sync() method
        /// "promotes" the display state parameters to their Viewport 2.0
        /// equivalents before calling this method.
        PXRUSDMAYAGL_API
        virtual bool _Sync(
                MPxSurfaceShape* surfaceShape,
                const unsigned int displayStyle,
                const MHWRender::DisplayStatus displayStatus) = 0;

        /// Helper for getting the wireframe color of the shape.
        ///
        /// Determining the wireframe color may involve inspecting the soft
        /// selection, for which the batch renderer manages a helper. This
        /// class is made a friend of the batch renderer class so that it can
        /// access the soft selection info.
        static bool _GetWireframeColor(
                const MHWRender::DisplayStatus displayStatus,
                const MDagPath& shapeDagPath,
                MColor* mayaWireColor);

        /// Construct a new uninitialized PxrMayaHdShapeAdapter.
        PXRUSDMAYAGL_API
        PxrMayaHdShapeAdapter();

        PXRUSDMAYAGL_API
        virtual ~PxrMayaHdShapeAdapter();

        MDagPath _shapeDagPath;

        PxrMayaHdRenderParams _renderParams;
        bool _drawShape;
        bool _drawBoundingBox;

        HdRprimCollection _rprimCollection;

        GfMatrix4d _rootXform;

        bool _isViewport2;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYAGL_SHAPE_ADAPTER_H
