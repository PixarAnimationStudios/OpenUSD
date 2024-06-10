//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_OBJECT_UTIL_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_OBJECT_UTIL_H

#include "pxr/pxr.h"
#include "hdPrman/sceneIndexObserverApi.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/rileySchemaTypeDefs.h"
#include "hdPrman/rileyPrimvarListSchema.h"
#include "hdPrman/rileyParamListSchema.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"

#include "Riley.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

/// \name (Non-path) Conversions to riley types
///
/// Conversions of data sources in a scene index to riley types such as
/// RtUString, riley::Transform or RtParamList.
///
/// The conversions are provided as RAII wrappers wrapping a riley type.
///
/// Recall that some riley API expects non-RAII objects, e.g., riley::Transform
/// has a raw pointer to an array of RtMatrix4x4's and its d'tor does
/// not free that array.
///
/// In hdPrman, we wrap such an object in an RAII object T with
/// T::rileyObject being the non-RAII object pointing to, e.g., the
/// data of a std::vector<riley::RtMatrix4x4> in T.
///
/// @{

///
/// Extract T::rileyObject as pointer from std::optional<T>.
///
/// Use as a helper to extract the non-RAII object from a std::optional<T>
/// where T is the RAII wrapper.
///
/// Similar to HdPrman_GetPtr, can be used as argument to Riley::ModifyFoo.
///
template<typename T>
auto HdPrman_GetRileyObjectPtr(
    /* non-const because ModifyRenderOutput takes
       non-const relativePixelVariance */
        /* const */ std::optional<T> &v) -> decltype(&(v->rileyObject))
{
    if (!v) {
        return nullptr;
    }
    return &(v->rileyObject);
}

struct HdPrman_RileyParamList
{
    using RileyType = RtParamList;

    HdPrman_RileyParamList(
        HdPrmanRileyParamListSchema schema);

    RileyType rileyObject;
};

struct HdPrman_RileyDetailType
{
    using RileyType = RtDetailType;

    HdPrman_RileyDetailType(
        HdTokenDataSourceHandle const &ds);
    
    RileyType rileyObject;
};

struct HdPrman_RileyPrimvarList
{
    using RileyType = RtPrimVarList;

    HdPrman_RileyPrimvarList(
        HdPrmanRileyPrimvarListSchema schema,
        const GfVec2f &shutterInterval);

    RileyType rileyObject;
};

struct HdPrman_RileyShadingNodeType
{
    using RileyType = riley::ShadingNode::Type;

    HdPrman_RileyShadingNodeType(
        HdTokenDataSourceHandle const &ds);

    RileyType rileyObject;
};

struct HdPrman_RileyShadingNode
{
    using RileyType = riley::ShadingNode;
    
    HdPrman_RileyShadingNode(
        HdPrmanRileyShadingNodeSchema schema);

    RileyType rileyObject;
};

struct HdPrman_RileyShadingNetwork
{
    using RileyType = riley::ShadingNetwork;

    HdPrman_RileyShadingNetwork(
        HdPrmanRileyShadingNodeVectorSchema schema);

    std::vector<riley::ShadingNode> shadingNodes;

    RileyType rileyObject;
};

/// A (RAII) object for transform samples extracted from matrix data
/// source.
///
struct HdPrman_RileyTransform
{
    using RileyType = riley::Transform;

    HdPrman_RileyTransform(
        HdMatrixDataSourceHandle const &ds,
        const GfVec2f &shutterInterval);

    std::vector<RtMatrix4x4> matrix;
    std::vector<float> time;

    // (Non-RAII) object that can be passed to, e.g.,
    // Riley::CreateCoordinateSystem.
    RileyType rileyObject;
};

struct HdPrman_RileyFloat
{
    using RileyType = float;
    
    HdPrman_RileyFloat(
        HdFloatDataSourceHandle const &ds,
        float fallbackValue = 0.0f);

    RileyType rileyObject;
};

/// Converts token to riley string.
///
struct HdPrman_RileyString
{
    using RileyType = RtUString;

    HdPrman_RileyString(
        HdTokenDataSourceHandle const &ds);

    RileyType rileyObject;
};

struct HdPrman_RileyExtent
{
    using RileyType = riley::Extent;

    HdPrman_RileyExtent(
        HdVec3iDataSourceHandle const &ds);

    RileyType rileyObject;
};

/// Converts token data source to riley enum type RenderOutputType.
///
struct HdPrman_RileyRenderOutputType
{
    using RileyType = riley::RenderOutputType;

    HdPrman_RileyRenderOutputType(
        HdTokenDataSourceHandle const &ds);

    RileyType rileyObject;
};

struct HdPrman_RileyFilterSize
{
    using RileyType = riley::FilterSize;

    HdPrman_RileyFilterSize(
        HdVec2fDataSourceHandle const &ds);

    RileyType rileyObject;
};

/// Similar to HdPrman_RileyString but adds a unique suffix to ensure
/// uniqueness.
///
/// Recall that Riley::CreateCamera(..., name, ...) requires name to be unique
/// across all cameras. This method can be used to ensure this.
///
/// Note that even if the scene index the prim managing scene index observer
/// is observing provides unique names, we can end up with brief moments of
/// non-unique names without this method. This can happen if, e.g., we resync
/// a camera while an HdPrman_RileyRenderViewPrim is still holding on to the
/// old HdPrman_RileyCameraPrim.
///
struct HdPrman_RileyUniqueString
{
    using RileyType = RtUString;

    HdPrman_RileyUniqueString(
        HdTokenDataSourceHandle const &ds);

    RileyType rileyObject;
};

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#endif
