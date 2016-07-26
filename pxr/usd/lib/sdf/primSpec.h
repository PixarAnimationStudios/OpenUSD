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
/// \file sdf/primSpec.h

#ifndef SDF_PRIMSPEC_H
#define SDF_PRIMSPEC_H

#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/declarePtrs.h"

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

template <class TypePolicy> class Sdf_ListEditor;

/// \class SdfPrimSpec
/// \brief Represents a prim description in an SdfLayer object.
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

    /// \brief Create a root prim spec.
    ///
    /// Creates a prim spec with a \p name, \p specifier and \p typeName as a
    /// root prim in the given layer.
    static SdfPrimSpecHandle
    New(const SdfLayerHandle& parentLayer,
        const std::string& name, SdfSpecifier spec,
        const std::string& typeName = std::string());

    /// \brief Create a prim spec.
    ///
    /// Creates a prim spec with a \p name, \p specifier and \p typeName as
    /// a namespace child of the given prim.
    static SdfPrimSpecHandle
    New(const SdfPrimSpecHandle& parentPrim,
        const std::string& name, SdfSpecifier spec,
        const std::string& typeName = std::string());

    /// \name Name
    /// @{

    /// Returns the prim's name.
    const std::string& GetName() const;

    /// Returns the prim's name, as a token.
    TfToken GetNameToken() const;

    /// \brief Returns true if setting the prim spec's name to \p newName will
    /// succeed.
    ///
    /// Returns false if it won't, and sets \p whyNot with a string
    /// describing why not.
    bool CanSetName(const std::string& newName, std::string* whyNot) const;

    /// \brief Sets the prim's name.
    ///
    /// Children prims must be unique by name. It is an error to
    /// set the name to the same name as an existing child of this
    /// prim's parent.
    ///
    /// Setting validate to false, will skip validation of the \p newName
    /// (that is, CanSetName will not be called).
    ///
    /// Returns true if successful, false otherwise.
    bool SetName(const std::string& newName, bool validate = true);

    /// \brief Returns true if the given string is a valid prim name.
    static bool IsValidName(const std::string& name);

    /// @}
    /// \name Namespace hierarchy
    /// @{

    /// \brief Returns the prim's namespace pseudo-root prim.
    SdfPrimSpecHandle GetNameRoot() const;

    /// Returns the prim's namespace parent.
    ///
    /// This does not return the pseudo-root for root prims.  Most
    /// algorithms that scan the namespace hierarchy upwards don't
    /// want to process the pseudo-root the same way as actual prims.
    /// Algorithms that do can always call \c GetRealNameParent().
    SdfPrimSpecHandle GetNameParent() const;

    /// Returns the prim's namespace parent.
    SdfPrimSpecHandle GetRealNameParent() const;

    /// Returns a keyed vector view of the prim's namespace children.
    NameChildrenView GetNameChildren() const;

    /// Updates nameChildren to match the given vector of prims.
    void SetNameChildren(const SdfPrimSpecHandleVector&);

    /// \brief Inserts a child.
    ///
    /// \p index is ignored except for range checking;  -1 is permitted.
    ///
    /// Returns true if successful, false if failed.
    bool InsertNameChild(const SdfPrimSpecHandle& child, int index = -1);

    /// Removes the child.  Returns true if succesful, false if failed.
    bool RemoveNameChild(const SdfPrimSpecHandle& child);

    /// \brief Returns the list of child names for this prim's reorder
    /// nameChildren statement.
    ///
    /// See SetNameChildrenOrder() for more info.
    SdfNameChildrenOrderProxy GetNameChildrenOrder() const;

    /// Returns true if this prim has name children order specified
    bool HasNameChildrenOrder() const;

    /// Given a list of (possibly sparse) child names, authors a reorder
    /// nameChildren statement for this prim.
    ///
    /// The reorder statement can modify the order of name children
    /// during composition.  This order doesn't affect GetNameChildren(),
    /// InsertNameChild(), SetNameChildren(), et al.
    void SetNameChildrenOrder(const std::vector<TfToken>& names);

    /// \brief Adds a new name child \p name in the name children order.
    /// If \p index is -1, the name is inserted at the end.
    void InsertInNameChildrenOrder(const TfToken& name, int index = -1);

    /// Removes a name child name from the name children order.
    void RemoveFromNameChildrenOrder(const TfToken& name);

    /// Removes a name child name from the name children order by index.
    void RemoveFromNameChildrenOrderByIndex(int index);

    /// \brief Reorders the given list of child names according to the reorder
    /// nameChildren statement for this prim.
    ///
    /// This routine employs the standard list editing operation for ordered
    /// items in a ListEditor.
    void ApplyNameChildrenOrder(std::vector<TfToken>* vec) const;

    /// @}
    /// \name Properties
    /// @{

    /// Returns the prim's properties.
    PropertySpecView GetProperties() const;

    /// Updates properties to match the given vector of properties.
    void SetProperties(const SdfPropertySpecHandleVector&);

    /// \brief Inserts a property
    ///
    /// \p index is ignored except for range checking;  -1 is permitted.
    ///
    /// Returns true if succesful, false if failed.
    bool InsertProperty(const SdfPropertySpecHandle& property, int index = -1);

    /// Removes the property.
    void RemoveProperty(const SdfPropertySpecHandle& property);

    /// Returns a view of the attributes of this prim.
    AttributeSpecView GetAttributes() const;

    /// Returns a view of the relationships of this prim.
    RelationshipSpecView GetRelationships() const;

    /// \brief Returns the list of property names for this prim's reorder
    /// properties statement.
    ///
    /// See SetPropertyOrder() for more info.
    SdfPropertyOrderProxy GetPropertyOrder() const;

    /// Returns true if this prim has a property ordering specified.
    bool HasPropertyOrder() const;

    /// Given a list of (possibly sparse) property names, authors a
    /// reorder properties statement for this prim.
    ///
    /// The reorder statement can modify the order of properties during
    /// composition.  This order doesn't affect GetProperties(),
    /// InsertProperty(), SetProperties(), et al.
    void SetPropertyOrder(const std::vector<TfToken>& names);

    /// \brief Add a new property \p name in the property order.
    /// If \p index is -1, the name is inserted at the end.
    void InsertInPropertyOrder(const TfToken& name, int index = -1);

    /// Remove a property name from the property order.
    void RemoveFromPropertyOrder(const TfToken& name);

    /// Remove a property name from the property order by index.
    void RemoveFromPropertyOrderByIndex(int index);

    /// \brief Reorders the given list of property names according to the
    /// reorder properties statement for this prim.
    ///
    /// This routine employs the standard list editing operation for ordered
    /// items in a ListEditor.
    void ApplyPropertyOrder(std::vector<TfToken>* vec) const;

    /// @}
    /// \name Lookup
    /// @{

    /// \brief Returns the object for the given \p path.
    ///
    /// If \p path is relative then it will be interpreted as
    /// relative to this prim.  If it is absolute then it will be
    /// interpreted as absolute in this prim's layer.
    ///
    /// Returns invalid handle if there is no object at \p path.
    SdfSpecHandle GetObjectAtPath(const SdfPath& path) const;

    /// \brief Returns a prim given its \p path.
    ///
    /// Returns invalid handle if there is no prim at \p path.
    /// This is simply a more specifically typed version of GetObjectAtPath.
    SdfPrimSpecHandle GetPrimAtPath(const SdfPath& path) const;

    /// \brief Returns a property given its \p path.
    ///
    /// Returns invalid handle if there is no property at \p path.
    /// This is simply a more specifically typed version of GetObjectAtPath.
    SdfPropertySpecHandle GetPropertyAtPath(const SdfPath& path) const;

    /// \brief Returns an attribute given its \p path.
    ///
    /// Returns invalid handle if there is no attribute at \p path.
    /// This is simply a more specifically typed version of GetObjectAtPath.
    SdfAttributeSpecHandle GetAttributeAtPath(const SdfPath& path) const;

    /// \brief Returns a relationship given its \p path.
    ///
    /// Returns invalid handle if there is no relationship at \p path.
    /// This is simply a more specifically typed version of GetObjectAtPath.
    SdfRelationshipSpecHandle GetRelationshipAtPath(const SdfPath& path) const;

    /// @}
    /// \name Metadata
    /// @{

    /// \brief Returns the typeName of the model prim.
    ///
    /// For prims this specifies the sub-class of MfPrim that
    /// this prim describes.
    ///
    /// The default value for typeName is the empty token.
    TfToken GetTypeName() const;

    /// Sets the typeName of the model prim.
    void SetTypeName(const std::string& value);

    /// \brief Returns the comment string for this prim spec.
    ///
    /// The default value for comment is @"".
    std::string GetComment() const;

    /// Sets the comment string for this prim spec.
    void SetComment(const std::string& value);

    /// \brief Returns the documentation string for this prim spec.
    ///
    /// The default value for documentation is @"".
    std::string GetDocumentation() const;

    /// Sets the documentation string for this prim spec.
    void SetDocumentation(const std::string& value);

    /// \brief Returns whether this prim spec is active.
    ///
    /// The default value for active is true.
    bool GetActive() const;

    /// Sets whether this prim spec is active.
    void SetActive(bool value);

    /// Returns true if this prim spec has an opinion about active.
    bool HasActive() const;

    /// Removes the active opinion in this prim spec if there is one.
    void ClearActive();

    /// \brief Returns whether this prim spec will be hidden
    /// in browsers.
    ///
    /// The default value for hidden is false.
    bool GetHidden() const;

    /// Sets whether this prim spec will be hidden in browsers.
    void SetHidden( bool value );

    /// \brief Returns this prim spec's kind.
    ///
    /// The default value for kind is an empty \c TfToken.
    TfToken GetKind() const;

    /// Sets this prim spec's kind.
    void SetKind(const TfToken& value);

    /// Returns true if this prim spec has an opinion about kind.
    bool HasKind() const;

    /// Remove the kind opinion from this prim spec if there is one.
    void ClearKind();

    /// \brief Returns the symmetry function for this prim.
    ///
    /// The default value for symmetry function is an empty token.
    TfToken GetSymmetryFunction() const;

    /// \brief Sets the symmetry function for this prim.
    ///
    /// If \p functionName is an empty token, then this removes any symmetry
    /// function for the given prim.
    void SetSymmetryFunction(const TfToken& functionName);

    /// \brief Returns the symmetry arguments for this prim.
    ///
    /// The default value for symmetry arguments is an empty dictionary.
    SdfDictionaryProxy GetSymmetryArguments() const;

    /// \brief Sets a symmetry argument for this prim.
    ///
    /// If \p value is empty, then this removes the setting
    /// for the given symmetry argument \p name.
    void SetSymmetryArgument(const std::string& name, const VtValue& value);

    /// \brief Returns the symmetric peer for this prim.
    ///
    /// The default value for symmetric peer is an empty string.
    std::string GetSymmetricPeer() const;

    /// \brief Sets a symmetric peer for this prim.
    ///
    /// If \p peerName is empty, then this removes the symmetric peer
    /// for this prim.
    void SetSymmetricPeer(const std::string& peerName);

    /// \brief Returns the prefix string for this prim spec.
    ///
    /// The default value for prefix is "".
    std::string GetPrefix() const;

    /// Sets the prefix string for this prim spec.
    void SetPrefix(const std::string& value);

    /// \brief Returns the custom data for this prim.
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
    SdfDictionaryProxy GetCustomData() const;

    /// \brief Returns the asset info dictionary for this prim.
    /// 
    /// The default value is an empty dictionary. 
    /// 
    /// The asset info dictionary is used to annotate prims representing the 
    /// root-prims of assets (generally organized as models) with various 
    /// data related to asset management. For example, asset name, root layer
    /// identifier, asset version etc.
    /// 
    SdfDictionaryProxy GetAssetInfo() const;

    /// \brief Sets a custom data entry for this prim.
    ///
    /// If \p value is empty, then this removes the given custom data entry.
    void SetCustomData(const std::string& name, const VtValue& value);

    /// \brief Sets a asset info entry for this prim.
    ///
    /// If \p value is empty, then this removes the given asset info entry.
    /// 
    /// \sa GetAssetInfo()
    ///
    void SetAssetInfo(const std::string& name, const VtValue& value);

    /// \brief Returns the spec specifier (def, over or class).
    SdfSpecifier GetSpecifier() const;

    /// Sets the spec specifier (def or over).
    void SetSpecifier(SdfSpecifier value);

    /// \brief Returns the prim's permission restriction.
    ///
    /// The default value for permission is SdfPermissionPublic.
    SdfPermission GetPermission() const;

    /// Sets the prim's permission restriction.
    void SetPermission(SdfPermission value);

    /// \brief Returns the prefixSubstitutions dictionary for this prim spec.
    ///
    /// The default value for prefixSubstitutions is an empty VtDictionary.
    VtDictionary GetPrefixSubstitutions() const;

    /// Sets the \p prefixSubstitutions dictionary for this prim spec.
    void SetPrefixSubstitutions(const VtDictionary& prefixSubstitutions);

    /// Sets the value for the prim's instanceable flag.
    void SetInstanceable(bool instanceable);

    /// Returns the value for the prim's instanceable flag.
    bool GetInstanceable() const;

    /// Returns true if this prim spec has a value authored for its
    /// instanceable flag, false otherwise.
    bool HasInstanceable() const;

    /// Clears the value for the prim's instanceable flag.
    void ClearInstanceable();

    /// @}
    /// \name Payload
    /// @{

    /// \brief Returns this prim spec's payload.
    ///
    /// The default value for payload is an empty \c SdfPayload.
    SdfPayload GetPayload() const;

    /// Sets this prim spec's payload.
    void SetPayload(const SdfPayload& value);

    /// Returns true if this prim spec has an opinion about payload.
    bool HasPayload() const;

    /// Remove the payload opinion from this prim spec if there is one.
    void ClearPayload();

    /// @}
    /// \name Inherits
    /// @{

    /// \brief Returns a proxy for the prim's inherit paths.
    ///
    /// Inherit paths for this prim may be modified through the proxy.
    SdfInheritsProxy GetInheritPathList() const;

    /// Returns true if this prim has inherit paths set.
    bool HasInheritPaths() const;

    /// Clears the inherit paths for this prim.
    void ClearInheritPathList();

    /// @}
    /// \name Specializes
    /// @{

    /// \brief Returns a proxy for the prim's specializes paths.
    ///
    /// Specializes for this prim may be modified through the proxy.
    SdfSpecializesProxy GetSpecializesList() const;

    /// Returns true if this prim has specializes set.
    bool HasSpecializes() const;

    /// Clears the specializes for this prim.
    void ClearSpecializesList();

    /// @}
    /// \name References
    /// @{

    /// \brief Returns a proxy for the prim's references.
    ///
    /// References for this prim may be modified through the proxy.
    SdfReferencesProxy GetReferenceList() const;

    /// Returns true if this prim has references set.
    bool HasReferences() const;

    /// Clears the references for this prim.
    void ClearReferenceList();

    /// @}
    /// \name Variants
    /// @{

    /// \brief Returns a proxy for the prim's variant sets.
    ///
    /// Variant sets for this prim may be modified through the proxy.
    SdfVariantSetNamesProxy GetVariantSetNameList() const;

    /// Returns true if this prim has variant sets set.
    bool HasVariantSetNames() const;

    /// \brief Returns list of variant names for the given varient set.
    std::vector<std::string> GetVariantNames(const std::string& name) const;

    /// \brief Returns the variant sets.
    ///
    /// The result maps variant set names to variant sets.  Variant sets
    /// may be removed through the proxy.
    SdfVariantSetsProxy GetVariantSets() const;

    /// \brief Removes the variant set with the given \a name.
    ///
    /// Note that the set's name should probably also be removed from
    /// the variant set names list.
    void RemoveVariantSet(const std::string& name);

    /// Returns an editable map whose keys are variant set names and
    /// whose values are the variants selected for each set.
    SdfVariantSelectionProxy GetVariantSelections() const;

    /// \brief Sets the variant selected for the given variant set.
    /// If \p variantName is empty, then this removes the variant
    /// selected for the variant set \p variantSetName.
    void SetVariantSelection(const std::string& variantSetName,
                             const std::string& variantName);

    /// @}
    /// \name Relocates
    /// @{

    /// \brief Get an editing proxy for the map of namespace relocations
    /// specified on this prim.
    ///
    /// The map of namespace relocation paths is editable in-place via
    /// this editing proxy.  Individual source-target pairs can be added,
    /// removed, or altered using common map operations.
    ///
    /// The map is organized as target \c SdfPath indexed by source \c SdfPath.
    /// Key and value paths are stored as absolute regardless of how they're
    /// added.
    SdfRelocatesMapProxy GetRelocates() const;
    
    /// Set the entire map of namespace relocations specified on this prim.
    /// Use the editing proxy for modifying single paths in the map.
    void SetRelocates(const SdfRelocatesMap& newMap);

    /// Returns true if this prim has any relocates opinion, including
    /// that there should be no relocates (i.e. an empty map).  An empty
    /// map (no relocates) does not mean the same thing as a missing map
    /// (no opinion).
    bool HasRelocates() const;
    
    /// Clears the relocates opinion for this prim.
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
SdfPrimSpecHandle SdfCreatePrimInLayer(const SdfLayerHandle& layer,
                                       const SdfPath& primPath);

#endif // SDF_PRIMSPEC_H
