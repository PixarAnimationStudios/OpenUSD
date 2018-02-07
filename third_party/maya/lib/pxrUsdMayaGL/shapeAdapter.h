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

#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usdImaging/usdImaging/delegate.h"

#include <maya/M3dView.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MHWGeometryUtilities.h>

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


/// Class to manage translation of Maya shape node data and viewport state for
/// imaging with Hydra.
class PxrMayaHdShapeAdapter
{
    public:

        /// Construct a new uninitialized \c PxrMayaHdShapeAdapter.
        PXRUSDMAYAGL_API
        PxrMayaHdShapeAdapter(
                const MDagPath& shapeDagPath,
                const UsdPrim& rootPrim,
                const SdfPathVector& excludedPrimPaths);

        PXRUSDMAYAGL_API
        size_t GetHash() const;

        PXRUSDMAYAGL_API
        void Init(HdRenderIndex* renderIndex);

        /// Prepare the shape adapter for being added to the batch renderer's
        /// queues.
        PXRUSDMAYAGL_API
        void PrepareForQueue(
                const UsdTimeCode time,
                const uint8_t refineLevel,
                const bool showGuides,
                const bool showRenderGuides,
                const bool tint,
                const GfVec4f& tintColor);

        /// Get a set of render params from the legacy viewport's display state.
        ///
        /// Sets \p drawShape and \p drawBoundingBox depending on whether shape
        /// and/or bounding box rendering is indicated from the state.
        PXRUSDMAYAGL_API
        PxrMayaHdRenderParams GetRenderParams(
                const M3dView::DisplayStyle displayStyle,
                const M3dView::DisplayStatus displayStatus,
                bool* drawShape,
                bool* drawBoundingBox);

        /// Get a set of render params from the Viewport 2.0 display state.
        ///
        /// Sets \p drawShape and \p drawBoundingBox depending on whether shape
        /// and/or bounding box rendering is indicated from the state.
        PXRUSDMAYAGL_API
        PxrMayaHdRenderParams GetRenderParams(
                const unsigned int displayStyle,
                const MHWRender::DisplayStatus displayStatus,
                bool* drawShape,
                bool* drawBoundingBox);

        PXRUSDMAYAGL_API
        const HdRprimCollection& GetRprimCollection() const {
            return _rprimCollection;
        }

        PXRUSDMAYAGL_API
        const GfMatrix4d& GetRootXform() const { return _rootXform; }

        /// Returns base params as set previously by \c PrepareForQueue().
        PXRUSDMAYAGL_API
        const PxrMayaHdRenderParams& GetBaseParams() const {
            return _baseParams;
        }

        PXRUSDMAYAGL_API
        const SdfPath& GetSharedId() const { return _sharedId; }

    private:

        /// Private helper for getting the wireframe color of the shape.
        ///
        /// Determining the wireframe color may involve inspecting the soft
        /// selection, for which the batch renderer manages a helper. This
        /// class is made a friend of the batch renderer class so that it can
        /// access the soft selection info.
        bool _GetWireframeColor(
                const MHWRender::DisplayStatus displayStatus,
                MColor* mayaWireColor);

        MDagPath _shapeDagPath;
        UsdPrim _rootPrim;
        SdfPathVector _excludedPrimPaths;
        GfMatrix4d _rootXform;

        PxrMayaHdRenderParams _baseParams;

        SdfPath _sharedId;
        bool _isPopulated;
        std::shared_ptr<UsdImagingDelegate> _delegate;

        HdRprimCollection _rprimCollection;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYAGL_SHAPE_ADAPTER_H
