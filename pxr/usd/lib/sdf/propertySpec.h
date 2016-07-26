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
/// \file sdf/propertySpec.h

#ifndef SDF_PROPERTYSPEC_H
#define SDF_PROPERTYSPEC_H

#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"

#include <iosfwd>
#include <string>

/// \class SdfPropertySpec
/// \brief Base class for SdfAttributeSpec and SdfRelationshipSpec.
///
/// Scene Spec Attributes (SdfAttributeSpec) and Relationships
/// (SdfRelationshipSpec) are the basic properties that make up Scene Spec Prims
/// (SdfPrimSpec).  They share many qualities and can sometimes be treated
/// uniformly.  The common qualities are provided by this base class.
///
/// NOTE: Do not use Python reserved words and keywords as attribute names.
/// This will cause attribute resolution to fail.
///
class SdfPropertySpec : public SdfSpec
{
    SDF_DECLARE_ABSTRACT_SPEC(SdfSchema, SdfPropertySpec, SdfSpec);

public:
    ///
    /// \name Name
    /// @{

    /// \brief Returns the property's name.
    const std::string &GetName() const;

    /// \brief Returns the property's name, as a token.
    TfToken GetNameToken() const;

    /// \brief Returns true if setting the property spec's name to \p newName
    /// will succeed.
    ///
    /// Returns false if it won't, and sets \p whyNot with a string
    /// describing why not.
    bool CanSetName(const std::string &newName, std::string *whyNot) const;

    /// \brief Sets the property's name.
    ///
    /// A Prim's properties must be unique by name. Setting the
    /// name to the same name as an existing property is an error.
    ///
    /// Setting \p validate to false, will skip validation of the newName
    /// (that is, CanSetName will not be called).
    bool SetName(const std::string &newName, bool validate = true);

    /// \brief Returns true if the given name is considered a valid name for a
    /// property.  A valid name is not empty, and does not use invalid
    /// characters (such as '/', '[', or '.').
    static bool IsValidName(const std::string &name);

    /// @}
    /// \name Ownership
    /// @{

    /// \brief Returns the owner prim or relationship of this property.
    SdfSpecHandle GetOwner() const;

    /// @}
    /// \name Metadata
    /// @{

    /// Returns the property's custom data.
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

    /// \brief Returns the asset info dictionary for this property.
    /// 
    /// The default value is an empty dictionary. 
    /// 
    /// The asset info dictionary is used to annotate SdfAssetPath-valued 
    /// attributes pointing to the root-prims of assets (generally organized 
    /// as models) with various data related to asset management. For example, 
    /// asset name, root layer identifier, asset version etc.
    /// 
    /// \note It is only valid to author assetInfo on attributes that are of 
    /// type SdfAssetPath.
    /// 
    SdfDictionaryProxy GetAssetInfo() const;

    /// \brief Sets a property custom data entry.
    ///
    /// If \p value is empty, then this removes the given custom data entry.
    void SetCustomData(const std::string &name, const VtValue &value);

    /// \brief Sets a asset info entry for this property.
    ///
    /// If \p value is empty, then this removes the given asset info entry.
    /// 
    /// \sa GetAssetInfo()
    ///
    void SetAssetInfo(const std::string& name, const VtValue& value);

    /// \brief Returns the displayGroup string for this property spec.
    ///
    /// The default value for displayGroup is empty string.
    std::string GetDisplayGroup() const;

    /// Sets the displayGroup string for this property spec.
    void SetDisplayGroup(const std::string &value);

    /// \brief Returns the displayName string for this property spec.
    ///
    /// The default value for displayName is empty string.
    std::string GetDisplayName() const;

    /// Sets the displayName string for this property spec.
    void SetDisplayName(const std::string &value);

    /// \brief Returns the documentation string for this property spec.
    ///
    /// The default value for documentation is empty string.
    std::string GetDocumentation() const;

    /// Sets the documentation string for this property spec.
    void SetDocumentation(const std::string &value);

    /// \brief Returns whether this property spec will be hidden in browsers.
    ///
    /// The default value for hidden is false.
    bool GetHidden() const;

    /// Sets whether this property spec will be hidden in browsers.
    void SetHidden(bool value);

    /// \brief Returns the property's permission restriction.
    ///
    /// The default value for permission is SdfPermissionPublic.
    SdfPermission GetPermission() const;

    /// Sets the property's permission restriction.
    void SetPermission(SdfPermission value);

    /// \brief Returns the prefix string for this property spec.
    ///
    /// The default value for prefix is "".
    std::string GetPrefix() const;

