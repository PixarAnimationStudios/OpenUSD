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
#ifndef PXRUSDMAYAGL_INSTANCER_SHAPE_ADAPTER_H
#define PXRUSDMAYAGL_INSTANCER_SHAPE_ADAPTER_H

/// \file pxrUsdMayaGL/instancerShapeAdapter.h

#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/shapeAdapter.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usdImaging/usdImaging/delegate.h"

#include <maya/M3dView.h>

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


/// Class to manage translation of native Maya instancers into
/// UsdGeomPointInstancers for imaging with Hydra.
/// This adapter will translate instancer prototypes that are USD reference
/// assemblies into UsdGeomPointInstancer prototypes, ignoring any prototypes
/// that are not reference assemblies.
class UsdMayaGL_InstancerShapeAdapter : public PxrMayaHdShapeAdapter
{
    public:

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
        bool UpdateVisibility(const M3dView* view = nullptr) override;

        /// Gets whether the shape adapter's shape is visible.
        ///
        /// This should be called after a call to UpdateVisibility() to ensure
        /// that the returned value is correct. 
        bool IsVisible() const override;

        void SetRootXform(const GfMatrix4d& transform) override;

        const SdfPath& GetDelegateID() const override;

        ~UsdMayaGL_InstancerShapeAdapter() override;

    protected:

        /// Update the shape adapter's state from the shape with the given
        /// \p dagPath and display state.
        ///
        /// This method should be called by both public versions of Sync() and
        /// should perform shape data updates that are common to both the
        /// legacy viewport and Viewport 2.0. The legacy viewport Sync() method
        /// "promotes" the display state parameters to their Viewport 2.0
        /// equivalents before calling this method.
        bool _Sync(
                const MDagPath& shapeDagPath,
                const unsigned int displayStyle,
                const MHWRender::DisplayStatus displayStatus) override;

        /// Construct a new uninitialized UsdMayaGL_InstancerShapeAdapter.
        ///
        /// Note that only friends of this class are able to construct
        /// instances of this class.
        UsdMayaGL_InstancerShapeAdapter();

    private:

        /// Initialize the shape adapter using the given \p renderIndex.
        ///
        /// This method is called automatically during Sync() when the shape
        /// adapter's "identity" changes. This happens when the delegateId or
        /// the rprim collection name computed from the shape adapter's shape
        /// is different than what is currently stored in the shape adapter.
        /// The shape adapter will then query the batch renderer for its render
        /// index and use that to re-create its delegate and re-add its rprim
        /// collection, if necessary.
        bool _Init(HdRenderIndex* renderIndex);

        /// Updates the prototypes prims and the corresponding prototypes rel on
        /// the point instancer. Errored or untranslatable prototypes are left
        /// as empty prims in the prototype order. Returns the total number of
        /// prototypes (including errored or untranslatable prototypes).
        size_t _SyncInstancerPrototypes(
                const UsdGeomPointInstancer& usdInstancer,
                const MPlug& inputHierarchy);

        /// Updates all of the attributes on the \p usdInstancer from the native
        /// Maya instancer given by \p mayaInstancerPath.
        /// If there was a problem reading prototypes or there are no
        /// prototypes, then the whole instancer will be emptied out.
        void _SyncInstancer(
                const UsdGeomPointInstancer& usdInstancer,
                const MDagPath& mayaInstancerPath);

        UsdStageRefPtr _instancerStage;

        std::shared_ptr<UsdImagingDelegate> _delegate;

        /// The classes that maintain ownership of and are responsible for
        /// updating the shape adapter for their shape are made friends of
        /// UsdMayaGL_InstancerShapeAdapter so that they alone can set
        /// properties on the shape adapter.
        friend class UsdMayaGL_InstancerImager;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
