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
#ifndef USDSHADE_OUTPUT_H
#define USDSHADE_OUTPUT_H

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdShade/utils.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdShadeConnectableAPI;
class UsdShadeInput;

/// \class UsdShadeOutput
/// 
/// This class encapsulates a shader or node-graph output, which is a 
/// connectable property representing a typed, externally computed value.
/// 
class UsdShadeOutput
{
public:
    /// Default constructor returns an invalid Output.  Exists for 
    /// container classes
    UsdShadeOutput()
    {
        // nothing
    }

    /// Get the name of the attribute associated with the output. 
    /// 
    /// \note Returns the relationship name if it represents a terminal on a 
    /// material.
    /// 
    TfToken const &GetFullName() const { 
        return _prop.GetName(); 
    }

    /// Returns the name of the output. 
    /// 
    /// We call this the base name since it strips off the "outputs:" namespace 
    /// prefix from the attribute name, and returns it.
    /// 
    /// \note This simply returns the full property name if the Output represents a 
    /// terminal on a material.
    /// 
    USDSHADE_API
    TfToken GetBaseName() const;

    /// Get the prim that the output belongs to.
    UsdPrim GetPrim() const {
        return _prop.GetPrim();
    }

    /// Get the "scene description" value type name of the attribute associated 
    /// with the output.
    /// 
    /// \note If this is an output belonging to a terminal on a material, which 
    /// does not have an associated attribute, we return 'Token' as the type.
    /// 
    USDSHADE_API
    SdfValueTypeName GetTypeName() const;
    
    /// Set a value for the output.
    /// 
    /// It's unusual to be setting a value on an output since it represents 
    /// an externally computed value. The Set API is provided here just for the 
    /// sake of completeness and uniformity with other property schema.
    /// 
    USDSHADE_API
    bool Set(const VtValue& value, 
             UsdTimeCode time = UsdTimeCode::Default()) const;

    /// \overload 
    /// Set the attribute value of the Output at \p time 
    /// 
    template <typename T>
    bool Set(const T& value, UsdTimeCode time = UsdTimeCode::Default()) const {
        if (UsdAttribute attr = GetAttr()) {
            return attr.Set(value, time);
        }
        return false;
    }

    /// \name Configuring the Output's Type
    /// @{

    /// Specify an alternative, renderer-specific type to use when
    /// emitting/translating this output, rather than translating based
    /// on its GetTypeName()
    ///
    /// For example, we set the renderType to "struct" for outputs that
    /// are of renderman custom struct types.
    ///
    /// \return true on success
    USDSHADE_API
    bool SetRenderType(TfToken const& renderType) const;

    /// Return this output's specialized renderType, or an empty
    /// token if none was authored.
    ///
    /// \sa SetRenderType()
    USDSHADE_API
    TfToken GetRenderType() const;

    /// Return true if a renderType has been specified for this
    /// output.
    ///
    /// \sa SetRenderType()
    USDSHADE_API
    bool HasRenderType() const;

    /// @}

    // ---------------------------------------------------------------
    /// \name UsdAttribute API
    // ---------------------------------------------------------------

    /// @{

    typedef const UsdProperty UsdShadeOutput::*_UnspecifiedBoolType;

    /// Speculative constructor that will produce a valid UsdShadeOutput when
    /// \p attr already represents a shade Output, and produces an \em invalid 
    /// UsdShadeOutput otherwise (i.e. \ref UsdShadeOutput_bool_type 
    /// "unspecified-bool-type()" will return false).
    USDSHADE_API
    explicit UsdShadeOutput(const UsdAttribute &attr);

    /// Test whether a given UsdAttribute represents a valid Output, which
    /// implies that creating a UsdShadeOutput from the attribute will succeed.
    ///
    /// Success implies that \c prop.IsDefined() is true.
    USDSHADE_API
    static bool IsOutput(const UsdAttribute &attr);

    /// Explicit UsdAttribute extractor.
    UsdAttribute GetAttr() const { return _prop.As<UsdAttribute>(); }
    
