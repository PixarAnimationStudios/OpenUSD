//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_RELATIONSHIP_SPEC_H
#define PXR_USD_SDF_RELATIONSHIP_SPEC_H

/// \file sdf/relationshipSpec.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfRelationshipSpec
///
/// A property that contains a reference to one or more SdfPrimSpec instances.
///
/// A relationship may refer to one or more target prims or attributes.
/// All targets of a single relationship are considered to be playing the same
/// role. Note that \c role does not imply that the target prims or attributes
/// are of the same \c type.
///
/// Relationships may be annotated with relational attributes.
/// Relational attributes are named SdfAttributeSpec objects containing
/// values that describe the relationship.  For example, point weights are
/// commonly expressed as relational attributes.
///
class SdfRelationshipSpec : public SdfPropertySpec
{
    SDF_DECLARE_SPEC(SdfRelationshipSpec, SdfPropertySpec);

public:
    typedef SdfRelationshipSpec This;
    typedef SdfPropertySpec Parent;

    ///
    /// \name Spec creation
    /// @{

    /// Creates a new prim relationship instance.
    ///
    /// Creates and returns a new relationship for the given prim.
    /// The \p owner will own the newly created relationship.
    SDF_API
    static SdfRelationshipSpecHandle
    New(const SdfPrimSpecHandle& owner,
        const std::string& name,
        bool custom = true,
        SdfVariability variability = SdfVariabilityUniform);

    /// @}

    /// \name Relationship targets
    /// @{

    /// Returns the relationship's target path list editor.
    ///
    /// The list of the target paths for this relationship may be modified
    /// through the proxy.
    SDF_API
    SdfTargetsProxy GetTargetPathList() const;

    /// Returns true if the relationship has any target paths.
    SDF_API
    bool HasTargetPathList() const;

    /// Clears the list of target paths on this relationship.
    SDF_API
    void ClearTargetPathList() const;

    /// Updates the specified target path.
    ///
    /// Replaces the path given by \p oldPath with the one specified by
    /// \p newPath.  Relational attributes are updated if necessary.
    SDF_API
    void ReplaceTargetPath(const SdfPath& oldPath, const SdfPath& newPath);

    /// Removes the specified target path.
    ///
    /// Removes the given target path and any relational attributes for the
    /// given target path. If \p preserveTargetOrder is \c true, Erase() is
    /// called on the list editor instead of RemoveItemEdits(). This preserves
    /// the ordered items list.
    SDF_API
    void RemoveTargetPath(const SdfPath& path, bool preserveTargetOrder = false);

    /// @}

    /// Get whether loading the target of this relationship is necessary
    /// to load the prim we're attached to
    SDF_API
    bool GetNoLoadHint(void) const;

    /// Set whether loading the target of this relationship is necessary
    /// to load the prim we're attached to
    SDF_API
    void SetNoLoadHint(bool noload);

private:
    SdfPath _CanonicalizeTargetPath(const SdfPath& path) const;

    SdfPath _MakeCompleteTargetSpecPath(const SdfPath& srcPath) const;

    SdfSpecHandle _GetTargetSpec(const SdfPath& path) const;

    // Allow access to _GetTarget() for the  relational attribute c'tor
    friend class SdfAttributeSpec;

    // Allow access to retrieve relationship spec for this API object.
    friend class Sdf_PyRelationshipAccess;
};

/// Convenience function to create a relationshipSpec on a primSpec at the
/// given path, and any necessary parent primSpecs, in the given layer.
///
/// If a relationshipSpec already exists at the given path, author
/// variability and custom according to passed arguments and return
/// a relationship spec handle.
///
/// Any newly created prim specs have SdfSpecifierOver and an empty type (as if
/// created by SdfJustCreatePrimInLayer()).  relPath must be a valid prim
/// property path (see SdfPath::IsPrimPropertyPath()).  Return false and issue
/// an error if we fail to author the required scene description.
SDF_API
SdfRelationshipSpecHandle
SdfCreateRelationshipInLayer(
    const SdfLayerHandle &layer,
    const SdfPath &relPath,
    SdfVariability variability = SdfVariabilityVarying,
    bool isCustom = false);

/// Convenience function to create a relationshipSpec on a primSpec at the
/// given path, and any necessary parent primSpecs, in the given layer.
///
/// If a relationshipSpec already exists at the given path, author
/// variability and custom according to passed arguments and return
/// a relationship spec handle.
///
/// Any newly created prim specs have SdfSpecifierOver and an empty type (as if
/// created by SdfJustCreatePrimInLayer()).  relPath must be a valid prim
/// property path (see SdfPath::IsPrimPropertyPath()).  Return false and issue
/// an error if we fail to author the required scene description.
///
/// Differs only from SdfCreateRelationshipInLayer only in that a
/// bool, not a handle, is returned.
SDF_API
bool
SdfJustCreateRelationshipInLayer(
    const SdfLayerHandle &layer,
    const SdfPath &relPath,
    SdfVariability variability = SdfVariabilityVarying,
    bool isCustom = false);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_RELATIONSHIP_SPEC_H
