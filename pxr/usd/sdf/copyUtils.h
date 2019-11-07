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
#ifndef PXR_USD_SDF_COPY_UTILS_H
#define PXR_USD_SDF_COPY_UTILS_H

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
/// Variant specs may be copied to prim paths and vice versa. When copying a
/// variant to a prim, the specifier and typename from the variant's parent
/// prim will be used.
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
/// \p valueToCopy with the desired value. \p valueToCopy may also be a
/// SdfCopySpecsValueEdit that specifies an editing operation for this field. 
/// If \p valueToCopy is not set, the field value from the source spec will be 
/// used as-is. Setting \p valueToCopy to an empty VtValue indicates that the 
/// field should be removed from the destination spec, if it already exists.
///
/// Note that if this function returns true and the source spec has no value
/// for \p field (e.g., fieldInSrc == false), the field in the destination
/// spec will also be set to no value.
using SdfShouldCopyValueFn = std::function<
    bool(SdfSpecType specType, const TfToken& field,
         const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool fieldInSrc,
         const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
         boost::optional<VtValue>* valueToCopy)>;

/// \class SdfCopySpecsValueEdit
/// Value containing an editing operation for SdfCopySpecs.
///
/// The SdfShouldCopyValueFn callback allows users to return a value to copy
/// into the destination spec via the \p valueToCopy parameter. However, there
/// may be cases where it would be more efficient to perform incremental edits 
/// using specific SdfLayer API instead.
///
/// To accommodate this, consumers may provide a callback that applies a
/// scene description edit in \p valueToCopy via an SdfCopySpecsValueEdit 
/// object. 
class SdfCopySpecsValueEdit
{
public:
    /// Callback to apply a scene description edit to the specified layer and
    /// spec path.
    using EditFunction = 
        std::function<void(const SdfLayerHandle&, const SdfPath&)>;

    explicit SdfCopySpecsValueEdit(const EditFunction& edit) : _edit(edit) { }
    const EditFunction& GetEditFunction() const { return _edit; }

    /// SdfCopySpecsValueEdit objects are not comparable, but must provide
    /// operator== to be stored in a VtValue.
    bool operator==(const SdfCopySpecsValueEdit& rhs) const { return false; }
    bool operator!=(const SdfCopySpecsValueEdit& rhs) const { return true; }

private:
    EditFunction _edit;
};

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

/// SdfShouldCopyValueFn used by the simple version of SdfCopySpec.
///
/// Copies all values from the source, transforming path-valued fields prefixed
/// with \p srcRootPath to have the prefix \p dstRootPath.
///
/// Existing values in the destination will be overwritten by values in the
/// source.  Any fields in the destination that aren't in the source will be
/// cleared.
SDF_API
bool
SdfShouldCopyValue(
    const SdfPath& srcRootPath, const SdfPath& dstRootPath,
    SdfSpecType specType, const TfToken& field,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool fieldInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
    boost::optional<VtValue>* valueToCopy);

/// SdfShouldCopyChildrenFn used by the simple version of SdfCopySpec.
///
/// Copies all child values from the source, transforming path-valued fields
/// prefixed with \p srcRootPath to have the prefix \p dstRootPath.
///
/// Existing values in the destination will be overwritten by values in the
/// source.  Any fields in the destination that aren't in the source will be
/// cleared.
SDF_API
bool
SdfShouldCopyChildren(
    const SdfPath& srcRootPath, const SdfPath& dstRootPath,
    const TfToken& childrenField,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool fieldInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
    boost::optional<VtValue>* srcChildren, 
    boost::optional<VtValue>* dstChildren);

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
/// Variant specs may be copied to prim paths and vice versa. When copying a
/// variant to a prim, the specifier and typename from the variant's parent
/// prim will be used.
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

#endif // PXR_USD_SDF_COPY_UTILS_H
