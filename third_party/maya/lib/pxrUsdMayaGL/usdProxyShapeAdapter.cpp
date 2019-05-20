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
#include "pxrUsdMayaGL/usdProxyShapeAdapter.h"

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/batchRenderer.h"
#include "pxrUsdMayaGL/debugCodes.h"
#include "pxrUsdMayaGL/renderParams.h"
#include "pxrUsdMayaGL/shapeAdapter.h"
#include "usdMaya/proxyShape.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usdImaging/usdImaging/delegate.h"

#include <maya/M3dView.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFrameContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MMatrix.h>
#include <maya/MObjectHandle.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <boost/functional/hash.hpp>

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


/* virtual */
bool
PxrMayaHdUsdProxyShapeAdapter::UpdateVisibility(const M3dView* view)
{
    bool isVisible;

    /// Note that M3dView::pluginObjectDisplay() is unfortunately not declared
    /// const, so we have to cast away the const-ness here.
    M3dView* nonConstView = const_cast<M3dView*>(view);

    if (nonConstView &&
            !nonConstView->pluginObjectDisplay(UsdMayaProxyShape::displayFilterName)) {
        // USD proxy shapes are being filtered from this view, so don't bother
        // checking any other visibility state.
        isVisible = false;
    } else if (!_GetVisibility(_shapeDagPath, view, &isVisible)) {
        return false;
    }

    if (_delegate && _delegate->GetRootVisibility() != isVisible) {
        _delegate->SetRootVisibility(isVisible);
        return true;
    }

    return false;
}

/* virtual */
bool
PxrMayaHdUsdProxyShapeAdapter::IsVisible() const
{
    return (_delegate && _delegate->GetRootVisibility());
}

/* virtual */
void
PxrMayaHdUsdProxyShapeAdapter::SetRootXform(const GfMatrix4d& transform)
{
    _rootXform = transform;

    if (_delegate) {
        _delegate->SetRootTransform(_rootXform);
    }
}

/* virtual */
const SdfPath&
PxrMayaHdUsdProxyShapeAdapter::GetDelegateID() const
{
    if (_delegate) {
        return _delegate->GetDelegateID();
    }

    return SdfPath::EmptyPath();
}

