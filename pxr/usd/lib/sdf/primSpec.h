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
#ifndef SDF_PRIMSPEC_H
#define SDF_PRIMSPEC_H

/// \file sdf/primSpec.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/tf/declarePtrs.h"

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

template <class TypePolicy> class Sdf_ListEditor;

/// \class SdfPrimSpec
///
/// Represents a prim description in an SdfLayer object.
///
/// Every SdfPrimSpec object is defined in a layer.  It is identified by its
/// path (SdfPath class) in the namespace hierarchy of its layer.
///
/// SdfPrimSpec objects have properties of two general types: attributes
/// (containing values) and relationships (different types of connections to
/// other prims and attributes).  Attributes are represented by the
/// SdfAttributeSpec class and relationships by the SdfRelationshipSpec class.
/// Each prim has its own namespace of properties.  Properties are stored and
/// accessed by their name.
///
/// SdfPrimSpec objects have a typeName, permission restriction, and they
/// reference and inherit prim paths.  Permission restrictions control which
/// other layers may refer to, or express opinions about a prim. See the
/// SdfPermission class for more information.
///
/// Compared with Menv2x, prims are most closely analogous to hooks and cues.
///
/// \todo
/// \li Insert doc about references and inherits here.
/// \li Should have validate... methods for name, children, properties
///
class SdfPrimSpec : public SdfSpec
{
    SDF_DECLARE_SPEC(SdfSchema, SdfSpecTypePrim, SdfPrimSpec, SdfSpec);

public:
    typedef SdfPrimSpecView NameChildrenView;
    typedef SdfPropertySpecView PropertySpecView;
    typedef SdfAttributeSpecView AttributeSpecView;
    typedef SdfRelationshipSpecView RelationshipSpecView;

    ///
    /// \name Spec creation
    /// @{

    /// Create a root prim spec.
    ///
    /// Creates a prim spec with a \p name, \p specifier and \p typeName as a
    /// root prim in the given layer.
    SDF_API
    static SdfPrimSpecHandle
    New(const SdfLayerHandle& parentLayer,
        const std::string& name, SdfSpecifier spec,
        const std::string& typeName = std::string());

    /// Create a prim spec.
    ///
    /// Creates a prim spec with a \p name, \p specifier and \p typeName as
    /// a namespace child of the given prim.
    SDF_API
    static SdfPrimSpecHandle
    New(const SdfPrimSpecHandle& parentPrim,
        const std::string& name, SdfSpecifier spec,
        const std::string& typeName = std::string());

    /// \name Name
    /// @{

    /// Returns the prim's name.
    SDF_API
    const std::string& GetName() const;

    /// Returns the prim's name, as a token.
    SDF_API
    TfToken GetNameToken() const;

    /// Returns true if setting the prim spec's name to \p newName will
    /// succeed.
    ///
    /// Returns false if it won't, and sets \p whyNot with a string
    /// describing why not.
    SDF_API
    bool CanSetName(const std::string& newName, std::string* whyNot) const;

    /// Sets the prim's name.
    ///
    /// Children prims must be unique by name. It is an error to
    /// set the name to the same name as an existing child of this
    /// prim's parent.
    ///
    /// Setting validate to false, will skip validation of the \p newName
    /// (that is, CanSetName will not be called).
    ///
    /// Returns true if successful, false otherwise.
    SDF_API
    bool SetName(const std::string& newName, bool validate = true);

    /// Returns true if the given string is a valid prim name.
    SDF_API
    static bool IsValidName(const std::string& name);

    /// @}
    /// \name Namespace hierarchy
    /// @{

    /// Returns the prim's namespace pseudo-root prim.
    SDF_API
    SdfPrimSpecHandle GetNameRoot() const;

    /// Returns the prim's namespace parent.
    ///
    /// This does not return the pseudo-root for root prims.  Most
    /// algorithms that scan the namespace hierarchy upwards don't
    /// want to process the pseudo-root the same way as actual prims.
    /// Algorithms that do can always call \c GetRealNameParent().
    SDF_API
    SdfPrimSpecHandle GetNameParent() const;

    /// Returns the prim's namespace parent.
    SDF_API
    SdfPrimSpecHandle GetRealNameParent() const;

    /// Returns a keyed vector view of the prim's namespace children.
    SDF_API
    NameChildrenView GetNameChildren() const;

    /// Updates nameChildren to match the given vector of prims.
    SDF_API
    void SetNameChildren(const SdfPrimSpecHandleVector&);

    /// Inserts a child.
    ///
    /// \p index is ignored except for range checking;  -1 is permitted.
    ///
    /// Returns true if successful, false if failed.
    SDF_API
    bool InsertNameChild(const SdfPrimSpecHandle& child, int index = -1);

