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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_RELATIONSHIP_SPEC_H
