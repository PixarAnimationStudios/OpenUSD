//
// Copyright 2024 Pixar
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
#include "pxr/usdImaging/usdImaging/simpleTextAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/text.h"
#include "pxr/usdImaging/usdImaging/textRenderer.h"
#include "pxr/usdImaging/usdImaging/textStyle.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/simpleText.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/usd/usdText/simpleText.h"
#include "pxr/usd/usdText/textStyle.h"
#include "pxr/usd/usdText/textStyleAPI.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the adapter.
TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingSimpleTextAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingSimpleTextAdapter::~UsdImagingSimpleTextAdapter()
{
}

bool
UsdImagingSimpleTextAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    if (!UsdImagingText::IsInitialized())
    {
        if (!UsdImagingText::DefaultInitialize())
            return false;
    }
    return index->IsRprimTypeSupported(HdPrimTypeTokens->simpleText);
}

bool GenerateTextGeometries(UsdPrim const& prim, UsdTimeCode time, VtVec3fArray& geometries,
    VtVec4fArray& textCoords, VtVec3fArray& lineGeometries)
{
    UsdTextSimpleText text(prim);
    // The text data and encoding must be specified.
    std::string textData;
    if (!TF_VERIFY(text.GetTextDataAttr().Get(&textData, time),
        "The text primitive must contains text data.")) {
        return false;
    }

    UsdImagingTextStyle textStyle;
    // Get the textstyle from the TextStyleBinding.
    if (TF_VERIFY(UsdTextTextStyleAPI::CanApply(prim),
        "The simple text primitive must bind to a text style."))
    {
        UsdTextTextStyleAPI::TextStyleBinding styleBinding =
            UsdTextTextStyleAPI(prim).GetTextStyleBinding(prim.GetPath());
        UsdTextTextStyle style = styleBinding.GetTextStyle();
        if (!style.GetPath().IsEmpty())
        {
            // The typeface and height must be specified.
            if (!TF_VERIFY(style.GetTypefaceAttr().Get(&textStyle._typeface, time),
                "The text style must contain a typeface of the font.")) {
                return false;
            }
            if (!TF_VERIFY(style.GetTextHeightAttr().Get(&textStyle._height, time),
                "The text style must have a height.")) {
                return false;
            }
            // The widthFactor, obliqueAngle, characterSpace, bold, italic, underline and 
            // overline can have default value.
            if (!style.GetTextWidthFactorAttr().Get(&textStyle._widthFactor, time)) {
                textStyle._widthFactor = 1.0f;
            }
            if (!style.GetObliqueAngleAttr().Get(&textStyle._obliqueAngle, time)) {
                textStyle._obliqueAngle = 0.0f;
            }
            if (!style.GetCharSpacingAttr().Get(&textStyle._characterSpaceFactor, time)) {
                textStyle._characterSpaceFactor = 0.0f;
            }
            if (!style.GetBoldAttr().Get(&textStyle._bold, time)) {
                textStyle._bold = false;
            }
            if (!style.GetItalicAttr().Get(&textStyle._italic, time)) {
                textStyle._italic = false;
            }
            std::string lineType = "none";
            if (!style.GetUnderlineTypeAttr().Get(&lineType, time)) {
                lineType = "none";
            }
            if (lineType == "normal")
                textStyle._underlineType = UsdImagingTextTokens->normal;
            else
                textStyle._underlineType = UsdImagingTextTokens->none;

            if (!style.GetOverlineTypeAttr().Get(&lineType, time)) {
                lineType = "none";
            }
            if (lineType == "normal")
                textStyle._overlineType = UsdImagingTextTokens->normal;
            else
                textStyle._overlineType = UsdImagingTextTokens->none;

            if (!style.GetStrikethroughTypeAttr().Get(&lineType, time)) {
                lineType = "none";
            }
            if (lineType == "normal")
                textStyle._strikethroughType = UsdImagingTextTokens->normal;
            else if (lineType == "doubleLines")
                textStyle._strikethroughType = UsdImagingTextTokens->doubleLines;
            else
                textStyle._strikethroughType = UsdImagingTextTokens->none;
        }
    }
    else {
        return false;
    }

    // Get the rendering technique of the text prim. By default it is shader based.
    std::string rendererName("");
    if (!text.GetRendererAttr().Get(&rendererName, 0))
    {
        rendererName = std::string("");
    }

    UsdImagingTextRendererSharedPtr renderer = UsdImagingTextRenderer::GetTextRenderer(rendererName);
    if (!TF_VERIFY(renderer, "The text primitive must set a reasonable renderer."))
        return false;

    return UsdImagingText::GenerateSimpleTextGeometries(renderer, textData, textStyle,
        geometries, textCoords, lineGeometries);
}

