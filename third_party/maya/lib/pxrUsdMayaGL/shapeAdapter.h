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

/// \file shapeAdapter.h

#include "pxr/pxr.h"

#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/renderParams.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/base/tf/debug.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usdImaging/usdImaging/delegate.h"

#include <maya/M3dView.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MPxSurfaceShape.h>

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEBUG_CODES(
    PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE
);


class UsdMayaProxyDrawOverride;
class UsdMayaProxyShapeUI;


/// Class to manage translation of Maya shape node data and viewport state for
/// imaging with Hydra.
class PxrMayaHdShapeAdapter
{
    public:

        /// Update the shape adapter's state from the given \c MPxSurfaceShape
        /// and the legacy viewport display state.
        PXRUSDMAYAGL_API
        bool Sync(
                MPxSurfaceShape* surfaceShape,
                const M3dView::DisplayStyle legacyDisplayStyle,
                const M3dView::DisplayStatus legacyDisplayStatus);

        /// Update the shape adapter's state from the given \c MPxSurfaceShape
        /// and the Viewport 2.0 display state.
        PXRUSDMAYAGL_API
        bool Sync(
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
        bool UpdateVisibility();

        /// Get a set of render params from the shape adapter's current state.
        ///
        /// Sets \p drawShape and \p drawBoundingBox depending on whether shape
        /// and/or bounding box rendering is indicated from the state.
        PXRUSDMAYAGL_API
        PxrMayaHdRenderParams GetRenderParams(
                bool* drawShape,
                bool* drawBoundingBox) const;

        PXRUSDMAYAGL_API
        const HdRprimCollection& GetRprimCollection() const {
            return _rprimCollection;
        }

        PXRUSDMAYAGL_API
        const GfMatrix4d& GetRootXform() const { return _rootXform; }

        PXRUSDMAYAGL_API
        const SdfPath& GetDelegateID() const;

        /// Get whether this shape adapter is for use with Viewport 2.0.
        ///
        /// The shape adapter gets its viewport renderer affiliation from the
        /// version of Sync() that is used to populate it.
        ///
        /// Returns true if the shape adapter should be used for batched
        /// drawing/selection in Viewport 2.0, or false if it should be used
        /// in the legacy viewport.
        PXRUSDMAYAGL_API
        bool IsViewport2() const;

    private:

        /// Construct a new uninitialized PxrMayaHdShapeAdapter.
        ///
        /// Note that only friends of this class are able to construct
        /// instances of this class.
        PXRUSDMAYAGL_API
        PxrMayaHdShapeAdapter();

        PXRUSDMAYAGL_API
        virtual ~PxrMayaHdShapeAdapter();

        /// Initialize the shape adapter using the given \p renderIndex.
        ///
        /// This method is called automatically during Sync() when the shape
        /// adapter's "identity" changes. This happens when the delegateId or
        /// the rprim collection name computed from the shape adapter's shape
        /// is different than what is currently stored in the shape adapter.
        /// The shape adapter will then query the batch renderer for its render
        /// index and use that to re-create its delegate and re-add its rprim
        /// collection, if necessary.
        PXRUSDMAYAGL_API
        bool _Init(HdRenderIndex* renderIndex);

        /// Private helper for getting the wireframe color of the shape.
        ///
        /// Determining the wireframe color may involve inspecting the soft
        /// selection, for which the batch renderer manages a helper. This
        /// class is made a friend of the batch renderer class so that it can
        /// access the soft selection info.
        static bool _GetWireframeColor(
                const MHWRender::DisplayStatus displayStatus,
                const MDagPath& shapeDagPath,
                MColor* mayaWireColor);

        MDagPath _shapeDagPath;
        UsdPrim _rootPrim;
        SdfPathVector _excludedPrimPaths;
        GfMatrix4d _rootXform;

        std::shared_ptr<UsdImagingDelegate> _delegate;

        HdRprimCollection _rprimCollection;

        PxrMayaHdRenderParams _renderParams;
        bool _drawShape;
        bool _drawBoundingBox;

        bool _isViewport2;

        /// The classes that maintain ownership of and are responsible for
        /// updating the shape adapter for their shape are made friends of
        /// PxrMayaHdShapeAdapter so that they alone can set properties on the
        /// shape adapter.
        ///
        /// XXX: Eventually, each type of shape that is imaged by Hydra will
        /// have its own derived class of PxrMayaHdShapeAdapter so that we do
        /// not end up with a single master list here.
        friend class UsdMayaProxyDrawOverride;
        friend class UsdMayaProxyShapeUI;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYAGL_SHAPE_ADAPTER_H