/* virtual */
bool
PxrMayaHdUsdProxyShapeAdapter::_Sync(
        const MDagPath& shapeDagPath,
        const unsigned int displayStyle,
        const MHWRender::DisplayStatus displayStatus)
{
    UsdMayaProxyShape* usdProxyShape =
            UsdMayaProxyShape::GetShapeAtDagPath(shapeDagPath);
    if (!usdProxyShape) {
        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
                "Failed to get UsdMayaProxyShape for '%s'\n",
                shapeDagPath.fullPathName().asChar());
        return false;
    }

    UsdPrim usdPrim;
    SdfPathVector excludedPrimPaths;
    int refineLevel;
    UsdTimeCode timeCode;
    bool drawRenderPurpose = false;
    bool drawProxyPurpose = true;
    bool drawGuidePurpose = false;
    if (!usdProxyShape->GetAllRenderAttributes(&usdPrim,
                                               &excludedPrimPaths,
                                               &refineLevel,
                                               &timeCode,
                                               &drawRenderPurpose,
                                               &drawProxyPurpose,
                                               &drawGuidePurpose)) {
        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
                "Failed to get render attributes for UsdMayaProxyShape '%s'\n",
                shapeDagPath.fullPathName().asChar());
        return false;
    }

    // Check for updates to the shape or changes in the batch renderer that
    // require us to re-initialize the shape adapter.
    HdRenderIndex* renderIndex =
        UsdMayaGLBatchRenderer::GetInstance().GetRenderIndex();
    if (!(shapeDagPath == _shapeDagPath) ||
            usdPrim != _rootPrim ||
            excludedPrimPaths != _excludedPrimPaths ||
            !_delegate ||
            renderIndex != &_delegate->GetRenderIndex()) {
        _shapeDagPath = shapeDagPath;
        _rootPrim = usdPrim;
        _excludedPrimPaths = excludedPrimPaths;

        if (!_Init(renderIndex)) {
            return false;
        }
    }

    // Reset _renderParams to the defaults.
    PxrMayaHdRenderParams renderParams;
    _renderParams = renderParams;

    TfTokenVector renderTags;
    renderTags.push_back(HdTokens->geometry);
    if (drawRenderPurpose) {
        renderTags.push_back(UsdGeomTokens->render);
    }
    if (drawProxyPurpose) {
        renderTags.push_back(HdTokens->proxy);
    }
    if (drawGuidePurpose) {
        renderTags.push_back(HdTokens->guide);
    }

    if (_rprimCollection.GetRenderTags() != renderTags) {
        _rprimCollection.SetRenderTags(renderTags);

        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
            "    Render tags changed: %s\n"
            "        Marking collection dirty: %s\n",
            TfStringJoin(renderTags.begin(), renderTags.end()).c_str(),
            _rprimCollection.GetName().GetText());

        _delegate->GetRenderIndex().GetChangeTracker().MarkCollectionDirty(
            _rprimCollection.GetName());
    }

    MStatus status;
    const MMatrix transform = _shapeDagPath.inclusiveMatrix(&status);
    if (status == MS::kSuccess) {
        _rootXform = GfMatrix4d(transform.matrix);
        _delegate->SetRootTransform(_rootXform);
    }

    _delegate->SetRefineLevelFallback(refineLevel);

    // Will only react if time actually changes.
    _delegate->SetTime(timeCode);

    unsigned int reprDisplayStyle = displayStyle;

    MColor mayaWireframeColor;
    const bool useWireframeColor =
        _GetWireframeColor(
            displayStyle,
            displayStatus,
            _shapeDagPath,
            &mayaWireframeColor);
    if (useWireframeColor) {
        _renderParams.wireframeColor = GfVec4f(mayaWireframeColor.r,
                                               mayaWireframeColor.g,
                                               mayaWireframeColor.b,
                                               mayaWireframeColor.a);

        // Add in kWireFrame to the display style we'll use to determine the
        // repr selector (e.g. so that we draw the wireframe over the shaded
        // geometry for selected objects).
        reprDisplayStyle |= MHWRender::MFrameContext::DisplayStyle::kWireFrame;
    }

    HdReprSelector reprSelector =
        GetReprSelectorForDisplayState(
            reprDisplayStyle,
            displayStatus);

    _drawShape = reprSelector.AnyActiveRepr();
    _drawBoundingBox =
        (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBoundingBox);

    // If the repr selector specifies a wireframe-only repr, then disable
    // lighting.
    if (reprSelector.Contains(HdReprTokens->wire) ||
            reprSelector.Contains(HdReprTokens->refinedWire)) {
        _renderParams.enableLighting = false;
    }

    if (_delegate->GetRootVisibility() != _drawShape) {
        _delegate->SetRootVisibility(_drawShape);
    }

    if (_rprimCollection.GetReprSelector() != reprSelector) {
        _rprimCollection.SetReprSelector(reprSelector);

        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
                "    Repr selector changed: %s\n"
                "        Marking collection dirty: %s\n",
                reprSelector.GetText(),
                _rprimCollection.GetName().GetText());

        _delegate->GetRenderIndex().GetChangeTracker().MarkCollectionDirty(
            _rprimCollection.GetName());
    }

    // The kBackfaceCulling display style was introduced in Maya 2016 SP2.
    HdCullStyle cullStyle = HdCullStyleNothing;
#if MAYA_API_VERSION >= 201603
    if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling) {
        cullStyle = HdCullStyleBackUnlessDoubleSided;
    }
#endif

    _delegate->SetCullStyleFallback(cullStyle);

    return true;
}

