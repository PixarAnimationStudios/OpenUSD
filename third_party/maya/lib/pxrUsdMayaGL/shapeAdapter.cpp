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
#include "pxrUsdMayaGL/shapeAdapter.h"

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/batchRenderer.h"
#include "pxrUsdMayaGL/renderParams.h"
#include "pxrUsdMayaGL/softSelectHelper.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usdImaging/usdImaging/delegate.h"

#include <maya/M3dView.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MFrameContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MMatrix.h>
#include <maya/MObjectHandle.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <boost/functional/hash.hpp>

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((RenderGuidesTag, "render"))
);


PxrMayaHdShapeAdapter::PxrMayaHdShapeAdapter(
        const MDagPath& shapeDagPath,
        const UsdPrim& rootPrim,
        const SdfPathVector& excludedPrimPaths) :
    _shapeDagPath(shapeDagPath),
    _rootPrim(rootPrim),
    _excludedPrimPaths(excludedPrimPaths),
    _isPopulated(false)
{
}

size_t
PxrMayaHdShapeAdapter::GetHash() const
{
    size_t shapeHash(MObjectHandle(_shapeDagPath.transform()).hashCode());
    boost::hash_combine(shapeHash, _rootPrim);
    boost::hash_combine(shapeHash, _excludedPrimPaths);

    return shapeHash;
}

void
PxrMayaHdShapeAdapter::Init(HdRenderIndex* renderIndex)
{
    const size_t shapeHash = GetHash();

    // Create a simple hash string to put into a flat SdfPath "hierarchy".
    // This is much faster than more complicated pathing schemes.
    const std::string idString = TfStringPrintf("/x%zx", shapeHash);
    _sharedId = SdfPath(idString);

    _delegate.reset(new UsdImagingDelegate(renderIndex, _sharedId));

    _rprimCollection.SetName(TfToken(_shapeDagPath.fullPathName().asChar()));
    _rprimCollection.SetReprName(HdTokens->refined);
    _rprimCollection.SetRootPath(_sharedId);

    renderIndex->GetChangeTracker().AddCollection(_rprimCollection.GetName());
}

void
PxrMayaHdShapeAdapter::PrepareForQueue(
        const UsdTimeCode time,
        const uint8_t refineLevel,
        const bool showGuides,
        const bool showRenderGuides,
        const bool tint,
        const GfVec4f& tintColor)
{
    // Initialization of default parameters go here. These parameters get used
    // in all viewports and for selection.
    _baseParams.timeCode = time;
    _baseParams.refineLevel = refineLevel;

    // XXX Not yet adding ability to turn off display of proxy geometry, but
    // we should at some point, as in usdview.
    _baseParams.renderTags.clear();
    _baseParams.renderTags.push_back(HdTokens->geometry);
    _baseParams.renderTags.push_back(HdTokens->proxy);
    if (showGuides) {
        _baseParams.renderTags.push_back(HdTokens->guide);
    }
    if (showRenderGuides) {
        _baseParams.renderTags.push_back(_tokens->RenderGuidesTag);
    }

    if (_rprimCollection.GetRenderTags() != _baseParams.renderTags) {
        _rprimCollection.SetRenderTags(_baseParams.renderTags);

        _delegate->GetRenderIndex().GetChangeTracker().MarkCollectionDirty(
            _rprimCollection.GetName());
    }

    if (tint) {
        _baseParams.overrideColor = tintColor;
    }

    if (_delegate) {
        MStatus status;
        const MMatrix transform = _shapeDagPath.inclusiveMatrix(&status);
        if (status == MS::kSuccess) {
            _rootXform = GfMatrix4d(transform.matrix);
            _delegate->SetRootTransform(_rootXform);
        }

        _delegate->SetRefineLevelFallback(refineLevel);

        // Will only react if time actually changes.
        _delegate->SetTime(time);

        _delegate->SetRootCompensation(_rootPrim.GetPath());

        if (!_isPopulated) {
            _delegate->Populate(_rootPrim, _excludedPrimPaths, SdfPathVector());
            _isPopulated = true;
        }
    }
}

// Helper function that converts M3dView::DisplayStatus (legacy viewport) into
// MHWRender::DisplayStatus (Viewport 2.0).
static inline
MHWRender::DisplayStatus
_ToMHWRenderDisplayStatus(const M3dView::DisplayStatus displayStatus)
{
    // These enums are equivalent, but statically checking just in case.
    static_assert(
        ((int)M3dView::kActive == (int)MHWRender::kActive) &&
        ((int)M3dView::kLive == (int)MHWRender::kLive) &&
        ((int)M3dView::kDormant == (int)MHWRender::kDormant) &&
        ((int)M3dView::kInvisible == (int)MHWRender::kInvisible) &&
        ((int)M3dView::kHilite == (int)MHWRender::kHilite) &&
        ((int)M3dView::kTemplate == (int)MHWRender::kTemplate) &&
        ((int)M3dView::kActiveTemplate == (int)MHWRender::kActiveTemplate) &&
        ((int)M3dView::kActiveComponent == (int)MHWRender::kActiveComponent) &&
        ((int)M3dView::kLead == (int)MHWRender::kLead) &&
        ((int)M3dView::kIntermediateObject == (int)MHWRender::kIntermediateObject) &&
        ((int)M3dView::kActiveAffected == (int)MHWRender::kActiveAffected) &&
        ((int)M3dView::kNoStatus == (int)MHWRender::kNoStatus),
            "M3dView::DisplayStatus == MHWRender::DisplayStatus");

    return MHWRender::DisplayStatus((int)displayStatus);
}

