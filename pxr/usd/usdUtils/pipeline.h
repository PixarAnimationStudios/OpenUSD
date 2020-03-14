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
/// \sa UsdUtilsRegisteredVariantSet
///

/// Returns the set of UsdUtilsRegisteredVariantSet objects that are registered
/// with the pipeline. 
///
/// This list will be empty until one or more plugInfo.json files
/// discoverable by your USD installation contain an entry in the
/// UsdUtilsPipeline group like the following:
/// \code{json}
///    "UsdUtilsPipeline": {
///        "RegisteredVariantSets": [
///            "modelingVariant": {
///                "selectionExportPolicy": {
///                    "always"
///                }
///            },
///            "standin": {
///                "selectionExportPolicy": {
///                    "never"
///                }
///            }
///        ]
///    }
/// \endcode
USDUTILS_API
const std::set<UsdUtilsRegisteredVariantSet>& UsdUtilsGetRegisteredVariantSets();

/// If a valid UsdPrim already exists at \p path on the USD stage \p stage, 
/// returns it. It not, it checks to see if the path belongs to a prim 
/// underneath an instance and returns the corresponding master prim. 
/// 
/// This returns an invalid UsdPrim if no corresponding master prim can be 
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
/// master.
USDUTILS_API
UsdPrim UsdUtilsUninstancePrimAtPath(const UsdStagePtr &stage, 
                                     const SdfPath &path);

/// Returns the name of the primary UV set used on meshes and nurbs.
/// By default the name is "st".
USDUTILS_API
TfToken UsdUtilsGetPrimaryUVSetName();

/// Returns the name of the reference position used on meshes and nurbs.
/// By default the name is "pref".
USDUTILS_API
TfToken UsdUtilsGetPrefName();

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
