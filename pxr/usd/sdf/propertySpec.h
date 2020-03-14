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
#ifndef PXR_USD_SDF_PROPERTY_SPEC_H
#define PXR_USD_SDF_PROPERTY_SPEC_H

/// \file sdf/propertySpec.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
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

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfPropertySpec
///
/// Base class for SdfAttributeSpec and SdfRelationshipSpec.
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
    SDF_DECLARE_ABSTRACT_SPEC(SdfPropertySpec, SdfSpec);

public:
    ///
    /// \name Name
    /// @{

    /// Returns the property's name.
    SDF_API
    const std::string &GetName() const;

    /// Returns the property's name, as a token.
    SDF_API
    TfToken GetNameToken() const;

    /// Returns true if setting the property spec's name to \p newName
    /// will succeed.
    ///
    /// Returns false if it won't, and sets \p whyNot with a string
    /// describing why not.
    SDF_API
    bool CanSetName(const std::string &newName, std::string *whyNot) const;

    /// Sets the property's name.
    ///
    /// A Prim's properties must be unique by name. Setting the
    /// name to the same name as an existing property is an error.
    ///
    /// Setting \p validate to false, will skip validation of the newName
    /// (that is, CanSetName will not be called).
    SDF_API
    bool SetName(const std::string &newName, bool validate = true);

    /// Returns true if the given name is considered a valid name for a
    /// property.  A valid name is not empty, and does not use invalid
    /// characters (such as '/', '[', or '.').
    SDF_API
    static bool IsValidName(const std::string &name);

    /// @}
    /// \name Ownership
    /// @{

    /// Returns the owner prim or relationship of this property.
    SDF_API
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
    SDF_API
    SdfDictionaryProxy GetCustomData() const;

    /// Returns the asset info dictionary for this property.
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
    SDF_API
    SdfDictionaryProxy GetAssetInfo() const;

    /// Sets a property custom data entry.
    ///
    /// If \p value is empty, then this removes the given custom data entry.
    SDF_API
    void SetCustomData(const std::string &name, const VtValue &value);

    /// Sets a asset info entry for this property.
    ///
    /// If \p value is empty, then this removes the given asset info entry.
    /// 
    /// \sa GetAssetInfo()
    ///
    SDF_API
    void SetAssetInfo(const std::string& name, const VtValue& value);

    /// Returns the displayGroup string for this property spec.
    ///
    /// The default value for displayGroup is empty string.
    SDF_API
    std::string GetDisplayGroup() const;

    /// Sets the displayGroup string for this property spec.
    SDF_API
    void SetDisplayGroup(const std::string &value);

    /// Returns the displayName string for this property spec.
    ///
    /// The default value for displayName is empty string.
    SDF_API
    std::string GetDisplayName() const;

    /// Sets the displayName string for this property spec.
    SDF_API
    void SetDisplayName(const std::string &value);

    /// Returns the documentation string for this property spec.
    ///
    /// The default value for documentation is empty string.
    SDF_API
    std::string GetDocumentation() const;

    /// Sets the documentation string for this property spec.
    SDF_API
    void SetDocumentation(const std::string &value);

    /// Returns whether this property spec will be hidden in browsers.
    ///
    /// The default value for hidden is false.
    SDF_API
    bool GetHidden() const;

    /// Sets whether this property spec will be hidden in browsers.
    SDF_API
    void SetHidden(bool value);

    /// Returns the property's permission restriction.
    ///
    /// The default value for permission is SdfPermissionPublic.
    SDF_API
    SdfPermission GetPermission() const;

    /// Sets the property's permission restriction.
    SDF_API
    void SetPermission(SdfPermission value);

    /// Returns the prefix string for this property spec.
    ///
    /// The default value for prefix is "".
    SDF_API
    std::string GetPrefix() const;

    /// Sets the prefix string for this property spec.
    SDF_API
    void SetPrefix(const std::string &value);

    /// Returns the suffix string for this property spec.
    ///
    /// The default value for suffix is "".
    SDF_API
    std::string GetSuffix() const;

    /// Sets the suffix string for this property spec.
    SDF_API
    void SetSuffix(const std::string &value);

    /// Returns the property's symmetric peer.
    ///
    /// The default value for the symmetric peer is an empty string.
    SDF_API
    std::string GetSymmetricPeer() const;

    /// Sets the property's symmetric peer.
    ///
    /// If \p peerName is empty, then this removes any symmetric peer for the
    /// given property.
    SDF_API
    void SetSymmetricPeer(const std::string &peerName);

    /// Returns the property's symmetry arguments.
    ///
    /// The default value for symmetry arguments is an empty dictionary.
    SDF_API
    SdfDictionaryProxy GetSymmetryArguments() const;

    /// Sets a property symmetry argument.
    ///
    /// If \p value is empty, then this removes the argument with the given
    /// \p name.
    SDF_API
    void SetSymmetryArgument(const std::string &name, const VtValue &value);

    /// Returns the property's symmetry function.
    ///
    /// The default value for the symmetry function is an empty token.
    SDF_API
    TfToken GetSymmetryFunction() const;

    /// Sets the property's symmetry function.
    ///
    /// If \p functionName is empty, then this removes any symmetry function
    /// for the given property.
    SDF_API
    void SetSymmetryFunction(const TfToken &functionName);

    /// @}
    /// \name Property value API
    /// @{

    /// Returns the entire set of time samples.
    SDF_API
    SdfTimeSampleMap GetTimeSampleMap() const;

    /// Returns the TfType representing the value type this property holds.
    SDF_API
    TfType GetValueType() const;

    /// Returns the name of the value type that this property holds.
    ///
    /// Returns the typename used to represent the types of value held by
    /// this attribute.
    SDF_API
    SdfValueTypeName GetTypeName() const;

    /// Returns the attribute's default value.
    ///
    /// If it doesn't have a default value, an empty VtValue is returned.
    SDF_API
    VtValue GetDefaultValue() const;

    /// Sets the attribute's default value.
    ///
    /// Returns true if successful, false otherwise.  Fails if \p defaultValue
    /// has wrong type.
    SDF_API
    bool SetDefaultValue(const VtValue &defaultValue);

    /// Returns true if a default value is set for this attribute.
    SDF_API
    bool HasDefaultValue() const;

    /// Clear the attribute's default value.
    SDF_API
    void ClearDefaultValue();

    /// @}
    /// \name Spec properties
    /// @{

    /// Returns the comment string for this property spec.
    ///
    /// The default value for comment is "".
    SDF_API
    std::string GetComment() const;

    /// Sets the comment string for this property spec.
    SDF_API
    void SetComment(const std::string &value);

    /// Returns true if this spec declares a custom property
    SDF_API
    bool IsCustom() const;

    /// Sets whether this spec declares a custom property
    SDF_API
    void SetCustom(bool custom);

    /// Returns the variability of the property.
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
    SDF_API
    SdfVariability GetVariability() const;

    /// Returns true if this PropertySpec has no significant data other than
    /// just what is necessary for instantiation.
    /// 
    /// For example, "double foo" has only required fields, but "double foo = 3"
    /// has more than just what is required.
    /// 
    /// This is similar to IsInert except that IsInert will always return false 
    /// even for properties that have only required fields; PropertySpecs are 
    /// never considered inert because even a spec with only required fields 
    /// will cause instantiation of on-demand properties.
    ///
    SDF_API
    bool HasOnlyRequiredFields() const;

private:
    inline TfToken _GetAttributeValueTypeName() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // #ifndef PXR_USD_SDF_PROPERTY_SPEC_H
