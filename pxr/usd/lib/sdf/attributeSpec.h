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
#ifndef SDF_ATTRIBUTESPEC_H
#define SDF_ATTRIBUTESPEC_H

/// \file sdf/attributeSpec.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/tf/enum.h"

PXR_NAMESPACE_OPEN_SCOPE

template <class T> class Sdf_MarkerUtils;

/// \class SdfAttributeSpec
///
/// A subclass of SdfPropertySpec that holds typed data.
///
/// Attributes are typed data containers that can optionally hold any
/// and all of the following:
/// \li A single default value.
/// \li An array of knot values describing how the value varies over time.
/// \li A dictionary of posed values, indexed by name.
///
/// The values contained in an attribute must all be of the same type.  In the
/// Python API the \c typeName property holds the attribute type.  In the C++
/// API, you can get the attribute type using the GetTypeName() method.  In
/// addition, all values, including all knot values, must be the same shape.
/// For information on shapes, see the VtShape class reference in the C++
/// documentation.
///
class SdfAttributeSpec : public SdfPropertySpec
{
    SDF_DECLARE_SPEC(SdfSchema, SdfSpecTypeAttribute,
                     SdfAttributeSpec, SdfPropertySpec);

public:
    typedef SdfAttributeSpec This;
    typedef SdfPropertySpec Parent;

    ///
    /// \name Spec construction
    /// @{

    /// Constructs a new prim attribute instance.
    ///
    /// Creates and returns a new attribute for the given prim.
    /// The \p owner will own the newly created attribute.
    static SdfAttributeSpecHandle
    New(const SdfPrimSpecHandle& owner,
        const std::string& name, const SdfValueTypeName& typeName,
        SdfVariability variability = SdfVariabilityVarying,
        bool custom = false);

    /// Constructs a new relational attribute instance.
    ///
    /// Creates and returns a new attribute for the given relationship
    /// and target. The \p owner will own the newly created attribute.
    /// The new attribute will appear at the end of the target's
    /// attribute list.
    static SdfAttributeSpecHandle
    New(const SdfRelationshipSpecHandle& owner,
        const SdfPath& targetPath,
        const std::string& name, const SdfValueTypeName& typeName,
        SdfVariability variability = SdfVariabilityVarying,
        bool custom = false);

    /// @}

public:
    /// \name Connections
    /// @{

    /// Returns a proxy for editing the attribute's connection paths.
    ///
    /// The returned proxy, which is an SdfListEditorProxy, modifies the
    /// SdfListOp that represents this attribute's connections.
    SdfConnectionsProxy GetConnectionPathList() const;

    /// Returns \c true if any connection paths are set on this attribute.
    bool HasConnectionPaths() const;

    /// Clears the connection paths for this attribute.
    void ClearConnectionPaths();

    /// @}
    /// \name Mappers
    /// @{

    /// Returns the mappers for this attribute.
    ///
    /// Returns an editable map whose keys are connection paths and whose
    /// values are mappers.  Mappers may be removed from the map.  Mappers
    /// are added by directly constructing them.
    SdfConnectionMappersProxy GetConnectionMappers() const;

    /// Returns the target path that mapper \p mapper is associated with.
    SdfPath GetConnectionPathForMapper(const SdfMapperSpecHandle& mapper);

    /// Changes the path a mapper is associated with from \p oldPath to
    /// \p newPath.
    void ChangeMapperPath(const SdfPath& oldPath, const SdfPath& newPath);

    /// @}
    /// \name Markers
    /// @{

    typedef std::map<SdfPath, std::string,
                     SdfPath::FastLessThan> ConnectionMarkerMap;

    /// Sets all the connection markers for this attribute.
    void SetConnectionMarkers(const ConnectionMarkerMap& markers);

    /// Returns the marker for the given connection path.
    /// If no marker exists, returns the empty string.
    std::string GetConnectionMarker(const SdfPath& path) const;

    /// Sets the marker for the given connection path.
    /// Clears the marker if an empty string is given.
    void SetConnectionMarker(const SdfPath& path, const std::string& marker);

    /// Clears the marker for the given connection path.
    void ClearConnectionMarker(const SdfPath& path);

    /// Returns all connection paths on which markers are specified.
    SdfPathVector GetConnectionMarkerPaths() const;

    /// @}
    /// \name Attribute value API
    /// @{

    /// Returns the allowed tokens metadata for this attribute.
    /// Consumers may use this metadata to define a set of predefined
    /// options for this attribute's value. However, this metadata is
    /// purely advisory. It is up to the consumer to perform any
    /// validation against this set of tokens, if desired.
    VtTokenArray GetAllowedTokens() const;

    /// Sets the allowed tokens metadata for this attribute.
    void SetAllowedTokens(const VtTokenArray& allowedTokens);

    /// Returns true if allowed tokens metadata is set for this attribute.
    bool HasAllowedTokens() const;

    /// Clears the allowed tokens metadata for this attribute.
    void ClearAllowedTokens(); 

    /// Returns the display unit of the attribute.
    TfEnum GetDisplayUnit() const;

    /// Sets the display unit of the attribute.
    void SetDisplayUnit(const TfEnum& displayUnit);

    /// Returns true if a display unit is set for this attribute.
    bool HasDisplayUnit() const;

    /// Clears the display unit of the attribute.
    void ClearDisplayUnit();

    /// @}
    /// \name Spec properties
    /// @{

    /// Returns the roleName for this attribute's typeName.
    ///
    /// If the typeName has no roleName, return empty token.
    TfToken GetRoleName() const;

    /// @}

private:
    static SdfAttributeSpecHandle
    _New(const SdfSpecHandle &owner,
         const SdfPath& attributePath,
         const SdfValueTypeName& typeName,
         SdfVariability variability,
         bool custom);

    static SdfAttributeSpecHandle
    _New(const SdfRelationshipSpecHandle& owner,
         const SdfPath& targetPath,
         const std::string& name,
         const SdfValueTypeName& typeName,
         SdfVariability variability,
         bool custom);

    SdfPath _CanonicalizeConnectionPath(const SdfPath& connectionPath) const;

    SdfSpecHandle _FindOrCreateChildSpecForMarker(const SdfPath& key);

    friend class SdfMapperSpec;
    friend class Sdf_MarkerUtils<SdfAttributeSpec>;
    friend class Sdf_PyAttributeAccess;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_ATTRIBUTESPEC_H
