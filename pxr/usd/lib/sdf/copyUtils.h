//
// Copyright 2017 Pixar
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
#ifndef SDF_COPY_UTILS_H
#define SDF_COPY_UTILS_H

/// \file sdf/copyUtils.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/types.h"

#include <boost/optional.hpp>
#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;
class TfToken;
class VtValue;
SDF_DECLARE_HANDLES(SdfLayer);

/// \name Simple Spec Copying API
/// @{

/// Utility function for copying spec data at \p srcPath in \p srcLayer to
/// \p destPath in \p destLayer.
///
/// Copying is performed recursively: all child specs are copied as well.
/// Any destination specs that already exist will be overwritten.
///
/// Parent specs of the destination are not created, and must exist before
/// SdfCopySpec is called, or a coding error will result.  For prim parents,
/// clients may find it convenient to call SdfCreatePrimInLayer before
/// SdfCopySpec.
///
/// As a special case, if the top-level object to be copied is a relationship
/// target or a connection, the destination spec must already exist.  That is
/// because we don't want SdfCopySpec to impose any policy on how list edits are
/// made; client code should arrange for relationship targets and connections to
/// be specified as prepended, appended, deleted, and/or ordered, as needed.
///
/// Attribute connections, relationship targets, inherit and specializes paths,
/// and internal sub-root references that target an object beneath \p srcPath 
/// will be remapped to target objects beneath \p dstPath.
///
SDF_API
bool
SdfCopySpec(
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath);

/// @}

/// \name Advanced Spec Copying API
/// @{

/// Return true if \p field should be copied from the spec at \p srcPath in
/// \p srcLayer to the spec at \p dstPath in \p dstLayer. \p fieldInSrc and
/// \p fieldInDst indicates whether the field has values at the source and
/// destination specs. Return false otherwise.
///
/// This function may modify the value that is copied by filling in 
/// \p valueToCopy with the desired value. If this field is not set, the 
/// field from the source spec will be used as-is. Setting \p valueToCopy 
/// to an empty VtValue indicates that the field should be removed from the 
/// destination spec, if it already exists.
///
/// Note that if this function returns true and the source spec has no value
/// for \p field (e.g., fieldInSrc == false), the field in the destination
/// spec will also be set to no value.
using SdfShouldCopyValueFn = std::function<
    bool(SdfSpecType specType, const TfToken& field,
         const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool fieldInSrc,
         const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
         boost::optional<VtValue>* valueToCopy)>;

/// Return true if \p childrenField and the child objects the field represents
/// should be copied from the spec at \p srcPath in \p srcLayer to the spec at 
/// \p dstPath in \p dstLayer. \p fieldInSrc and \p fieldInDst indicates 
/// whether that field has values at the source and destination specs. 
/// Return false otherwise.
///
/// This function may modify which children are copied by filling in
/// \p srcChildren and \p dstChildren will the children to copy and their
/// destination. Both of these values must be set, and must contain the same
/// number of children.
/// 
/// Note that if this function returns true and the source spec has no value 
/// for \p childrenField (e.g., fieldInSrc == false), the field in the 
/// destination spec will also be set to no value, causing any existing children
/// to be removed.
using SdfShouldCopyChildrenFn = std::function<
    bool(const TfToken& childrenField,
         const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool fieldInSrc,
         const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
         boost::optional<VtValue>* srcChildren, 
         boost::optional<VtValue>* dstChildren)>;

/// Utility function for copying spec data at \p srcPath in \p srcLayer to
/// \p destPath in \p destLayer. Various behaviors (such as which parts of the
/// spec to copy) are controlled by the supplied \p shouldCopyValueFn and
/// \p shouldCopyChildrenFn.
///
/// Copying is performed recursively: all child specs are copied as well, except
/// where prevented by \p shouldCopyChildrenFn.
///
/// Parent specs of the destination are not created, and must exist before
/// SdfCopySpec is called, or a coding error will result.  For prim parents,
/// clients may find it convenient to call SdfCreatePrimInLayer before
/// SdfCopySpec.
///
/// As a special case, if the top-level object to be copied is a relationship
/// target or a connection, the destination spec must already exist.  That is
/// because we don't want SdfCopySpec to impose any policy on how list edits are
/// made; client code should arrange for relationship targets and connections to
/// be specified as prepended, appended, deleted, and/or ordered, as needed.
///
SDF_API
bool 
SdfCopySpec(
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath,
    const SdfShouldCopyValueFn& shouldCopyValueFn,
    const SdfShouldCopyChildrenFn& shouldCopyChildrenFn);

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_COPY_UTILS_H
