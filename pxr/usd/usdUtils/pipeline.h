//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_UTILS_PIPELINE_H
#define PXR_USD_USD_UTILS_PIPELINE_H

/// \file usdUtils/pipeline.h
///
/// Collection of module-scoped utilities for establishing pipeline
/// conventions for things not currently suitable or possible to canonize in
/// USD's schema modules.

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/api.h"
#include "pxr/usd/usdUtils/registeredVariantSet.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/common.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

USDUTILS_API
extern TfEnvSetting<bool> USD_FORCE_DEFAULT_MATERIALS_SCOPE_NAME;

/// Define the shading pipeline's convention for naming a companion
/// alpha/opacity attribute and primvarnames given the full name of a
/// color-valued attribute
USDUTILS_API
TfToken UsdUtilsGetAlphaAttributeNameForColor(TfToken const &colorAttrName);

/// Returns the model name associated with a given root layer. In order,
/// it looks for defaultPrim metadata, a prim matching the filename,
/// and then the first concrete root prim.
USDUTILS_API
TfToken UsdUtilsGetModelNameFromRootLayer(const SdfLayerHandle& rootLayer);

/// Certain variant sets can be registered with the system.
///
/// Returns the set of UsdUtilsRegisteredVariantSet objects that are registered
/// with the pipeline. 
///
/// Variant sets can be registered through direct enumeration inside a
/// `plugInfo.json`, or via a plugin for situations that require dynamic
/// configuration at runtime.
///
/// This list will be empty until one or more `plugInfo.json` files
/// discoverable by your USD installation contain an entry in the
/// UsdUtilsPipeline group like the following:
/// \code{json}
///    "UsdUtilsPipeline": {
///        "RegisteredVariantSets": {
///            "modelingVariant": {
///                "selectionExportPolicy": "always"
///            },
///            "standin": {
///                "selectionExportPolicy": "never"
///            }
///        }
///    }    
/// \endcode
///
/// After the above variantSets are registered, this will then load any plugin 
/// that has a `plugInfo.json` with:
///
/// \code{json}
///     "UsdUtilsPipeline": {
///         "RegistersVariantSets": true
///     }
/// \endcode
///
/// This plugin should then have code that registers code to run for
/// `UsdUtilsRegisteredVariantSet`:
/// \code{cpp}
/// TF_REGISTRY_FUNCTION(UsdUtilsRegisteredVariantSet) {
///   std::string variantSetName = ...;
///   UsdUtilsRegisteredVariantSet::SelectionExportPolicy exportPolicy = ...;
///   UsdUtilsRegisterVariantSet(variantSetName, exportPolicy);
/// }
/// \endcode
///
/// \sa UsdUtilsRegisterVariantSet 
USDUTILS_API
const std::set<UsdUtilsRegisteredVariantSet>& UsdUtilsGetRegisteredVariantSets();

/// Registers \p variantSetName with \p selectionExportPolicy.
///
/// \sa UsdUtilsGetRegisteredVariantSets
USDUTILS_API
void UsdUtilsRegisterVariantSet(
    const std::string& variantSetName,
    const UsdUtilsRegisteredVariantSet::SelectionExportPolicy&
        selectionExportPolicy);

/// If a valid UsdPrim already exists at \p path on the USD stage \p stage, 
/// returns it. It not, it checks to see if the path belongs to a prim 
/// underneath an instance and returns the corresponding prototype prim. 
/// 
/// This returns an invalid UsdPrim if no corresponding prototype prim can be 
/// found and if no prim exists at the path.
///
/// This method is similar to UsdStage::GetPrimAtPath(), in that it will never 
/// author scene description, and therefore is safe to use as a "reader" in the 
/// Usd multi-threading model.
USDUTILS_API
UsdPrim UsdUtilsGetPrimAtPathWithForwarding(const UsdStagePtr &stage, 
                                            const SdfPath &path);

/// Given a path, uninstances all the instanced prims in the namespace chain and
/// returns the resulting prim at the requested path. Returns a NULL prim if the
/// given path doesn't exist and does not correspond to a valid prim inside a
/// prototype.
USDUTILS_API
UsdPrim UsdUtilsUninstancePrimAtPath(const UsdStagePtr &stage, 
                                     const SdfPath &path);

/// Returns the name of the primary UV set used on meshes and nurbs.
/// By default the name is "st".
USDUTILS_API
const TfToken& UsdUtilsGetPrimaryUVSetName();

/// Returns the name of the reference position used on meshes and nurbs.
/// By default the name is "pref".
USDUTILS_API
const TfToken& UsdUtilsGetPrefName();

/// Get the name of the USD prim under which materials are expected to be
/// authored.
///
/// The scope name can be configured in the metadata of a plugInfo.json file
/// like so:
/// \code{json}
///    "UsdUtilsPipeline": {
///        "MaterialsScopeName": "SomeScopeName"
///    }
/// \endcode
///
/// If \p forceDefault is true, any value specified in a plugInfo.json will be
/// ignored and the built-in default will be returned. This is primarily used
/// for unit testing purposes as a way to ignore any site-based configuration.
USDUTILS_API
TfToken UsdUtilsGetMaterialsScopeName(const bool forceDefault = false);

/// Get the name of the USD prim representing the primary camera.
/// By default the name is "main_cam".
///
/// The camera name can be configured in the metadata of a plugInfo.json file
/// like so:
/// \code{json}
///    "UsdUtilsPipeline": {
///        "PrimaryCameraName": "SomeCameraName"
///    }
/// \endcode
///
/// If \p forceDefault is true, any value specified in a plugInfo.json will be
/// ignored and the built-in default will be returned. This is primarily used
/// for unit testing purposes as a way to ignore any site-based configuration.
USDUTILS_API
TfToken UsdUtilsGetPrimaryCameraName(const bool forceDefault = false);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
