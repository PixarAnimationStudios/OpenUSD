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
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdGeom/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdGeomWrapTokens {

// Helper to return a static token as a string.  We wrap tokens as Python
// strings and for some reason simply wrapping the token using def_readonly
// bypasses to-Python conversion, leading to the error that there's no
// Python type for the C++ TfToken type.  So we wrap this functor instead.
class _WrapStaticToken {
public:
    _WrapStaticToken(const TfToken* token) : _token(token) { }

    std::string operator()() const
    {
        return _token->GetString();
    }

private:
    const TfToken* _token;
};

template <typename T>
void
_AddToken(T& cls, const char* name, const TfToken& token)
{
    cls.add_static_property(name,
                            boost::python::make_function(
                                _WrapStaticToken(&token),
                                boost::python::return_value_policy<
                                    boost::python::return_by_value>(),
                                boost::mpl::vector1<std::string>()));
}

} // anonymous

void wrapUsdGeomTokens()
{
    boost::python::class_<UsdGeomTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "accelerations", UsdGeomTokens->accelerations);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "all", UsdGeomTokens->all);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "angularVelocities", UsdGeomTokens->angularVelocities);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "axis", UsdGeomTokens->axis);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "basis", UsdGeomTokens->basis);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "bezier", UsdGeomTokens->bezier);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "bilinear", UsdGeomTokens->bilinear);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "boundaries", UsdGeomTokens->boundaries);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "bounds", UsdGeomTokens->bounds);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "box", UsdGeomTokens->box);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "bspline", UsdGeomTokens->bspline);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "cards", UsdGeomTokens->cards);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "catmullClark", UsdGeomTokens->catmullClark);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "catmullRom", UsdGeomTokens->catmullRom);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "clippingPlanes", UsdGeomTokens->clippingPlanes);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "clippingRange", UsdGeomTokens->clippingRange);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "closed", UsdGeomTokens->closed);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "constant", UsdGeomTokens->constant);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "cornerIndices", UsdGeomTokens->cornerIndices);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "cornerSharpnesses", UsdGeomTokens->cornerSharpnesses);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "cornersOnly", UsdGeomTokens->cornersOnly);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "cornersPlus1", UsdGeomTokens->cornersPlus1);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "cornersPlus2", UsdGeomTokens->cornersPlus2);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "creaseIndices", UsdGeomTokens->creaseIndices);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "creaseLengths", UsdGeomTokens->creaseLengths);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "creaseSharpnesses", UsdGeomTokens->creaseSharpnesses);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "cross", UsdGeomTokens->cross);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "cubic", UsdGeomTokens->cubic);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "curveVertexCounts", UsdGeomTokens->curveVertexCounts);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "default_", UsdGeomTokens->default_);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "doubleSided", UsdGeomTokens->doubleSided);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "edgeAndCorner", UsdGeomTokens->edgeAndCorner);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "edgeOnly", UsdGeomTokens->edgeOnly);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "elementSize", UsdGeomTokens->elementSize);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "elementType", UsdGeomTokens->elementType);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "exposure", UsdGeomTokens->exposure);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "extent", UsdGeomTokens->extent);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "extentsHint", UsdGeomTokens->extentsHint);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "face", UsdGeomTokens->face);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "faceVarying", UsdGeomTokens->faceVarying);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "faceVaryingLinearInterpolation", UsdGeomTokens->faceVaryingLinearInterpolation);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "faceVertexCounts", UsdGeomTokens->faceVertexCounts);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "faceVertexIndices", UsdGeomTokens->faceVertexIndices);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "familyName", UsdGeomTokens->familyName);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "focalLength", UsdGeomTokens->focalLength);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "focusDistance", UsdGeomTokens->focusDistance);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "fromTexture", UsdGeomTokens->fromTexture);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "fStop", UsdGeomTokens->fStop);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "guide", UsdGeomTokens->guide);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "guideVisibility", UsdGeomTokens->guideVisibility);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "height", UsdGeomTokens->height);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "hermite", UsdGeomTokens->hermite);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "holeIndices", UsdGeomTokens->holeIndices);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "horizontalAperture", UsdGeomTokens->horizontalAperture);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "horizontalApertureOffset", UsdGeomTokens->horizontalApertureOffset);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "ids", UsdGeomTokens->ids);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "inactiveIds", UsdGeomTokens->inactiveIds);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "indices", UsdGeomTokens->indices);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "inherited", UsdGeomTokens->inherited);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "interpolateBoundary", UsdGeomTokens->interpolateBoundary);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "interpolation", UsdGeomTokens->interpolation);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "invisible", UsdGeomTokens->invisible);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "invisibleIds", UsdGeomTokens->invisibleIds);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "knots", UsdGeomTokens->knots);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "left", UsdGeomTokens->left);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "leftHanded", UsdGeomTokens->leftHanded);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "linear", UsdGeomTokens->linear);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "loop", UsdGeomTokens->loop);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "metersPerUnit", UsdGeomTokens->metersPerUnit);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "modelApplyDrawMode", UsdGeomTokens->modelApplyDrawMode);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "modelCardGeometry", UsdGeomTokens->modelCardGeometry);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "modelCardTextureXNeg", UsdGeomTokens->modelCardTextureXNeg);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "modelCardTextureXPos", UsdGeomTokens->modelCardTextureXPos);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "modelCardTextureYNeg", UsdGeomTokens->modelCardTextureYNeg);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "modelCardTextureYPos", UsdGeomTokens->modelCardTextureYPos);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "modelCardTextureZNeg", UsdGeomTokens->modelCardTextureZNeg);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "modelCardTextureZPos", UsdGeomTokens->modelCardTextureZPos);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "modelDrawMode", UsdGeomTokens->modelDrawMode);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "modelDrawModeColor", UsdGeomTokens->modelDrawModeColor);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "mono", UsdGeomTokens->mono);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "motionVelocityScale", UsdGeomTokens->motionVelocityScale);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "none", UsdGeomTokens->none);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "nonOverlapping", UsdGeomTokens->nonOverlapping);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "nonperiodic", UsdGeomTokens->nonperiodic);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "normals", UsdGeomTokens->normals);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "open", UsdGeomTokens->open);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "order", UsdGeomTokens->order);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "orientation", UsdGeomTokens->orientation);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "orientations", UsdGeomTokens->orientations);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "origin", UsdGeomTokens->origin);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "orthographic", UsdGeomTokens->orthographic);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "partition", UsdGeomTokens->partition);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "periodic", UsdGeomTokens->periodic);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "perspective", UsdGeomTokens->perspective);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "pinned", UsdGeomTokens->pinned);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "pivot", UsdGeomTokens->pivot);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "points", UsdGeomTokens->points);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "pointWeights", UsdGeomTokens->pointWeights);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "positions", UsdGeomTokens->positions);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "power", UsdGeomTokens->power);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "primvarsDisplayColor", UsdGeomTokens->primvarsDisplayColor);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "primvarsDisplayOpacity", UsdGeomTokens->primvarsDisplayOpacity);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "projection", UsdGeomTokens->projection);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "protoIndices", UsdGeomTokens->protoIndices);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "prototypes", UsdGeomTokens->prototypes);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "proxy", UsdGeomTokens->proxy);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "proxyPrim", UsdGeomTokens->proxyPrim);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "proxyVisibility", UsdGeomTokens->proxyVisibility);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "purpose", UsdGeomTokens->purpose);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "radius", UsdGeomTokens->radius);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "ranges", UsdGeomTokens->ranges);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "render", UsdGeomTokens->render);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "renderVisibility", UsdGeomTokens->renderVisibility);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "right", UsdGeomTokens->right);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "rightHanded", UsdGeomTokens->rightHanded);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "scales", UsdGeomTokens->scales);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "shutterClose", UsdGeomTokens->shutterClose);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "shutterOpen", UsdGeomTokens->shutterOpen);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "size", UsdGeomTokens->size);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "smooth", UsdGeomTokens->smooth);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "stereoRole", UsdGeomTokens->stereoRole);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "subdivisionScheme", UsdGeomTokens->subdivisionScheme);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "tangents", UsdGeomTokens->tangents);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "triangleSubdivisionRule", UsdGeomTokens->triangleSubdivisionRule);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "trimCurveCounts", UsdGeomTokens->trimCurveCounts);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "trimCurveKnots", UsdGeomTokens->trimCurveKnots);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "trimCurveOrders", UsdGeomTokens->trimCurveOrders);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "trimCurvePoints", UsdGeomTokens->trimCurvePoints);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "trimCurveRanges", UsdGeomTokens->trimCurveRanges);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "trimCurveVertexCounts", UsdGeomTokens->trimCurveVertexCounts);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "type", UsdGeomTokens->type);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "uForm", UsdGeomTokens->uForm);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "uKnots", UsdGeomTokens->uKnots);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "unauthoredValuesIndex", UsdGeomTokens->unauthoredValuesIndex);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "uniform", UsdGeomTokens->uniform);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "unrestricted", UsdGeomTokens->unrestricted);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "uOrder", UsdGeomTokens->uOrder);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "upAxis", UsdGeomTokens->upAxis);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "uRange", UsdGeomTokens->uRange);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "uVertexCount", UsdGeomTokens->uVertexCount);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "varying", UsdGeomTokens->varying);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "velocities", UsdGeomTokens->velocities);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "vertex", UsdGeomTokens->vertex);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "verticalAperture", UsdGeomTokens->verticalAperture);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "verticalApertureOffset", UsdGeomTokens->verticalApertureOffset);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "vForm", UsdGeomTokens->vForm);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "visibility", UsdGeomTokens->visibility);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "visible", UsdGeomTokens->visible);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "vKnots", UsdGeomTokens->vKnots);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "vOrder", UsdGeomTokens->vOrder);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "vRange", UsdGeomTokens->vRange);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "vVertexCount", UsdGeomTokens->vVertexCount);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "widths", UsdGeomTokens->widths);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "wrap", UsdGeomTokens->wrap);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "x", UsdGeomTokens->x);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "xformOpOrder", UsdGeomTokens->xformOpOrder);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "y", UsdGeomTokens->y);
    pxrUsdUsdGeomWrapTokens::_AddToken(cls, "z", UsdGeomTokens->z);
}
