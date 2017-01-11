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
#include <vector>

class UsdShadeConnectableAPI;
class UsdShadeParameter;

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
    TfToken const &GetName() const { 
        return _attr.GetName(); 
    }

    /// Get the name of the output. 
    /// This strips off the "outputs:" namespace prefix from the attribute name 
    /// and returns it.
    TfToken GetOutputName() const;

    /// Get the "scene description" value type name of the attribute associated 
    /// with the output.
    /// If this is an output belonging to a subgraph that does not have 
    /// an associated attribute, we return 'Token' as the (fallback) type.
    SdfValueTypeName GetTypeName() const { 
        return  _attr.GetTypeName();
    }
    
    /// Set the value for the output.
    bool Set(const VtValue& value, 
             UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Set the attribute value of the Primvar at \p time 
    template <typename T>
    bool Set(const T& value, UsdTimeCode time = UsdTimeCode::Default()) const {
        return _attr.Set(value, time);
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
    /// \param outputName  the particular computation or parameter we 
    ///        want to consume
    ///
    /// \param outputIsParameter outputs and parameters are namespaced 
    ///        differently on a shader prim, therefore we need to know
    ///        to which we are connecting.  By default we assume we are
    ///        connecting to a computational output, but you can specify
    ///        instead a parameter (assuming your renderer supports it)
    ///        with a value of \c true.
    /// \sa GetConnectedSource(), GetConnectedSources()
    bool ConnectToSource(
            UsdShadeConnectableAPI const &source, 
            TfToken const &outputName,
            bool outputIsParameter=false) const;

    /// \overload
    ///
    /// Connects this output to the given output.
    /// 
    bool ConnectToSource(UsdShadeOutput const &output) const;

    /// \overload
    ///
    /// Connects this output to the given parameter.
    /// 
    /// XXX: Should this be allowed?
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
    /// and \p outputName to which it is connected.
    ///
    /// We name the object that an Output is connected to a "source," as 
    /// the "source" produces or contains a value for the Output.
    /// \return \c true if \p source is a defined prim on the stage, and 
    /// \p source has an attribute that is either an input or output;
    /// \c false if not connected to a defined prim.
    ///
    /// \note The python wrapping for this method returns a 
    /// (source, ouputName) tuple if the Output is connected, else
    /// \c None
    bool GetConnectedSource(
            UsdShadeConnectableAPI *source, 
            TfToken *outputName) const;

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

    // ---------------------------------------------------------------
    /// \name UsdAttribute API
    // ---------------------------------------------------------------

    /// @{

    typedef const UsdAttribute UsdShadeOutput::*_UnspecifiedBoolType;

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
    
    /// Allow UsdShadeOutput to auto-convert to UsdAttribute, so you can
    /// pass a UsdShadeOutput to any function that accepts a UsdAttribute or
    /// const-ref thereto.
    operator UsdAttribute const& () const { return _attr; }

    /// Explicit UsdAttribute extractor
    UsdAttribute const &GetAttr() const { return _attr; }

    /// Return true if the wrapped UsdAttribute is defined, and in
    /// addition the attribute is identified as an output.
    bool IsDefined() const {
        return _attr and IsOutput(_attr);
    }

    /// @}

    /// \anchor UsdShadeOutput_bool_type
    /// Return true if this output is valid for querying and authoring
    /// values and metadata, which is identically equivalent to IsDefined().
#ifdef doxygen
    operator unspecified-bool-type() const();
#else
    operator _UnspecifiedBoolType() const {
        return IsDefined() ? &UsdShadeOutput::_attr : 0;
    }
#endif // doxygen

private:
    friend class UsdShadeConnectableAPI;

    // Constructor that creates a UsdShadeOutput with the given name on the 
    // given prim.
    UsdShadeOutput(UsdPrim prim,
                   TfToken const &name,
                   SdfValueTypeName const &typeName);
    
    // This is currently a relationship if the output belongs to a subgraph.
    // In the future, all outputs will have associated props.
    UsdAttribute _attr;
};

#endif // USDSHADE_OUTPUT_H
