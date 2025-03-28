//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_RENDER_SPEC_H
#define PXR_USD_USD_RENDER_SPEC_H

/// \file usdRender/spec.h

#include "pxr/pxr.h"
#include "pxr/usd/usdRender/api.h"
#include "pxr/usd/usdRender/settingsBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdRender/tokens.h"

#include "pxr/base/gf/frustum.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdRenderSettings;

/// A self-contained specification of render settings.
/// \note This is preliminary API and is likely to change.
struct UsdRenderSpec {
    /// Specification of a product.  See UsdRenderProduct.
    struct Product {
        /// The path of this product, which uniquely identifies it.
        SdfPath renderProductPath;
        /// The type of product, ex: "raster".
        TfToken type;
        /// The name of the product, which uniquely identifies it.
        TfToken name;
        /// Path to the primary UsdGeomCamera camera to use for this product.
        SdfPath cameraPath;
        /// If set to true, disables motion blur.
        bool disableMotionBlur;
        /// If set to true, disables depth of field.
        bool disableDepthOfField;
        /// The pixel resolution of the product.
        GfVec2i resolution = GfVec2i(0);
        /// The pixel aspect ratio as adjusted by aspectRatioConformPolicy.
        float pixelAspectRatio;
        /// The policy that was applied to conform aspect ratio
        /// mismatches between the aperture and image.
        TfToken aspectRatioConformPolicy;
        /// The camera aperture size as adjusted by aspectRatioConformPolicy.
        GfVec2f apertureSize = GfVec2f(0);
        /// The data window, in NDC terms relative to the aperture.
        /// (0,0) corresponds to bottom-left and (1,1) corresponds to
        /// top-right.  Note that the data window can partially cover
        /// or extend beyond the unit range, for representing overscan
        /// or cropped renders.
        GfRange2f dataWindowNDC;
        /// The render vars used by this product, as indices into the
        /// top-level renderVars array.
        std::vector<size_t> renderVarIndices;
        /// Any extra settings values discovered in requested namespaces.
        VtDictionary namespacedSettings;
    };
    /// Specification of a render variable (aka AOV).  See UsdRenderVar.
    struct RenderVar {
        /// The path of this render var, which uniquely identifies it.
        SdfPath renderVarPath;
        /// The value data type of the variable, as a USD type name.
        TfToken dataType;
        std::string sourceName;
        TfToken sourceType;
        /// Any extra settings values discovered in requested namespaces.
        VtDictionary namespacedSettings;
    };
    /// The full list of products requested by this render.
    std::vector<Product> products;
    /// The full list of render vars requested by products in this render.
    std::vector<RenderVar> renderVars;
    /// List of purposes to use to filter scene contents.
    VtArray<TfToken> includedPurposes;
    /// List of material binding purposes.
    VtArray<TfToken> materialBindingPurposes;
    /// Any extra settings values discovered in requested namespaces.
    VtDictionary namespacedSettings;
};

/// Computes the specification of the render settings.
/// For each product, applies the aspectRatioConformPolicy
/// and computes a final screenWindow and pixelAspectRatio.
///
/// Any other attributes encountered are returned in namespacedSettings.
/// If a non-empty list of namespaces is provided, only attributes
/// within those namespaces are returned.
/// If an empty list of namespaces is provided, all custom
/// (non-schema) attributes are returned.
/// The same list of namespaces is used for finding namespacedSettings
/// in all UsdRender prim types.
USDRENDER_API
UsdRenderSpec
UsdRenderComputeSpec(UsdRenderSettings const& settings,
                     TfTokenVector const& namespaces);

/// Returns a dictionary populated with attributes filtered by the namespaces.
/// If a non-empty list of namespaces is provided, only authored attributes
/// within those namespaces are returned.
/// If an empty list of namespaces is provided, all custom (non-schema)
/// attributes are returned.
/// \note Special handling is provided for connectable attributes that are used
///       to represent node graph outputs.
USDRENDER_API
VtDictionary
UsdRenderComputeNamespacedSettings(UsdPrim const& prim,
                                   TfTokenVector const& namespaces);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