    /// Removes the child.  Returns true if successful, false if failed.
    SDF_API
    bool RemoveNameChild(const SdfPrimSpecHandle& child);

    /// Returns the list of child names for this prim's reorder.
    /// nameChildren statement.
    ///
    /// See SetNameChildrenOrder() for more info.
    SDF_API
    SdfNameChildrenOrderProxy GetNameChildrenOrder() const;

    /// Returns true if this prim has name children order specified
    SDF_API
    bool HasNameChildrenOrder() const;

    /// Given a list of (possibly sparse) child names, authors a reorder
    /// nameChildren statement for this prim.
    ///
    /// The reorder statement can modify the order of name children
    /// during composition.  This order doesn't affect GetNameChildren(),
    /// InsertNameChild(), SetNameChildren(), et al.
    SDF_API
    void SetNameChildrenOrder(const std::vector<TfToken>& names);

    /// Adds a new name child \p name in the name children order.
    /// If \p index is -1, the name is inserted at the end.
    SDF_API
    void InsertInNameChildrenOrder(const TfToken& name, int index = -1);

    /// Removes a name child name from the name children order.
    SDF_API
    void RemoveFromNameChildrenOrder(const TfToken& name);

    /// Removes a name child name from the name children order by index.
    SDF_API
    void RemoveFromNameChildrenOrderByIndex(int index);

    /// Reorders the given list of child names according to the reorder
    /// nameChildren statement for this prim.
    ///
    /// This routine employs the standard list editing operation for ordered
    /// items in a ListEditor.
    SDF_API
    void ApplyNameChildrenOrder(std::vector<TfToken>* vec) const;

    /// @}
    /// \name Properties
    /// @{

    /// Returns the prim's properties.
    SDF_API
    PropertySpecView GetProperties() const;

    /// Updates properties to match the given vector of properties.
    SDF_API
    void SetProperties(const SdfPropertySpecHandleVector&);

    /// Inserts a property.
    ///
    /// \p index is ignored except for range checking;  -1 is permitted.
    ///
    /// Returns true if successful, false if failed.
    SDF_API
    bool InsertProperty(const SdfPropertySpecHandle& property, int index = -1);

    /// Removes the property.
    SDF_API
    void RemoveProperty(const SdfPropertySpecHandle& property);

    /// Returns a view of the attributes of this prim.
    SDF_API
    AttributeSpecView GetAttributes() const;

    /// Returns a view of the relationships of this prim.
    SDF_API
    RelationshipSpecView GetRelationships() const;

    /// Returns the list of property names for this prim's reorder
    /// properties statement.
    ///
    /// See SetPropertyOrder() for more info.
    SDF_API
    SdfPropertyOrderProxy GetPropertyOrder() const;

    /// Returns true if this prim has a property ordering specified.
    SDF_API
    bool HasPropertyOrder() const;

    /// Given a list of (possibly sparse) property names, authors a
    /// reorder properties statement for this prim.
    ///
    /// The reorder statement can modify the order of properties during
    /// composition.  This order doesn't affect GetProperties(),
    /// InsertProperty(), SetProperties(), et al.
    SDF_API
    void SetPropertyOrder(const std::vector<TfToken>& names);

    /// Add a new property \p name in the property order.
    /// If \p index is -1, the name is inserted at the end.
    SDF_API
    void InsertInPropertyOrder(const TfToken& name, int index = -1);

    /// Remove a property name from the property order.
    SDF_API
    void RemoveFromPropertyOrder(const TfToken& name);

    /// Remove a property name from the property order by index.
    SDF_API
    void RemoveFromPropertyOrderByIndex(int index);

    /// Reorders the given list of property names according to the
    /// reorder properties statement for this prim.
    ///
    /// This routine employs the standard list editing operation for ordered
    /// items in a ListEditor.
    SDF_API
    void ApplyPropertyOrder(std::vector<TfToken>* vec) const;

    /// @}
    /// \name Lookup
    /// @{

    /// Returns the object for the given \p path.
    ///
    /// If \p path is relative then it will be interpreted as
    /// relative to this prim.  If it is absolute then it will be
    /// interpreted as absolute in this prim's layer.
    ///
    /// Returns invalid handle if there is no object at \p path.
    SDF_API
    SdfSpecHandle GetObjectAtPath(const SdfPath& path) const;

    /// Returns a prim given its \p path.
    ///
    /// Returns invalid handle if there is no prim at \p path.
    /// This is simply a more specifically typed version of GetObjectAtPath.
    SDF_API
    SdfPrimSpecHandle GetPrimAtPath(const SdfPath& path) const;