    /// Explicit UsdProperty extractor.
    const UsdProperty &GetProperty() const { return _prop; }

    /// Allow UsdShadeOutput to auto-convert to UsdAttribute, so you can
    /// pass a UsdShadeOutput to any function that accepts a UsdAttribute or
    /// const-ref thereto.
    operator UsdAttribute () const { return GetAttr(); }

    /// Allow UsdShadeOutput to auto-convert to UsdProperty, so you can
    /// pass a UsdShadeOutput to any function that accepts a UsdProperty or
    /// const-ref thereto.
    operator const UsdProperty & () const { return GetProperty(); }

    /// Explicit UsdRelationship extractor.
    UsdRelationship GetRel() const { return _prop.As<UsdRelationship>(); }
    
    /// Returns whether the Output represents a terminal relationship on a 
    /// material, which is a concept we'd like to retire in favor of outputs.
    /// This is termporary convenience API.
    bool IsTerminal() const { return GetRel(); }

    /// Return true if the wrapped UsdAttribute is defined, and in
    /// addition the attribute is identified as an output.
    bool IsDefined() const {
        if (UsdAttribute attr = GetAttr()) {
            return attr && IsOutput(attr);
        }
        return false;
    }

    /// @}

    // -------------------------------------------------------------------------
    /// \name Connections API
    // -------------------------------------------------------------------------
    /// @{

    /// Determines whether this Output can be connected to the given 
    /// source attribute, which can be an input or an output.
    /// 
    /// An output is considered to be connectable only if it belongs to a 
    /// node-graph. Shader outputs are not connectable.
    /// 
    /// \sa UsdShadeConnectableAPI::CanConnect
    USDSHADE_API
    bool CanConnect(const UsdAttribute &source) const;

    /// \overload
    USDSHADE_API
    bool CanConnect(const UsdShadeInput &sourceInput) const;

    /// \overload
    USDSHADE_API
    bool CanConnect(const UsdShadeOutput &sourceOutput) const;

    /// Authors a connection for this Output to the source described by the 
    /// following three elements: 
    /// \p source, the connectable owning the source,
    /// \p sourceName, the name of the source and 
    /// \p sourceType, the value type of the source shading attribute.
    ///
    /// \p typeName if specified, is the typename of the attribute to create 
    /// on the source if it doesn't exist. It is also used to validate whether 
    /// the types of the source and consumer of the connection are compatible.
    ///
    /// \sa UsdShadeConnectableAPI::ConnectToSource
    ///
    USDSHADE_API
    bool ConnectToSource(
        UsdShadeConnectableAPI const &source, 
        TfToken const &sourceName, 
        UsdShadeAttributeType const sourceType=UsdShadeAttributeType::Output,
        SdfValueTypeName typeName=SdfValueTypeName()) const;

    /// Authors a connection for this Output to the source at the given path.
    /// 
    /// \sa UsdShadeConnectableAPI::ConnectToSource
    ///
    USDSHADE_API
    bool ConnectToSource(SdfPath const &sourcePath) const;

    /// Connects this Output to the given input, \p sourceInput.
    /// 
    /// \sa UsdShadeConnectableAPI::ConnectToSource
    ///
    USDSHADE_API
    bool ConnectToSource(UsdShadeInput const &sourceInput) const;

    /// Connects this Output to the given output, \p sourceOutput.
    /// 
    /// \sa UsdShadeConnectableAPI::ConnectToSource
    ///
    USDSHADE_API
    bool ConnectToSource(UsdShadeOutput const &sourceOutput) const;

