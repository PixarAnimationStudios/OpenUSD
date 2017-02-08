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
#ifndef _USDUTILS_AUTHORING_H_
#define _USDUTILS_AUTHORING_H_

/// \file usdUtils/authoring.h 
///
/// A collection of utilities for higher-level authoring and copying scene
/// description than provided by the core Usd and Sdf API's

#include "pxr/pxr.h"
#include "pxr/usd/sdf/declareHandles.h"

PXR_NAMESPACE_OPEN_SCOPE


SDF_DECLARE_HANDLES(SdfLayer);

/// Given two layers \p source and \p destination, copy the authored metadata
/// from one to the other.  By default, copy **all** authored metadata;
/// however, you can skip certain classes of metadata with the parameter
/// \p skipSublayers, which will prevent copying subLayers or subLayerOffsets
///
/// Makes no attempt to clear metadata that may already be authored in
/// \p destination, but any fields that are already in \p destination but also
/// in \p source will be replaced.
///
/// \return \c true on success, \c false on error.
bool UsdUtilsCopyLayerMetadata(const SdfLayerHandle &source,
                               const SdfLayerHandle &destination,
                               bool skipSublayers = false);


PXR_NAMESPACE_CLOSE_SCOPE

#endif /* _USDUTILS_PIPELINE_H_ */