    /// Returns a property given its \p path.
    ///
    /// Returns invalid handle if there is no property at \p path.
    /// This is simply a more specifically typed version of GetObjectAtPath.
    SDF_API
    SdfPropertySpecHandle GetPropertyAtPath(const SdfPath& path) const;

    /// Returns an attribute given its \p path.
    ///
    /// Returns invalid handle if there is no attribute at \p path.
    /// This is simply a more specifically typed version of GetObjectAtPath.
    SDF_API
    SdfAttributeSpecHandle GetAttributeAtPath(const SdfPath& path) const;

    /// Returns a relationship given its \p path.
    ///
    /// Returns invalid handle if there is no relationship at \p path.
    /// This is simply a more specifically typed version of GetObjectAtPath.
    SDF_API
    SdfRelationshipSpecHandle GetRelationshipAtPath(const SdfPath& path) const;

    /// @}
    /// \name Metadata
    /// @{

    /// Returns the typeName of the model prim.
    ///
    /// For prims this specifies the sub-class of MfPrim that
    /// this prim describes.
    ///
    /// The default value for typeName is the empty token.
    SDF_API
    TfToken GetTypeName() const;

    /// Sets the typeName of the model prim.
    SDF_API
    void SetTypeName(const std::string& value);

    /// Returns the comment string for this prim spec.
    ///
    /// The default value for comment is @"".
    SDF_API
    std::string GetComment() const;

    /// Sets the comment string for this prim spec.
    SDF_API
    void SetComment(const std::string& value);

    /// Returns the documentation string for this prim spec.
    ///
    /// The default value for documentation is @"".
    SDF_API
    std::string GetDocumentation() const;

    /// Sets the documentation string for this prim spec.
    SDF_API
    void SetDocumentation(const std::string& value);

    /// Returns whether this prim spec is active.
    ///
    /// The default value for active is true.
    SDF_API
    bool GetActive() const;

    /// Sets whether this prim spec is active.
    SDF_API
    void SetActive(bool value);

    /// Returns true if this prim spec has an opinion about active.
    SDF_API
    bool HasActive() const;

    /// Removes the active opinion in this prim spec if there is one.
    SDF_API
    void ClearActive();

    /// Returns whether this prim spec will be hidden in browsers.
    ///
    /// The default value for hidden is false.
    SDF_API
    bool GetHidden() const;

    /// Sets whether this prim spec will be hidden in browsers.
    SDF_API
    void SetHidden( bool value );

    /// Returns this prim spec's kind.
    ///
    /// The default value for kind is an empty \c TfToken.
    SDF_API
    TfToken GetKind() const;

    /// Sets this prim spec's kind.
    SDF_API
    void SetKind(const TfToken& value);

    /// Returns true if this prim spec has an opinion about kind.
    SDF_API
    bool HasKind() const;

    /// Remove the kind opinion from this prim spec if there is one.
    SDF_API
    void ClearKind();

    /// Returns the symmetry function for this prim.
    ///
    /// The default value for symmetry function is an empty token.
    SDF_API
    TfToken GetSymmetryFunction() const;

    /// Sets the symmetry function for this prim.
    ///
    /// If \p functionName is an empty token, then this removes any symmetry
    /// function for the given prim.
    SDF_API
    void SetSymmetryFunction(const TfToken& functionName);

    /// Returns the symmetry arguments for this prim.
    ///
    /// The default value for symmetry arguments is an empty dictionary.
    SDF_API
    SdfDictionaryProxy GetSymmetryArguments() const;

    /// Sets a symmetry argument for this prim.
    ///
    /// If \p value is empty, then this removes the setting
    /// for the given symmetry argument \p name.
    SDF_API
    void SetSymmetryArgument(const std::string& name, const VtValue& value);

    /// Returns the symmetric peer for this prim.
    ///
    /// The default value for symmetric peer is an empty string.
    SDF_API
    std::string GetSymmetricPeer() const;

    /// Sets a symmetric peer for this prim.
    ///
    /// If \p peerName is empty, then this removes the symmetric peer
    /// for this prim.
    SDF_API
    void SetSymmetricPeer(const std::string& peerName);

    /// Returns the prefix string for this prim spec.
    ///
    /// The default value for prefix is "".
    SDF_API
    std::string GetPrefix() const;

    /// Sets the prefix string for this prim spec.
    SDF_API
    void SetPrefix(const std::string& value);

    /// Returns the suffix string for this prim spec.
    ///
    /// The default value for suffix is "".
    SDF_API
    std::string GetSuffix() const;

