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
#ifndef USDSHADE_PARAMETER_H
#define USDSHADE_PARAMETER_H

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdShade/utils.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class UsdShadeConnectableAPI;
class UsdShadeOutput;
class UsdShadeInput;
class UsdShadeInterfaceAttribute;

/// \class UsdShadeParameter
///
/// Schema wrapper for UsdAttribute for authoring and introspecting shader
/// parameters (which are attributes within a shading network).
///
class UsdShadeParameter
{
public:
    /// Default constructor returns an invalid Parameter.  Exists for 
    /// container classes
    UsdShadeParameter()
    {
        // nothing
    }

    /// Get the name of the UsdAttribute
    /// Since parameters do not live in a unique namespace, the parameter name 
    /// will always be identical to the UsdAttribute name.
    TfToken const GetName() const { 
        return _attr.GetName(); 
    }

    /// Get the "scene description" value type name for this attribute.
    SdfValueTypeName GetTypeName() const { 
        return _attr.GetTypeName(); 
    }

    /// \name Configuring the Parameter's Type
    /// @{
    
    /// Set the value for the shade parameter at \p time.
    bool Set(const VtValue& value, UsdTimeCode time = UsdTimeCode::Default()) const {
        return _attr.Set(value, time);
    }

    /// Set the value of the shade parameter at \p time.
    template <typename T>
    bool Set(const T & value, UsdTimeCode time = UsdTimeCode::Default()) const {
        return _attr.Set(value, time);
    }

    /// Specify an alternative, renderer-specific type to use when
    /// emitting/translating this parameter, rather than translating based
    /// on its GetTypeName()
    ///
    /// For example, we set the renderType to "struct" for parameters that
    /// are of renderman custom struct types.
    ///
    /// \return true on success
    USDSHADE_API
    bool SetRenderType(TfToken const& renderType) const;

    /// Return this parameter's specialized renderType, or an empty
    /// token if none was authored.
    ///
    /// \sa SetRenderType()
    USDSHADE_API
    TfToken GetRenderType() const;

    /// Return true if a renderType has been specified for this
    /// parameter.
    ///
    /// \sa SetRenderType()
    USDSHADE_API
    bool HasRenderType() const;

    /// @}

    /// \name Connections
    /// @{

    /// Connect parameter to a named output on a given \p source.
    ///
    /// This action simply records an introspectable relationship:
    /// it implies no actual dataflow in USD, and makes no statement
    /// about what client behavior should be when a Parameter
    /// is determined to possess both an authored value and a connection
    /// to a value source - client renderers are required to impose their
    /// own, self-consistent rules.
    ///
    /// The only constraint imposed by the shading model is that Parameter
    /// connections can be only single-targetted; that is, any given scalar
    /// parameter can target at most a single source/outputName pair.
    ///
    /// \param source the shader or node-graph object producing the value
    /// \param sourceName the particular computation or parameter we 
    ///        want to consume. This does not include the namespace prefix 
    ///        associated with the source type.
    /// \param sourceType the source of the connection can be an output, a 
    ///        parameter or an interface attribute. Each one is namespaced 
    ///        differently, so it is important to know the type of the source
    ///        attribute. By default we assume we are connecting to a 
    ///        computational output, but you can specify instead a parameter 
    ///        or an interface attribute (assuming your renderer supports it).
    ///        
    /// \sa GetConnectedSource(), GetConnectedSources()
    ///
    USDSHADE_API
    bool ConnectToSource(
            UsdShadeConnectableAPI const &source, 
            TfToken const &outputName,
            UsdShadeAttributeType sourceType=
                UsdShadeAttributeType::Output) const;

    /// \overload
    /// Connect parameter to the source, whose location is specified by \p
    /// sourcePath.
    /// 
    /// \p sourcePath should be the properly namespaced property path. 
    /// 
    /// This overload is provided for convenience, for use in contexts where 
    /// the prim types are unknown or unavailable.
    /// 
    USDSHADE_API
    bool ConnectToSource(const SdfPath &sourcePath) const;

    /// \overload
    ///
    /// Connects this parameter to the given parameter.
    /// 
    /// Once we flip the directionality of interface attributes and replace 
    /// them with inputs (that are simply UsdShadeParameters), we will 
    /// have parameter-to-parameter (or input-to-input) connections.
    /// 
    USDSHADE_API
    bool ConnectToSource(UsdShadeParameter const &param) const;

    /// \overload
    ///
    /// Connects this parameter to the given output.
    /// 
    USDSHADE_API
    bool ConnectToSource(UsdShadeOutput const &output) const;

    /// \overload
    ///
    /// Connects this parameter to the given interface attribute.
    /// 
    USDSHADE_API
    bool ConnectToSource(UsdShadeInterfaceAttribute const &interfaceAttribute) 
        const;