bool
PxrMayaHdShapeAdapter::_GetWireframeColor(
        const MHWRender::DisplayStatus displayStatus,
        MColor* mayaWireColor)
{
    // Dormant objects may be included in a soft selection.
    if (displayStatus == MHWRender::kDormant) {
        const UsdMayaGLSoftSelectHelper& softSelectHelper =
            UsdMayaGLBatchRenderer::GetInstance().GetSoftSelectHelper();
        return softSelectHelper.GetFalloffColor(_shapeDagPath, mayaWireColor);
    }
    else if ((displayStatus == MHWRender::kActive) ||
             (displayStatus == MHWRender::kLead) ||
             (displayStatus == MHWRender::kHilite)) {
        *mayaWireColor =
            MHWRender::MGeometryUtilities::wireframeColor(_shapeDagPath);
        return true;
    }

    return false;
}

PxrMayaHdRenderParams
PxrMayaHdShapeAdapter::GetRenderParams(
        const M3dView::DisplayStyle displayStyle,
        const M3dView::DisplayStatus displayStatus,
        bool* drawShape,
        bool* drawBoundingBox)
{
    // Legacy viewport Implementation.

    PxrMayaHdRenderParams params(_baseParams);

    // The legacy viewport does not allow shapes and bounding boxes to be drawn
    // at the same time...
    *drawBoundingBox = (displayStyle == M3dView::kBoundingBox);
    *drawShape = !*drawBoundingBox;

    MColor mayaWireframeColor;
    const bool needsWire = _GetWireframeColor(
        _ToMHWRenderDisplayStatus(displayStatus),
        &mayaWireframeColor);

    if (needsWire) {
        // The legacy viewport does not support color management, so we roll
        // our own gamma correction via framebuffer effect. But that means we
        // need to pre-linearize the wireframe color from Maya.
        params.wireframeColor =
            GfConvertDisplayToLinear(GfVec4f(mayaWireframeColor.r,
                                             mayaWireframeColor.g,
                                             mayaWireframeColor.b,
                                             1.0f));
    }

    switch (displayStyle) {
        case M3dView::kWireFrame:
        {
            params.drawRepr = HdTokens->refinedWire;
            params.enableLighting = false;
            break;
        }
        case M3dView::kGouraudShaded:
        {
            if (needsWire) {
                params.drawRepr = HdTokens->refinedWireOnSurf;
            } else {
                params.drawRepr = HdTokens->refined;
            }
            break;
        }
        case M3dView::kFlatShaded:
        {
            if (needsWire) {
                params.drawRepr = HdTokens->wireOnSurf;
            } else {
                params.drawRepr = HdTokens->hull;
            }
            break;
        }
        case M3dView::kPoints:
        {
            // Points mode is not natively supported by Hydra, so skip it...
        }
        default:
        {
            *drawShape = false;
        }
    };

    if (_rprimCollection.GetReprName() != params.drawRepr) {
        _rprimCollection.SetReprName(params.drawRepr);

        _delegate->GetRenderIndex().GetChangeTracker().MarkCollectionDirty(
            _rprimCollection.GetName());
    }

    return params;
}

PxrMayaHdRenderParams
PxrMayaHdShapeAdapter::GetRenderParams(
        const unsigned int displayStyle,
        const MHWRender::DisplayStatus displayStatus,
        bool* drawShape,
        bool* drawBoundingBox)
{
    // VP 2.0 Implementation

    PxrMayaHdRenderParams params(_baseParams);

    *drawShape = true;
    *drawBoundingBox =
        (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBoundingBox);

    MColor mayaWireframeColor;
    const bool needsWire = _GetWireframeColor(displayStatus,
                                              &mayaWireframeColor);
    if (needsWire) {
        params.wireframeColor = GfVec4f(mayaWireframeColor.r,
                                        mayaWireframeColor.g,
                                        mayaWireframeColor.b,
                                        1.0f);
    }

    // Maya 2015 lacks MHWRender::MFrameContext::DisplayStyle::kFlatShaded for
    // whatever reason...
    const bool flatShaded =
#if MAYA_API_VERSION >= 201600
        displayStyle & MHWRender::MFrameContext::DisplayStyle::kFlatShaded;
#else
        false;
#endif

    if (flatShaded) {
        if (needsWire) {
            params.drawRepr = HdTokens->wireOnSurf;
        } else {
            params.drawRepr = HdTokens->hull;
        }
    }
    else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kGouraudShaded)
    {
        if (needsWire || (displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame)) {
            params.drawRepr = HdTokens->refinedWireOnSurf;
        } else {
            params.drawRepr = HdTokens->refined;
        }
    }
    else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame)
    {
        params.drawRepr = HdTokens->refinedWire;
        params.enableLighting = false;
    }
    else
    {
        *drawShape = false;
    }

    // Maya 2016 SP2 lacks MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling
    // for whatever reason...
    params.cullStyle = HdCullStyleNothing;
#if MAYA_API_VERSION >= 201603
    if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling) {
        params.cullStyle = HdCullStyleBackUnlessDoubleSided;
    }
#endif

    if (_rprimCollection.GetReprName() != params.drawRepr) {
        _rprimCollection.SetReprName(params.drawRepr);

        _delegate->GetRenderIndex().GetChangeTracker().MarkCollectionDirty(
            _rprimCollection.GetName());
    }

    return params;
}


PXR_NAMESPACE_CLOSE_SCOPE