    /// Sets the suffix string for this prim spec.
    SDF_API
    void SetSuffix(const std::string& value);

    /// Returns the custom data for this prim.
    ///
    /// The default value for custom data is an empty dictionary.
    ///
    /// Custom data is for use by plugins or other non-tools supplied
    /// extensions that need to be able to store data attached to arbitrary
    /// scene objects.  Note that if the only objects you want to store data
    /// on are prims, using custom attributes is probably a better choice.
    /// But if you need to possibly store this data on attributes or
    /// relationships or as annotations on reference arcs, then custom data
    /// is an appropriate choice.
    SDF_API
    SdfDictionaryProxy GetCustomData() const;

    /// Returns the asset info dictionary for this prim.
    /// 
    /// The default value is an empty dictionary. 
    /// 
    /// The asset info dictionary is used to annotate prims representing the 
    /// root-prims of assets (generally organized as models) with various 
    /// data related to asset management. For example, asset name, root layer
    /// identifier, asset version etc.
    /// 
    SDF_API
    SdfDictionaryProxy GetAssetInfo() const;

    /// Sets a custom data entry for this prim.
    ///
    /// If \p value is empty, then this removes the given custom data entry.
    SDF_API
    void SetCustomData(const std::string& name, const VtValue& value);

    /// Sets a asset info entry for this prim.
    ///
    /// If \p value is empty, then this removes the given asset info entry.
    /// 
    /// \sa GetAssetInfo()
    ///
    SDF_API
    void SetAssetInfo(const std::string& name, const VtValue& value);

    /// Returns the spec specifier (def, over or class).
    SDF_API
    SdfSpecifier GetSpecifier() const;

    /// Sets the spec specifier (def or over).
    SDF_API
    void SetSpecifier(SdfSpecifier value);

    /// Returns the prim's permission restriction.
    ///
    /// The default value for permission is SdfPermissionPublic.
    SDF_API
    SdfPermission GetPermission() const;

    /// Sets the prim's permission restriction.
    SDF_API
    void SetPermission(SdfPermission value);

    /// Returns the prefixSubstitutions dictionary for this prim spec.
    ///
    /// The default value for prefixSubstitutions is an empty VtDictionary.
    SDF_API
    VtDictionary GetPrefixSubstitutions() const;

    /// Sets the \p prefixSubstitutions dictionary for this prim spec.
    SDF_API
    void SetPrefixSubstitutions(const VtDictionary& prefixSubstitutions);

    /// Returns the suffixSubstitutions dictionary for this prim spec.
    ///
    /// The default value for suffixSubstitutions is an empty VtDictionary.
    SDF_API
    VtDictionary GetSuffixSubstitutions() const;

    /// Sets the \p suffixSubstitutions dictionary for this prim spec.
    SDF_API
    void SetSuffixSubstitutions(const VtDictionary& suffixSubstitutions);

    /// Sets the value for the prim's instanceable flag.
    SDF_API
    void SetInstanceable(bool instanceable);

    /// Returns the value for the prim's instanceable flag.
    SDF_API
    bool GetInstanceable() const;

    /// Returns true if this prim spec has a value authored for its
    /// instanceable flag, false otherwise.
    SDF_API
    bool HasInstanceable() const;

    /// Clears the value for the prim's instanceable flag.
    SDF_API
    void ClearInstanceable();

    /// @}
    /// \name Payload
    /// @{

    /// Returns this prim spec's payload.
    ///
    /// The default value for payload is an empty \c SdfPayload.
    SDF_API
    SdfPayload GetPayload() const;

    /// Sets this prim spec's payload.
    SDF_API
    void SetPayload(const SdfPayload& value);

    /// Returns true if this prim spec has an opinion about payload.
    SDF_API
    bool HasPayload() const;

    /// Remove the payload opinion from this prim spec if there is one.
    SDF_API
    void ClearPayload();

    /// @}
    /// \name Inherits
    /// @{

    /// Returns a proxy for the prim's inherit paths.
    ///
    /// Inherit paths for this prim may be modified through the proxy.
    SDF_API
    SdfInheritsProxy GetInheritPathList() const;

    /// Returns true if this prim has inherit paths set.
    SDF_API
    bool HasInheritPaths() const;

    /// Clears the inherit paths for this prim.
    SDF_API
    void ClearInheritPathList();

    /// @}
    /// \name Specializes
    /// @{

    /// Returns a proxy for the prim's specializes paths.
    ///
    /// Specializes for this prim may be modified through the proxy.
    SDF_API
    SdfSpecializesProxy GetSpecializesList() const;

