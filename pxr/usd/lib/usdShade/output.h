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

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdShade/utils.h"

#include <vector>

class UsdShadeConnectableAPI;
class UsdShadeParameter;
class UsdShadeInterfaceAttribute;

/// \class UsdShadeOutput
/// 
/// This class encapsulates a shader or subgraph output, which is a 
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
    TfToken GetBaseName() const;

    /// Get the "scene description" value type name of the attribute associated 
    /// with the output.
    /// 
    /// \note If this is an output belonging to a terminal on a material, which 
    /// does not have an associated attribute, we return 'Token' as the type.
    /// 
    SdfValueTypeName GetTypeName() const;
    
    /// Set a value for the output.
    /// 
    /// It's unusual to be setting a value on an output since it represents 
    /// an externally computed value. The Set API is provided here just for the 
    /// sake of completeness and uniformity with other property schema.
    /// 
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
    bool SetRenderType(TfToken const& renderType) const;

    /// Return this output's specialized renderType, or an empty
    /// token if none was authored.
    ///
    /// \sa SetRenderType()
    TfToken GetRenderType() const;

    /// Return true if a renderType has been specified for this
    /// output.
    ///
    /// \sa SetRenderType()
    bool HasRenderType() const;

    /// @}

    /// \name Connections
    /// Outputs on subgraphs are connectable, but outputs on shaders are not. 
    /// 
    /// @{

    
    /// Connect this output to a named output on a given \p source.
    ///
    /// This action simply records an introspectable relationship:
    /// it implies no actual dataflow in USD, and makes no statement
    /// about what client behavior should be when an Output
    /// is determined to possess both a value and a connection
    /// to a value source - client renderers are required to impose their
    /// own, self-consistent rules.
    ///
    /// The only constraint imposed by the shading model is that Output
    /// connections can be only single-targetted; that is, any given scalar
    /// output can target at most a single source/outputName pair.
    ///
    /// \param source  the shader or subgraph object producing the value
    ///        
    /// \param sourceName the particular computation or parameter we 
    ///        want to consume. This does not include the namespace prefix 
    ///        associated with the source type.
    ///
    /// \param sourceType outputs and parameters are namespaced 
    ///        differently on shading prims, therefore we need to know
    ///        to which we are connecting.  By default we assume we are
    ///        connecting to a computational output, but you can specify
    ///        instead a parameter or another output (assuming your 
    ///        renderer supports it). 
    /// 
    /// In general, we don't connect outputs to interface attributes.
    ///        
    /// \sa GetConnectedSource(), GetConnectedSources()
    bool ConnectToSource(
            UsdShadeConnectableAPI const &source, 
            TfToken const &sourceName,
            UsdShadeAttributeType const sourceType=
                UsdShadeAttributeType::Output) const;

    /// \overload
    /// Connect Output to the source specified by \p sourcePath. 
    /// 
    /// \p sourcePath should be the properly namespaced property path. 
    /// 
    /// This overload is provided for convenience, for use in contexts where 
    /// the prim types are unknown or unavailable.
    /// 
    bool ConnectToSource(const SdfPath &sourcePath) const;

    /// \overload
    ///
    /// Connects this output to the given output.
    /// 
    bool ConnectToSource(UsdShadeOutput const &output) const;

    /// \overload
    ///
    /// Connects this output to the given parameter.
    /// 
    /// XXX: Not sure if this should be allowed.
    /// 
    bool ConnectToSource(UsdShadeParameter const &param) const;

    /// Disconnect source for this Output.
    ///
    /// This may author more scene description than you might expect - we define
    /// the behavior of disconnect to be that, even if an Output becomes
    /// connected in a weaker layer than the current UsdEditTarget, the
    /// Output will \em still be disconnected in the composition, therefore
    /// we must "block" it (see for e.g. UsdRelationship::BlockTargets()) in
    /// the current UsdEditTarget. 
    ///
    /// \sa ConnectToSource().
    bool DisconnectSource() const;

    /// Clears source for this Output in the current UsdEditTarget.
    ///
    /// Most of the time, what you probably want is DisconnectSource()
    /// rather than this function.
    ///
    /// \sa DisconnectSource(), UsdRelationship::ClearTargets()
    bool ClearSource() const;

    /// If this Output is connected, retrieve the \p source prim
    /// and \p sourceName to which it is connected.
    ///
    /// We name the object that an Output is connected to a "source," as 
    /// the "source" produces or contains a value for the Output.
    /// 
    /// \return 
    ///
    /// \c true if \p source is a defined prim on the stage, and 
    /// \p source has an attribute that is either a parameter or an output.
    /// It doesn't make sense for an output to be connected to an interface 
    /// attribute.
    /// 
    /// \c false if not connected to a defined prim.
    ///
    /// \note The python wrapping for this method returns a 
    /// (source, sourceName, sourceType) tuple if the Output is connected, else
    /// \c None
    /// 
    bool GetConnectedSource(
            UsdShadeConnectableAPI *source, 
            TfToken *sourceName,
            UsdShadeAttributeType *sourceType) const;

    /// Returns true if and only if the Output is currently connected to the
    /// output of another \em defined shader object.
    ///
    /// If you will be calling GetConnectedSource() afterwards anyways, 
    /// it will be \em much faster to instead guard like so:
    /// \code
    /// if (output.GetConnectedSource(&source, &outputName)){
    ///      // process connected output
    /// } else {
    ///      // process unconnected output
    /// }
    /// \endcode
    bool IsConnected() const;

    /// Return the name of the sibling relationship that would encode
    /// the connection for this output.
    TfToken GetConnectionRelName() const;

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
    explicit UsdShadeOutput(const UsdAttribute &attr);

    /// Test whether a given UsdAttribute represents a valid Output, which
    /// implies that creating a UsdShadeOutput from the attribute will succeed.
    ///
    /// Success implies that \c prop.IsDefined() is true.
    static bool IsOutput(const UsdAttribute &attr);

    /// Explicit UsdAttribute extractor.
    UsdAttribute GetAttr() const { return _prop.As<UsdAttribute>(); }
    
    /// Explicit UsdProperty extractor.
    const UsdProperty &GetProperty() const { return _prop; }

    /// Allow UsdShadeOutput to auto-convert to UsdAttribute, so you can
    /// pass a UsdShadeOutput to any function that accepts a UsdAttribute or
    /// const-ref thereto.
    operator UsdAttribute () const { return GetAttr(); }

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
            return attr and IsOutput(attr);
        }
        return false;
    }

    /// @}

    /// \anchor UsdShadeOutput_bool_type
    /// Return true if this output is valid for querying and authoring
    /// values and metadata, which is identically equivalent to IsDefined().
#ifdef doxygen
    operator unspecified-bool-type() const();
#else
    operator _UnspecifiedBoolType() const {
        return IsDefined() ? &UsdShadeOutput::_prop : 0;
    }
#endif // doxygen

private:
    friend class UsdShadeConnectableAPI;

    // Constructor that creates a UsdShadeOutput with the given name on the 
    // given prim.
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
    explicit UsdShadeOutput(const UsdRelationship &rel);
    
    // This is currently a relationship if the output belongs to a subgraph.
    // In the future, all outputs will have associated attributes and we 
    // can switch this to be a UsdAttribute instead of UsdProperty.
    UsdProperty _prop;
};

#endif // USDSHADE_OUTPUT_H
