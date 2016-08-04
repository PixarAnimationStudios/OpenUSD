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
/// \file sdf/relationshipSpec.h

#ifndef SDF_RELATIONSHIPSPEC_H
#define SDF_RELATIONSHIPSPEC_H

#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/types.h"

template <class TypePolicy> class Sdf_ListEditor;
template <class T> class Sdf_MarkerUtils;

/// \class SdfRelationshipSpec
/// \brief A property that contains a reference to one or more SdfPrimSpec
/// instances.
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
    SDF_DECLARE_SPEC(SdfSchema, SdfSpecTypeRelationship,
                     SdfRelationshipSpec, SdfPropertySpec);

public:
    typedef SdfRelationshipSpec This;
    typedef SdfPropertySpec Parent;

    ///
    /// \name Spec creation
    /// @{

    /// \brief Creates a new prim relationship instance.
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

    /// \brief Returns the relationship's target path list editor.
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

    /// \brief Updates the specified target path.
    ///
    /// Replaces the path given by \p oldPath with the one specified by
    /// \p newPath.  Relational attributes are updated if necessary.
    SDF_API
    void ReplaceTargetPath(const SdfPath& oldPath, const SdfPath& newPath);

    /// \brief Removes the specified target path.
    ///
    /// Removes the given target path and any relational attributes for the
    /// given target path. If \p preserveTargetOrder is \c true, Erase() is
    /// called on the list editor instead of RemoveItemEdits(). This preserves
    /// the ordered items list.
    SDF_API
    void RemoveTargetPath(const SdfPath& path, bool preserveTargetOrder = false);

    /// @}
    /// \name Relational attributes
    /// @{

    /// Gets the attributes for the given target path.
    SDF_API
    SdfRelationalAttributeSpecView
    GetAttributesForTargetPath(const SdfPath& path) const;

    /// Sets the attributes for the given target path as a vector.
    SDF_API
    void SetAttributesForTargetPath(const SdfPath& path,
                                    const SdfAttributeSpecHandleVector& newAttrs);

    /// Inserts the given attribute for the given target path.
    SDF_API
    bool InsertAttributeForTargetPath(const SdfPath& path,
                                      const SdfAttributeSpecHandle& attr,
                                      int index = -1);

    /// Removes an attribute from the given target path.
    SDF_API
    void RemoveAttributeForTargetPath(const SdfPath& path,
                                      const SdfAttributeSpecHandle& attr);

    /// Returns all target paths for which there are relational attributes.
    SDF_API
    SdfPathVector GetAttributeTargetPaths() const;

    /// Returns the target path for the given relational attribute.
    SDF_API
    SdfPath
    GetTargetPathForAttribute(const SdfAttributeSpecConstHandle& attr) const;

    /// @}
    /// \name Relational attribute ordering
    /// @{

    // A simple type used for bulk replacement of all attribute orders.
    typedef std::map< SdfPath, std::vector<TfToken> > AttributeOrderMap;

    /// Returns list of all target paths for which an ordering of relational
    /// attributes exists.
    SDF_API
    SdfPathVector GetAttributeOrderTargetPaths() const;

    /// Returns true if a relational attribute ordering is authored for the
    /// given target \p path.
    SDF_API
    bool HasAttributeOrderForTargetPath(const SdfPath& path) const;

    /// Returns a list editor proxy for authoring relational attribute
    /// orderings for the given target \p path. If no ordering exists for
    /// \p path, an invalid proxy object is returned.
    SDF_API
    SdfNameOrderProxy GetAttributeOrderForTargetPath(const SdfPath& path) const;

    /// Returns a list editor proxy for authoring relational attribute
    /// orderings for the given target \p path. This may create a relationship
    /// target spec for \p path if one does not already exist.
    SDF_API
    SdfNameOrderProxy GetOrCreateAttributeOrderForTargetPath(
        const SdfPath& path);

    /// \brief Replaces all target attribute orders with the given map.
    ///
    /// The map's keys are the target paths whose attributes should be
    /// ordered.  The values are vectors of strings specifying the
    /// ordering for each path.
    SDF_API
    void SetTargetAttributeOrders(const AttributeOrderMap& orders);

    /// \brief Reorders the given list of attribute names according to the
    /// reorder attributes statement for the given target path.
    ///
    /// This routine employs the standard list editing operation for ordered
    /// items in a ListEditor.
    SDF_API
    void ApplyAttributeOrderForTargetPath(
        const SdfPath& path, std::vector<TfToken>* vec) const;

    /// @}
    /// \name Markers
    /// @{

    // A simple type used for bulk replacement of all target markers.
    typedef std::map<SdfPath, std::string, 
                     SdfPath::FastLessThan> TargetMarkerMap;

    /// Returns a copy of all the target markers for this relationship.
    SDF_API
    TargetMarkerMap GetTargetMarkers() const;

    /// Sets the all the target markers for this relationship.
    SDF_API
    void SetTargetMarkers(const TargetMarkerMap& markers);

    /// \brief Returns the marker for this relationship for the given target 
    /// path.
    SDF_API
    std::string GetTargetMarker(const SdfPath& path) const;

    /// \brief Sets the marker for this relationship for the given target
    /// path. 
    /// 
    /// If an empty string is specified, the target marker will be cleared.
    SDF_API
    void SetTargetMarker(const SdfPath& path, const std::string& marker);

    /// \brief Clears the marker for the given target path.
    SDF_API
    void ClearTargetMarker(const SdfPath& path);

    /// Returns all target paths on which markers are specified.
    SDF_API
    SdfPathVector GetTargetMarkerPaths() const;

    /// \brief Get whether loading the target of this relationship is necessary
    /// to load the prim we're attached to
    SDF_API
    bool GetNoLoadHint(void) const;

    /// \brief Set whether loading the target of this relationship is necessary
    /// to load the prim we're attached to
    SDF_API
    void SetNoLoadHint(bool noload);

    /// @}

private:
    SdfPath _CanonicalizeTargetPath(const SdfPath& path) const;

    SdfPath _MakeCompleteTargetSpecPath(const SdfPath& srcPath) const;

    SdfSpecHandle _GetTargetSpec(const SdfPath& path) const;

    SdfSpecHandle _FindOrCreateTargetSpec(const SdfPath& path);

    boost::shared_ptr<Sdf_ListEditor<SdfNameTokenKeyPolicy> >
    _GetTargetAttributeOrderEditor(const SdfPath& path) const;

    SdfSpecHandle _FindOrCreateChildSpecForMarker(const SdfPath& key);

    // Allow access to functions for creating child specs for markers.
    friend class Sdf_MarkerUtils<SdfRelationshipSpec>;

    // Allow access to _GetTarget() for the  relational attribute c'tor
    friend class SdfAttributeSpec;

    // Allow access to retrieve relationship spec for this API object.
    friend class Sdf_PyRelationshipAccess;
};

#endif // SDF_RELATIONSHIPSPEC_H