    /// \overload
    ///
    /// Connects this parameter to the given input.
    /// 
    USDSHADE_API
    bool ConnectToSource(UsdShadeInput const &input) const;

    /// Disconnect source for this Parameter.
    ///
    /// This may author more scene description than you might expect - we define
    /// the behavior of disconnect to be that, even if a parameter becomes
    /// connected in a weaker layer than the current UsdEditTarget, the
    /// Parameter will \em still be disconnected in the composition, therefore
    /// we must "block" it (see for e.g. UsdRelationship::BlockTargets()) in
    /// the current UsdEditTarget. 
    ///
    /// \sa ConnectToSource().
    USDSHADE_API
    bool DisconnectSource() const;

    /// Clears source for this Parameter in the current UsdEditTarget.
    ///
    /// Most of the time, what you probably want is DisconnectSource()
    /// rather than this function.
    ///
    /// \sa DisconnectSource(), UsdRelationship::ClearTargets()
    USDSHADE_API
    bool ClearSource() const;

    /// If this parameter is connected, retrieve the \p source prim
    /// and \p sourceName to which it is connected.
    ///
    /// We name the object that a parameter is connected to a "source," as 
    /// the "source" produces or contains a value for the parameter.
    /// 
    /// \return 
    /// \c true if \p source is a defined prim on the stage, and 
    /// \p source has an attribute that is either a parameter or output;
    ///
    /// \c false if not connected to a defined prim.
    ///
    /// \note The python wrapping for this method returns a 
    /// (source, sourceName, sourceType) tuple if the parameter is connected, 
    /// else \c None
    USDSHADE_API
    bool GetConnectedSource(
            UsdShadeConnectableAPI *source, 
            TfToken *sourceName,
            UsdShadeAttributeType *sourceType) const;

    /// Returns true if and only if the parameter is currently connected to the
    /// output of another \em defined shader object.
    ///
    /// If you will be calling GetConnectedSource() afterwards anyways, 
    /// it will be \em much faster to instead guard like so:
    /// \code
    /// if (param.GetConnectedSource(&p, &s)){
    ///      // process connected parameter
    /// } else {
    ///      // process unconnected parameter
    /// }
    /// \endcode
    USDSHADE_API
    bool IsConnected() const;

    /// \deprecated
    /// 
    /// Return the name of the sibling relationship that would encode
    /// the connection for this parameter.
    /// 
    USDSHADE_API
    TfToken GetConnectionRelName() const;

    // ---------------------------------------------------------------
    /// \name UsdAttribute API
    // ---------------------------------------------------------------

    /// @{

    typedef const UsdAttribute UsdShadeParameter::*_UnspecifiedBoolType;

    /// Speculative constructor that will produce a valid UsdShadeParameter when
    /// \p attr already represents an attribute that is Parameter, and
    /// produces an \em invalid Parameter otherwise (i.e. 
    /// \ref UsdShadeParameter_bool_type "unspecified-bool-type()" will return
    /// false).
    USDSHADE_API
    explicit UsdShadeParameter(const UsdAttribute &attr);

    /// Test whether a given UsdAttribute represents a valid Parameter, which
    /// implies that creating a UsdShadeParameter from the attribute will succeed.
    ///
    /// Success implies that \c attr.IsDefined() is true.
    //static bool IsShaderParameter(const UsdAttribute &attr);
    
    /// Allow UsdShadeParameter to auto-convert to UsdAttribute, so you can
    /// pass a UsdShadeParameter to any function that accepts a UsdAttribute or
    /// const-ref thereto.
    operator UsdAttribute const& () const { return _attr; }

    /// Explicit UsdAttribute extractor
    UsdAttribute const &GetAttr() const { return _attr; }

    // TODO
    /// Return true if the wrapped UsdAttribute::IsDefined(), and in
    /// addition the attribute is identified as a Parameter.
    bool IsDefined() const { return 
        _attr;
        //IsShaderParameter(_attr); 
    }

    /// @}

    /// \anchor UsdShadeParameter_bool_type
    /// Return true if this parameter is valid for querying and authoring
    /// values and metadata, which is identically equivalent to IsDefined().
#ifdef doxygen
    operator unspecified-bool-type() const();
#else
    operator _UnspecifiedBoolType() const {
        return IsDefined() ? &UsdShadeParameter::_attr : 0;
    }
#endif // doxygen

private:
    friend class UsdShadeShader;

    UsdShadeParameter(
            UsdPrim prim,
            TfToken const &name,
            SdfValueTypeName const &typeName);
    
    UsdAttribute _attr;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSHADE_PARAMETER_H
