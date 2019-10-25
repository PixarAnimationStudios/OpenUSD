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
#include "pxrUsdMayaGL/userData.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/usd/sdf/path.h"

// XXX: On Linux, some Maya headers (notably M3dView.h) end up indirectly
//      including X11/Xlib.h, which #define's "Bool" as int. This can cause
//      compilation issues if sdf/types.h is included afterwards, so to fix
//      this, we ensure that it gets included first.
#include "pxr/usd/sdf/types.h"

#include <maya/M3dView.h>
#include <maya/MBoundingBox.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MDrawRequest.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MUserData.h>

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


/// Abstract base class for objects that manage translation of Maya shape node
/// data and viewport state for imaging with Hydra.
class PxrMayaHdShapeAdapter
{
    public:

        /// Update the shape adapter's state from the shape with the given
        /// \p shapeDagPath and the legacy viewport display state.
        PXRUSDMAYAGL_API
        virtual bool Sync(
                const MDagPath& shapeDagPath,
                const M3dView::DisplayStyle legacyDisplayStyle,
                const M3dView::DisplayStatus legacyDisplayStatus);

        /// Update the shape adapter's state from the shape with the given
        /// \p shapeDagPath and the Viewport 2.0 display state.
        PXRUSDMAYAGL_API
        virtual bool Sync(
                const MDagPath& shapeDagPath,
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
        /// The optional \p view parameter can be passed to have view-based
        /// state such as view and/or plugin object filtering factor into the
        /// shape's visibility.
        ///
        /// Returns true if the visibility state was changed, or false
        /// otherwise.
        PXRUSDMAYAGL_API
        virtual bool UpdateVisibility(const M3dView* view = nullptr);

        /// Gets whether the shape adapter's shape is visible.
        ///
        /// This should be called after a call to UpdateVisibility() to ensure
        /// that the returned value is correct.
        PXRUSDMAYAGL_API
        virtual bool IsVisible() const;

        /// Get the Maya user data object for drawing in the legacy viewport.
        ///
        /// This Maya user data is attached to the given \p drawRequest. Its
        /// lifetime is *not* managed by Maya, so the batch renderer deletes it
        /// manually at the end of a legacy viewport Draw().
        ///
        /// \p boundingBox may be set to nullptr if no box is desired to be
        /// drawn.
        ///
        PXRUSDMAYAGL_API
        virtual void GetMayaUserData(
                MPxSurfaceShapeUI* shapeUI,
                MDrawRequest& drawRequest,
                const MBoundingBox* boundingBox = nullptr);

        /// Get the Maya user data object for drawing in Viewport 2.0.
        ///
        /// \p oldData should be the same \p oldData parameter that Maya passed
        /// into the calling prepareForDraw() method. The return value from
        /// this method should then be returned back to Maya in the calling
        /// prepareForDraw().
        ///
        /// Note that this version of GetMayaUserData() is also invoked by the
        /// legacy viewport version, in which case we expect oldData to be
        /// nullptr.
        ///
        /// \p boundingBox may be set to nullptr if no box is desired to be
        /// drawn.
        ///
        /// Returns a pointer to a new PxrMayaHdUserData object populated with
        /// the given parameters if oldData is nullptr (or not an instance of
        /// PxrMayaHdUserData), otherwise returns oldData after having
        /// re-populated it.
        PXRUSDMAYAGL_API
        virtual PxrMayaHdUserData* GetMayaUserData(
                MUserData* oldData,
                const MBoundingBox* boundingBox = nullptr);

        /// Gets the HdReprSelector that corresponds to the given Maya display
        /// state.
        ///
        /// \p displayStyle should be a bitwise combination of
        /// MHWRender::MFrameContext::DisplayStyle values, typically either
        /// up-converted from a single M3dView::DisplayStyle value obtained
        /// using MDrawInfo::displayStyle() for the legacy viewport, or
        /// obtained using MHWRender::MFrameContext::getDisplayStyle() for
        /// Viewport 2.0.
        ///
        /// \p displayStatus is typically either up-converted from
        /// a M3dView::DisplayStatus value obtained using
        /// MDrawInfo::displayStatus() for the legacy viewport, or obtained
        /// using MHWRender::MGeometryUtilities::displayStatus() for Viewport
        /// 2.0.
        ///
        /// If there is no corresponding HdReprSelector for the given display
        /// state, an empty HdReprSelector is returned.
        PXRUSDMAYAGL_API
        virtual HdReprSelector GetReprSelectorForDisplayState(
                const unsigned int displayStyle,
                const MHWRender::DisplayStatus displayStatus) const;

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

        /// Retrieves the render tags for this shape.  I.e. which
        /// prim purposes should be drawn (such as geomerty, proxy, guides
        /// and/or render).
        /// This function just returns the _renderTags attribute and it
        /// is expected each subclass update the attribute in _Sync() or
        /// overrides this function if it needs special processing.
        PXRUSDMAYAGL_API
        virtual const TfTokenVector& GetRenderTags() const;


        PXRUSDMAYAGL_API
        virtual const GfMatrix4d& GetRootXform() const;

        PXRUSDMAYAGL_API
        virtual void SetRootXform(const GfMatrix4d& transform);

        PXRUSDMAYAGL_API
        virtual const SdfPath& GetDelegateID() const;

        PXRUSDMAYAGL_API
        virtual const MDagPath& GetDagPath() const;

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

        /// Update the shape adapter's state from the shape with the given
        /// \p dagPath and display state.
        ///
        /// This method should be called by both public versions of Sync() and
        /// should perform shape data updates that are common to both the
        /// legacy viewport and Viewport 2.0. The legacy viewport Sync() method
        /// "promotes" the display state parameters to their Viewport 2.0
        /// equivalents before calling this method.
        PXRUSDMAYAGL_API
        virtual bool _Sync(
                const MDagPath& shapeDagPath,
                const unsigned int displayStyle,
                const MHWRender::DisplayStatus displayStatus) = 0;

        /// Helper for computing the name of the shape's HdRprimCollection.
        ///
        /// The batch renderer currently creates a render task for each shape's
        /// HdRprimCollection, and those render tasks are identified by an
        /// SdfPath constructed using the collection's name. We therefore need
        /// the collection to have a name that is unique to the shape it
        /// represents and also sanitized for use in SdfPaths.
        ///
        /// Returns a TfToken collection name that is unique to the shape and
        /// is a valid SdfPath identifier, or an empty TfToken is there is an
        /// error.
        PXRUSDMAYAGL_API
        virtual TfToken _GetRprimCollectionName() const;

        /// Helper for getting the wireframe color of the shape.
        ///
        /// Determining the wireframe color may involve inspecting the soft
        /// selection, for which the batch renderer manages a helper. This
        /// class is made a friend of the batch renderer class so that it can
        /// access the soft selection info.
        ///
        /// Returns true if the wireframe color should be used, that is if the
        /// object and/or its component(s) are involved in a selection, or if
        /// the displayStyle indicates that a wireframe style is being drawn
        /// (either kWireFrame or kBoundingBox). Otherwise returns false.
        ///
        /// The wireframe color will always be returned in \p mayaWireColor (if
        /// it is not nullptr) in case the caller wants to use other criteria
        /// for determining whether to use it.
        static bool _GetWireframeColor(
                const unsigned int displayStyle,
                const MHWRender::DisplayStatus displayStatus,
                const MDagPath& shapeDagPath,
                MColor* mayaWireColor);

        /// Helper for computing the viewport visibility of the shape.
        ///
        /// Takes into account the visibility attribute on the shape and its
        /// DAG ancestors. If an M3dView is provided to \p view, then
        /// view-based state such as view and/or plugin object filtering will
        /// also be factored into the shape's visibility.
        ///
        /// Returns true if computing the visibility was successful, false if
        /// there was an error. The visibility is returned in \p visibility.
        static bool _GetVisibility(
                const MDagPath& dagPath,
                const M3dView* view,
                bool* visibility);

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
        TfTokenVector     _renderTags;

        GfMatrix4d _rootXform;

        bool _isViewport2;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