    /// Returns true if this prim has specializes set.
    SDF_API
    bool HasSpecializes() const;

    /// Clears the specializes for this prim.
    SDF_API
    void ClearSpecializesList();

    /// @}
    /// \name References
    /// @{

    /// Returns a proxy for the prim's references.
    ///
    /// References for this prim may be modified through the proxy.
    SDF_API
    SdfReferencesProxy GetReferenceList() const;

    /// Returns true if this prim has references set.
    SDF_API
    bool HasReferences() const;

    /// Clears the references for this prim.
    SDF_API
    void ClearReferenceList();

    /// @}
    /// \name Variants
    /// @{

    /// Returns a proxy for the prim's variant sets.
    ///
    /// Variant sets for this prim may be modified through the proxy.
    SDF_API
    SdfVariantSetNamesProxy GetVariantSetNameList() const;

    /// Returns true if this prim has variant sets set.
    SDF_API
    bool HasVariantSetNames() const;

    /// Returns list of variant names for the given variant set.
    SDF_API
    std::vector<std::string> GetVariantNames(const std::string& name) const;

    /// Returns the variant sets.
    ///
    /// The result maps variant set names to variant sets.  Variant sets
    /// may be removed through the proxy.
    SDF_API
    SdfVariantSetsProxy GetVariantSets() const;

    /// Removes the variant set with the given \a name.
    ///
    /// Note that the set's name should probably also be removed from
    /// the variant set names list.
    SDF_API
    void RemoveVariantSet(const std::string& name);

    /// Returns an editable map whose keys are variant set names and
    /// whose values are the variants selected for each set.
    SDF_API
    SdfVariantSelectionProxy GetVariantSelections() const;

    /// Sets the variant selected for the given variant set.
    /// If \p variantName is empty, then this removes the variant
    /// selected for the variant set \p variantSetName.
    SDF_API
    void SetVariantSelection(const std::string& variantSetName,
                             const std::string& variantName);

    /// @}
    /// \name Relocates
    /// @{

    /// Get an editing proxy for the map of namespace relocations
    /// specified on this prim.
    ///
    /// The map of namespace relocation paths is editable in-place via
    /// this editing proxy.  Individual source-target pairs can be added,
    /// removed, or altered using common map operations.
    ///
    /// The map is organized as target \c SdfPath indexed by source \c SdfPath.
    /// Key and value paths are stored as absolute regardless of how they're
    /// added.
    SDF_API
    SdfRelocatesMapProxy GetRelocates() const;
    
    /// Set the entire map of namespace relocations specified on this prim.
    /// Use the editing proxy for modifying single paths in the map.
    SDF_API
    void SetRelocates(const SdfRelocatesMap& newMap);

    /// Returns true if this prim has any relocates opinion, including
    /// that there should be no relocates (i.e. an empty map).  An empty
    /// map (no relocates) does not mean the same thing as a missing map
    /// (no opinion).
    SDF_API
    bool HasRelocates() const;
    
    /// Clears the relocates opinion for this prim.
    SDF_API
    void ClearRelocates();

    /// @}

private:
    // Returns true if this object is the pseudo-root.
    bool _IsPseudoRoot() const;

    // Raises an error and returns false if this is the pseudo-root,
    // otherwise returns true.  We want to allow clients to be able
    // to use pseudo-roots as any other prim in namespace editing
    // operations as well as silently permit read accesses on fields
    // pseudo-roots don't actually have in order to promote generic
    // algorithm programming.  Mutating methods on SdfPrimSpec use
    // this function as write access validation.
    bool _ValidateEdit(const TfToken& key) const;

    // Returns a list editor object for name children order list edits.
    boost::shared_ptr<Sdf_ListEditor<SdfNameTokenKeyPolicy> >
    _GetNameChildrenOrderEditor() const;

    // Returns a list editor object for property order list edits.
    boost::shared_ptr<Sdf_ListEditor<SdfNameTokenKeyPolicy> >
    _GetPropertyOrderEditor() const;

private:
    static SdfPrimSpecHandle
    _New(const SdfPrimSpecHandle &parentPrim,
         const TfToken &name, SdfSpecifier spec,
         const TfToken &typeName);
};

/// Convenience function to create a prim at the given path, and any
/// necessary parent prims, in the given layer.
///
/// If a prim already exists at the given path it will be returned
/// unmodified.
///
/// The new specs are created with SdfSpecifierOver and an empty type.
/// primPath must be a valid prim path.
SDF_API 
SdfPrimSpecHandle SdfCreatePrimInLayer(const SdfLayerHandle& layer,
                                       const SdfPath& primPath);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_PRIMSPEC_H