SdfPath
UsdImagingSimpleTextAdapter::Populate(UsdPrim const& prim,
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->simpleText,
        prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void
UsdImagingSimpleTextAdapter::TrackVariability(UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const*
    instancerContext) const
{
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);

    // The textData will impact the topology.
    if ((*timeVaryingBits & HdChangeTracker::DirtyTopology) == 0) {
        _IsVarying(prim, UsdTextTokens->textData,
            HdChangeTracker::DirtyTopology,
            UsdImagingTokens->usdVaryingTopology,
            timeVaryingBits, /*inherited*/false);
    }
}

bool
UsdImagingSimpleTextAdapter::_IsBuiltinPrimvar(TfToken const& primvarName) const
{
    return (primvarName == HdTokens->textCoord 
        || primvarName == HdTokens->linePoints || BaseAdapter::_IsBuiltinPrimvar(primvarName));
}

void
UsdImagingSimpleTextAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);

    UsdImagingPrimvarDescCache* primvarDescCache = _GetPrimvarDescCache();

    // Geometry aspect
    HdPrimvarDescriptorVector& primvars =
        primvarDescCache->GetPrimvars(cachePath);

    if (requestedBits & HdChangeTracker::DirtyTopology) {
        _MergePrimvar(&primvars, HdTokens->textCoord, HdInterpolationVertex);
        _MergePrimvar(&primvars, HdTokens->linePoints, HdInterpolationVertex);
    }
}

HdDirtyBits
UsdImagingSimpleTextAdapter::ProcessPropertyChange(UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& propertyName)
{
    // The textData will impact the topology and points.
    if (propertyName == UsdTextTokens->textData) {
        return HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyPoints;
    }

    // If the property is prefixed with textStyle, we will always dirty both the topology and
    // points.
    if (TfStringStartsWith(propertyName, HdTextTokens->textStyle))
    {
        return HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyPoints;
    }
    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

/*virtual*/
VtValue
UsdImagingSimpleTextAdapter::GetTopology(UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TextGeometryCache::const_accessor accessor;
    if (!_textGeometryCache.find(accessor, cachePath)) {
        // Generate the geometry information if it is missing.
        std::shared_ptr<TextGeometry> geometryPtr = std::make_shared<TextGeometry>();
        if (GenerateTextGeometries(prim, UsdTimeCode::Default(), geometryPtr->geometries,
            geometryPtr->textCoords, geometryPtr->lineGeometries)) {
            _textGeometryCache.emplace(accessor, cachePath, geometryPtr);
        }
        else {
            HdSimpleTextTopology topology(0, 0);
            return VtValue(topology);
        }
    }

    // Get the point count from geometries.
    std::shared_ptr<TextGeometry> geometryPtr = accessor->second;
    size_t pointCount = geometryPtr->geometries.size();
    size_t decorationCount = geometryPtr->lineGeometries.size() / 2;
    HdSimpleTextTopology topology(pointCount, decorationCount);
    return VtValue(topology);
}

/*virtual*/
VtValue
UsdImagingSimpleTextAdapter::Get(UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& key,
    UsdTimeCode time,
    VtIntArray *outIndices) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtValue value;
    UsdGeomGprim gprim(prim);
    if (!TF_VERIFY(gprim)) {
        return value;
    }

    if (key == HdTokens->points || key == HdTokens->textCoord || key == HdTokens->linePoints)
    {
        TextGeometryCache::const_accessor accessor;
        if (!_textGeometryCache.find(accessor, cachePath)) {
            // Generate the geometry information if it is missing.
            std::shared_ptr<TextGeometry> geometryPtr = std::make_shared<TextGeometry>();
            if (GenerateTextGeometries(prim, UsdTimeCode::Default(), geometryPtr->geometries,
                geometryPtr->textCoords, geometryPtr->lineGeometries)) {
                _textGeometryCache.emplace(accessor, cachePath, geometryPtr);
            }
            else {
                return VtValue(VtVec3fArray(0));
            }
        }
        std::shared_ptr<TextGeometry> geometryPtr = accessor->second;

        if (key == HdTokens->points)
            value = geometryPtr->geometries;
        else if (key == HdTokens->textCoord)
            value = geometryPtr->textCoords;
        else if (key == HdTokens->linePoints)
            value = geometryPtr->lineGeometries;
        return value;
    }
    return BaseAdapter::Get(prim, cachePath, key, time, outIndices);
}

void
UsdImagingSimpleTextAdapter::_RemovePrim(const SdfPath& cachePath, UsdImagingIndexProxy* index)
{
    _textGeometryCache.erase(cachePath);
    index->RemoveRprim(cachePath);
}

void
UsdImagingSimpleTextAdapter::MarkDirty(UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits dirty,
    UsdImagingIndexProxy* index)
{
    // Need to remove the created geometry and other points related information, so that we can
    // recalculate them.
    if (dirty & HdChangeTracker::DirtyPoints) {
        _textGeometryCache.erase(cachePath);
    }
    index->MarkRprimDirty(cachePath, dirty);
}

PXR_NAMESPACE_CLOSE_SCOPE
