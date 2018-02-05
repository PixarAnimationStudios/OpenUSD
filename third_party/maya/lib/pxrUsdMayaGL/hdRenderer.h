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
///
/// \file hdRenderer.h
///

#ifndef PXRUSDMAYAGL_HDRENDERER_H
#define PXRUSDMAYAGL_HDRENDERER_H

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usdImaging/usdImagingGL/gl.h"

#include <maya/M3dView.h>
#include <maya/MColor.h>
#include <maya/MDrawRequest.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


/// \brief This is an helper object that shapes can hold to get consistent usd
/// drawing in maya.
///
/// Typical usage is as follows:
///
/// \code
/// getDrawRequests(...) {
///
///   ...
///
///   request.setToken(DRAW_SHADED_SMOOTH);
///   ...
///
/// }
/// \endcode
///
/// \code
/// draw(...) {
///
///   // gather data from the shape
///   ...
///
///   _hdRenderer.CheckRendererSetup(prim, excludePaths);
///
///   // create a params object and setup it up for the shape.
///   UsdImagingGL::RenderParams params;
///   ...
///
///   // invoke the render
///   _hdRenderer.Render(..., params);
/// }
/// \endcode
class UsdMayaGLHdRenderer
{
public:

    /// \brief Enum for various drawing styles.  Should be used in \c
    /// getDrawRequests on the call to \c request.setToken.
    enum DRAWING_STYLES {
	DRAW_POINTS,
        DRAW_WIREFRAME,
        DRAW_SHADED_FLAT,
        DRAW_SHADED_SMOOTH,
    	DRAW_BOUNDING_BOX
    };

    /// \brief struct to hold all the information needed for a 
    /// viewport 2.0 draw request. 
    struct RequestData {
        GfVec4f fWireframeColor;
        MBoundingBox bounds;
        MDrawRequest drawRequest;
    };
    typedef std::vector<RequestData> RequestDataArray;

    /// \brief Should be called when the prim to \p usdPrim to draw or \p
    /// excludePaths change
    PXRUSDMAYAGL_API
    void CheckRendererSetup(
            const UsdPrim& usdPrim, 
            const SdfPathVector& excludePaths);

    /// \brief Generate an array of draw requests based on the selection status
    /// of \c objPath
    PXRUSDMAYAGL_API
    void GenerateDefaultVp2DrawRequests(
            const MDagPath& objPath,
            const MHWRender::MFrameContext& frameContext,
            const MBoundingBox& bounds,
            UsdMayaGLHdRenderer::RequestDataArray *requestArray);
    /// \brief Render the USD.
    ///
    /// This function overrides some of the members of \p params, in particular,
    /// the \c drawMode.
    PXRUSDMAYAGL_API
    void Render(
            const MDrawRequest& aRequest, 
            M3dView& aView, 
            UsdImagingGL::RenderParams params) const; 

    /// \brief Render the array of draw requests in viewport 2.0
    ///
    /// This function assumes that you have already set your desired values for
    /// \c complexity \c shotGuides and \c showRenderGuides members of
    /// \p params 
    PXRUSDMAYAGL_API
    void RenderVp2(
        const RequestDataArray &requests,
        const MHWRender::MDrawContext& context,
        UsdImagingGL::RenderParams params) const;

    /// \brief Test for intersection, for use in \c select().
    PXRUSDMAYAGL_API
    bool TestIntersection(
            M3dView& aView, 
            UsdImagingGL::RenderParams params,
            GfVec3d* hitPoint) const; 

    /// \brief Helper function to convert from \p subdLevel (int) into Hydra's
    /// \p complexity parameter (\p float)
    PXRUSDMAYAGL_API
    static float SubdLevelToComplexity(int subdLevel);

private:
    UsdPrim _renderedPrim;
    SdfPathVector _excludePrimPaths;
    std::unique_ptr<UsdImagingGL> _renderer;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYAGL_HDRENDERER_H