    /// Sets the prefix string for this property spec.
    void SetPrefix(const std::string &value);

    /// \brief Returns the property's symmetric peer.
    ///
    /// The default value for the symmetric peer is an empty string.
    std::string GetSymmetricPeer() const;

    /// \brief Sets the property's symmetric peer.
    ///
    /// If \p peerName is empty, then this removes any symmetric peer for the
    /// given property.
    void SetSymmetricPeer(const std::string &peerName);

    /// \brief Returns the property's symmetry arguments.
    ///
    /// The default value for symmetry arguments is an empty dictionary.
    SdfDictionaryProxy GetSymmetryArguments() const;

    /// \brief Sets a property symmetry argument.
    ///
    /// If \p value is empty, then this removes the argument with the given
    /// \p name.
    void SetSymmetryArgument(const std::string &name, const VtValue &value);

    /// \brief Returns the property's symmetry function.
    ///
    /// The default value for the symmetry function is an empty token.
    TfToken GetSymmetryFunction() const;

    /// \brief Sets the property's symmetry function.
    ///
    /// If \p functionName is empty, then this removes any symmetry function
    /// for the given property.
    void SetSymmetryFunction(const TfToken &functionName);

    /// @}
    /// \name Property value API
    /// @{

    /// \brief Returns the entire set of time samples.
    SdfTimeSampleMap GetTimeSampleMap() const;

    /// Returns the TfType representing the value type this property holds.
    TfType GetValueType() const;

    /// \brief Returns the name of the value type that this property holds.
    ///
    /// Returns the typename used to represent the types of value held by
    /// this attribute.
    SdfValueTypeName GetTypeName() const;

    /// \brief Returns the attribute's default value.
    ///
    /// If it doesn't have a default value, an empty VtValue is returned.
    VtValue GetDefaultValue() const;

    /// \brief Sets the attribute's default value.
    ///
    /// Returns true if successful, false otherwise.  Fails if \p defaultValue
    /// has wrong type.
    bool SetDefaultValue(const VtValue &defaultValue);

    /// Returns true if a default value is set for this attribute.
    bool HasDefaultValue() const;

    /// Clear the attribute's default value.
    void ClearDefaultValue();

    /// @}
    /// \name Spec properties
    /// @{

    /// \brief Returns the comment string for this property spec.
    ///
    /// The default value for comment is "".
    std::string GetComment() const;

    /// Sets the comment string for this property spec.
    void SetComment(const std::string &value);

    /// Returns true if this spec declares a custom property
    bool IsCustom() const;

    /// Sets whether this spec declares a custom property
    void SetCustom(bool custom);

    /// \brief Returns the variability of the property.
    ///
    /// An attribute's variability may be \c Varying (the default),
    /// \c Uniform, \c Config, or \c Computed.
    ///
    /// A relationship's variability may be \c Varying or \c Uniform (the
    /// default)
    ///
    /// <ul>
    ///     <li>\c Varying attributes may be directly authored, animated and
    ///         affected by \p Actions.  They are the most flexible.
    ///         Varying relationships can have a default and an anim spline,
    ///         in addition to a list of targets.
    ///
    ///     <li>\c Uniform attributes may be authored only with non-animated
    ///         values (default values).  They cannot be affected by \p Actions,
    ///         but they can be connected to other Uniform attributes.
    ///         Uniform relationships have a list of targets but do not have
    ///         default or anim spline values.
    ///
    ///     <li>\c Config attributes are the same as Uniform except that a Prim
    ///         can choose to alter its collection of built-in properties based
    ///         on the values of its Config attributes.
    ///
    ///     <li>\c Computed attributes may not be authored in scene description.
    ///         Prims determine the values of their Computed attributes through
    ///         Prim-specific computation.  They may not be connected.
    /// </ul>
    SdfVariability GetVariability() const;

    /// \brief Returns true if this PropertySpec has no significant data other 
    ///        than just what is necessary for instantiation.
    /// 
    /// For example, "double foo" has only required fields, but "double foo = 3"
    /// has more than just what is required.
    /// 
    /// This is similar to IsInert except that IsInert will always return false 
    /// even for properties that have only required fields; PropertySpecs are 
    /// never considered inert because even a spec with only required fields 
    /// will cause instantiation of on-demand properties.
    ///
    bool HasOnlyRequiredFields() const;

private:
    TfToken _GetAttributeValueTypeName() const;
};

#endif  // #ifndef SDF_PROPERTYSPEC_H