bool
PxrMayaHdUsdProxyShapeAdapter::_Init(HdRenderIndex* renderIndex)
{
    if (!TF_VERIFY(renderIndex,
                   "Cannot initialize shape adapter with invalid HdRenderIndex")) {
        return false;
    }

    const SdfPath delegatePrefix =
        UsdMayaGLBatchRenderer::GetInstance().GetDelegatePrefix(_isViewport2);

    // Create a simple "name" for this shape adapter to insert into the batch
    // renderer's SdfPath hierarchy.
    //
    // XXX: For as long as we're using the MAYA_VP2_USE_VP1_SELECTION
    // environment variable, we need to be able to pass responsibility back and
    // forth between the MPxDrawOverride's shape adapter for drawing and the
    // MPxSurfaceShapeUI's shape adapter for selection. This requires both
    // shape adapters to have the same "name", which forces us to build it
    // from data on the shape that will be common to both classes, as we do
    // below. When we remove MAYA_VP2_USE_VP1_SELECTION and can trust that a
    // single shape adapter handles both drawing and selection, we can do
    // something even simpler instead like using the shape adapter's memory
    // address as the "name".
    size_t shapeHash(MObjectHandle(_shapeDagPath.transform()).hashCode());
    boost::hash_combine(shapeHash, _rootPrim);
    boost::hash_combine(shapeHash, _excludedPrimPaths);

    // We prepend the Maya type name to the beginning of the delegate name to
    // ensure that there are no name collisions between shape adapters of
    // shapes with different Maya types.
    const TfToken delegateName(
        TfStringPrintf("%s_%zx",
                       UsdMayaProxyShapeTokens->MayaTypeName.GetText(),
                       shapeHash));

    const SdfPath delegateId = delegatePrefix.AppendChild(delegateName);

    if (_delegate &&
            delegateId == GetDelegateID() &&
            renderIndex == &_delegate->GetRenderIndex()) {
        // The delegate's current ID matches the delegate ID we computed and
        // the render index matches, so it must be up to date already.
        return true;
    }

    const TfToken collectionName(_shapeDagPath.fullPathName().asChar());

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Initializing PxrMayaHdUsdProxyShapeAdapter: %p\n"
        "    collection name: %s\n"
        "    delegateId     : %s\n",
        this,
        collectionName.GetText(),
        delegateId.GetText());

    _delegate.reset(new UsdImagingDelegate(renderIndex, delegateId));
    if (!TF_VERIFY(_delegate,
                  "Failed to create shape adapter delegate for shape %s",
                  _shapeDagPath.fullPathName().asChar())) {
        return false;
    }

    if (TfDebug::IsEnabled(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)) {
        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
            "    Populating delegate:\n"
            "        rootPrim         : %s\n"
            "        excludedPrimPaths: ",
            _rootPrim.GetPath().GetText());
        for (const SdfPath& primPath : _excludedPrimPaths) {
            TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
                "%s ",
                primPath.GetText());
        }
        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg("\n");
    }

    _delegate->Populate(_rootPrim, _excludedPrimPaths, SdfPathVector());

    if (collectionName != _rprimCollection.GetName()) {
        _rprimCollection.SetName(collectionName);
        renderIndex->GetChangeTracker().AddCollection(_rprimCollection.GetName());
    }

    _rprimCollection.SetReprSelector(HdReprSelector(HdReprTokens->refined));
    _rprimCollection.SetRootPath(delegateId);

    return true;
}

PxrMayaHdUsdProxyShapeAdapter::PxrMayaHdUsdProxyShapeAdapter()
{
    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Constructing PxrMayaHdUsdProxyShapeAdapter: %p\n",
        this);
}

/* virtual */
PxrMayaHdUsdProxyShapeAdapter::~PxrMayaHdUsdProxyShapeAdapter()
{
    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Destructing PxrMayaHdUsdProxyShapeAdapter: %p\n",
        this);
}


PXR_NAMESPACE_CLOSE_SCOPE