    /// Finds the source of a connection for this Output.
    /// 
    /// \p source is an output parameter which will be set to the source 
    /// connectable prim.
    /// \p sourceName will be set to the name of the source shading property, 
    /// which could be the parameter name, output name or the interface 
    /// attribute name. This does not include the namespace prefix associated 
    /// with the source type. 
    /// \p sourceType will have the value type of the source shading property.
    ///
    /// \return 
    /// \c true if this Input is connected to a valid, defined source.
    /// \c false if this Input is not connected to a single, valid source.
    /// 
    /// \note The python wrapping for this method returns a 
    /// (source, sourceName, sourceType) tuple if the parameter is connected, 
    /// else \c None
    ///
    /// \sa UsdShadeConnectableAPI::GetConnectedSource
    ///
    USDSHADE_API
    bool GetConnectedSource(UsdShadeConnectableAPI *source, 
                            TfToken *sourceName,
                            UsdShadeAttributeType *sourceType) const;

    /// Returns the "raw" (authored) connected source paths for this Output.
    /// 
    /// \sa UsdShadeConnectableAPI::GetRawConnectedSourcePaths
    ///
    USDSHADE_API
    bool GetRawConnectedSourcePaths(SdfPathVector *sourcePaths) const;

    /// Returns true if and only if this Output is currently connected to a 
    /// valid (defined) source. 
    ///
    /// \sa UsdShadeConnectableAPI::HasConnectedSource
    /// 
    USDSHADE_API
    bool HasConnectedSource() const;

    /// Returns true if the connection to this Output's source, as returned by 
    /// GetConnectedSource(), is authored across a specializes arc, which is 
    /// used to denote a base material.
    /// 
    /// \sa UsdShadeConnectableAPI::IsSourceConnectionFromBaseMaterial
    ///
    USDSHADE_API
    bool IsSourceConnectionFromBaseMaterial() const;

    /// Disconnect source for this Output.
    /// 
    /// \sa UsdShadeConnectableAPI::DisconnectSource
    ///
    USDSHADE_API
    bool DisconnectSource() const;

    /// Clears source for this shading property in the current UsdEditTarget.
    ///
    /// Most of the time, what you probably want is DisconnectSource()
    /// rather than this function.
    ///
    /// \sa UsdShadeConnectableAPI::ClearSource
    ///
    USDSHADE_API
    bool ClearSource() const;

    /// @}

    /// Return true if this Output is valid for querying and authoring
    /// values and metadata, which is identically equivalent to IsDefined().
    explicit operator bool() { 
        return IsDefined(); 
    }

    /// Equality comparison. Returns true if \a lhs and \a rhs represent the 
    /// same UsdShadeOutput, false otherwise.
    friend bool operator==(const UsdShadeOutput &lhs, const UsdShadeOutput &rhs) {
        return lhs.GetProperty() == rhs.GetProperty();
    }

private:
    friend class UsdShadeConnectableAPI;

    // Befriend UsdRiMaterialAPI which will provide a backwards compatible 
    // interface for managing terminal relationships, which turn into outputs
    // in the new encoding of shading networks.
    // This is temporary to assist in the transition to the new shading 
    // encoding.
    friend class UsdRiMaterialAPI;

    // Constructor that creates a UsdShadeOutput with the given name on the 
    // given prim.
    // \p name here is the unnamespaced name of the output.
    UsdShadeOutput(UsdPrim prim,
                   TfToken const &name,
                   SdfValueTypeName const &typeName);

    // Speculative constructor that will produce a valid UsdShadeOutput when 
    // \p rel represents a terminal relationship on a material, a concept that 
    // has been retired in favor of outputs represented as (attribute, 
    // relationship) pair.
    // 
    // Outputs wrapping a terminal relationship are always considered valid 
    // as long as the relationship is defined and valid.
    // 
    // This exists only to allow higher level API to be backwards compatible
    // and treat terminals and outputs uniformly.
    // 
    USDSHADE_API
    explicit UsdShadeOutput(const UsdRelationship &rel);

    // Constructor that wraps the given shading property in a UsdShadeOutput
    // object.
    explicit UsdShadeOutput(const UsdProperty &prop);

    // This is currently a relationship if the output belongs to a node-graph.
    // In the future, all outputs will have associated attributes and we 
    // can switch this to be a UsdAttribute instead of UsdProperty.
    UsdProperty _prop;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSHADE_OUTPUT_H
