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
#ifndef USDSHADE_INTERFACEATTRIBUTE_H
#define USDSHADE_INTERFACEATTRIBUTE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdShade/parameter.h"

PXR_NAMESPACE_OPEN_SCOPE


class UsdShadeParameter;
class UsdShadeOutput;

/// \class UsdShadeInterfaceAttribute
///
/// Schema wrapper for UsdAttribute for authoring and introspecting
/// interface attributes, which are attributes on a UsdShadeSubgraph that provide
/// values that can be instanced onto UsdShadeParameter's in shading networks .
///
/// See \ref UsdShadeSubgraph_Interfaces "Look Interface Attributes" for more 
/// detail on Look Interfaces and the API for using them.
///
class UsdShadeInterfaceAttribute
{
public:
    /// Default constructor returns an invalid InterfaceAttribute.  Exists for
    /// container classes
    UsdShadeInterfaceAttribute()
    {
        // nothing
    }

    /// Convenience wrapper for UsdAttribute::Get()
    bool Get(VtValue* value, UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Returns the un-namespaced name of this interface attribute.
    TfToken GetName() const;

    /// Get the "scene description" value type name of the attribute associated 
    /// with the interface attribute.
    SdfValueTypeName GetTypeName() const { 
        return  _attr.GetTypeName();
    }

    /// Returns a list of all of the shader parameters in the specified
    /// \p renderTarget that should be driven by this InterfaceAttribute's
    /// authored value (if any).
    ///
    /// \todo Provide a way to retrieve \em all driven parameters of all
    /// render targets.
    std::vector<UsdShadeParameter> GetRecipientParameters(
            const TfToken& renderTarget) const;

    /// \name Authoring Values and Driving them to Recipient Parameters
    /// @{

    /// Set the value for the look attribute.
    bool Set(const VtValue& value, UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Set the attribute value of the interface attribute at \p time 
    template <typename T>
    bool Set(const T& value, UsdTimeCode time = UsdTimeCode::Default()) const {
        return _attr.Set(value, time);
    }

    /// Make this InterfaceAttribute drive the value of UsdShadeParameter
    /// \p recipient
    /// 
    /// \p recipient should be a UsdShadeParameter on a shader for a
    /// \p renderTarget network.  This method resets the set of driven 
    /// parameters to, uniquely, \p recipient
    ///
    /// \return true if this was successfully authored.
    bool SetRecipient(
            const TfToken& renderTarget,
            const UsdShadeParameter& recipient) const;

    /// Make this InterfaceAttribute drive the value of UsdShadeParameter
    /// \p recipient
    /// 
    /// \overload
    /// \p recipientPath should be the path to a UsdShadeParameter on a shader
    /// for a \p renderTarget network.  This version of the function is useful
    /// if you're connecting to something that may not exist in the current 
    /// stage.
    bool SetRecipient(
            const TfToken& renderTarget,
            const SdfPath& recipientPath) const;

    /// \name Connections
    /// 
    /// Interface attributes on subgraphs are connectable.
    /// 
    /// Subgraphs can be connected to interface attributes on enclosing material 
    /// prims or to outputs of shaders in the material.
    /// 
    /// @{

    /// Connect this interface attribute to a named input on a given \p source.
    ///
    /// This action simply records an introspectable relationship:
    /// it implies no actual dataflow in USD, and makes no statement
    /// about what client behavior should be when an interface attribute
    /// is determined to possess both a value and a connection
    /// to a value source - client renderers are required to impose their
    /// own, self-consistent rules.
    ///
    /// The only constraint imposed by the shading model is that interface 
    /// attribute connections can be only single-targetted; that is, any given 
    /// interface attribute can target at most a single source/sourceName pair.
    ///
    /// \param source  the shader or subgraph object producing the value
    ///        
    /// \param sourceName  the particular computation or parameter we 
    ///        want to consume. This does not include the namespace prefix
    ///        associated with the source type.
    ///
    /// \param sourceType outputs, parameters and interfaceAttributes are 
    ///        namespaced differently on a connectable prim, therefore we need 
    ///        to know to which we are connecting.  By default, we assume we are
    ///        connecting to a computational output, but you can specify
    ///        instead an input with a value of 
    ///        \c UsdShadeAttributeType::Input or an interface 
    ///        attribute with a value of 
    ///        \c UsdShadeAttributeType::InterfaceAttribute. 
    ///        
    /// Interface attribute are typically connected to interface attributes 
    /// on enclosing the material or to outputs of shaders in the material's 
    /// shading network. 
    ///       
    /// \sa GetConnectedSource(), GetConnectedSources()
    bool ConnectToSource(
            UsdShadeConnectableAPI const &source, 
            TfToken const &sourceName,
            UsdShadeAttributeType sourceType=
                UsdShadeAttributeType::Output) const;

    /// \overload
    /// Connect InterfaceAttribute to the source, whose location is specified 
    /// by \p sourcePath.
    /// 
    /// \p sourcePath should be the properly namespaced property path. 
    /// 
    /// This overload is provided for convenience, for use in contexts where 
    /// the prim types are unknown or unavailable.
    /// 
    bool ConnectToSource(const SdfPath &sourcePath) const;

    /// \overload
    ///
    /// Connects this interface attribute to the given \p param.
    /// 
    bool ConnectToSource(UsdShadeParameter const &param) const;

    /// \overload
    ///
    /// Connects this interface attribute to the given \p output.
    /// 
    bool ConnectToSource(UsdShadeOutput const &output) const;

    /// \overload
    ///
    /// Connects this interface attribute to the given \p interfaceAttribute.
    /// 
    bool ConnectToSource(UsdShadeInterfaceAttribute const &interfaceAttribute) const;

    /// Disconnect source for this interface attribute.
    ///
    /// This may author more scene description than you might expect - we define
    /// the behavior of disconnect to be that, even if an interface attribute 
    /// becomes connected in a weaker layer than the current UsdEditTarget, it 
    /// will \em still be disconnected in the composition, therefore we must 
    /// "block" it (see for e.g. UsdRelationship::BlockTargets()) in the current 
    /// UsdEditTarget. 
    ///
    /// \sa ConnectToSource().
    bool DisconnectSource() const;

    /// Clears source for this interface attribute in the current UsdEditTarget.
    ///
    /// Most of the time, what you probably want is DisconnectSource()
    /// rather than this function.
    ///
    /// \sa DisconnectSource(), UsdRelationship::ClearTargets()
    bool ClearSource() const;

    /// If this interface attribute is connected, retrieve the \p source prim
    /// and \p sourceName, which is the name of the parameter, output or 
    /// interface attribute to which it is connected. \p sourceType indicates 
    /// the type of the source.
    ///
    /// We name the object that an interface attribute is connected to a 
    /// "source," as the "source" produces or contains a value for the 
    /// interface attribute.
    /// 
    /// \return 
    /// \c true if \p source is a defined prim on the stage, and 
    /// \p source has an attribute that connects to this interface attribute;
    /// \c false if not connected to a defined prim.
    ///
    /// \note The python wrapping for this method returns a 
    /// (source, sourceName, sourceType) tuple if the interface attribute is 
    /// connected, else \c None
    bool GetConnectedSource(
            UsdShadeConnectableAPI *source, 
            TfToken *sourceName,
            UsdShadeAttributeType *sourceType) const;

    /// Returns true if and only if the interface attribute is currently 
    /// connected to another \em defined object.
    ///
    /// If you will be calling GetConnectedSource() afterwards anyways, 
    /// it will be \em much faster to instead guard like so:
    /// \code
    /// if (interfaceAttribute.GetConnectedSource(&src, &sourceName, 
    ///                                           &sourceType)){
    ///      // process connected interfaceAttribute
    /// } else {
    ///      // process unconnected interfaceAttribute
    /// }
    /// \endcode
    bool IsConnected() const;

    /// Return the name of the sibling relationship that would encode
    /// the connection for this interface attribute.
    TfToken GetConnectionRelName() const;

    /// @}

    /// Set documentation string for this attribute.
    /// \sa UsdObject::SetDocumentation()
    bool SetDocumentation(
            const std::string& docs) const;

    /// Get documentation string for this attribute.
    /// \sa UsdObject::GetDocumentation()
    std::string GetDocumentation() const;

    /// Set the displayGroup metadata for this interface attribute,
    /// i.e. hinting for the location and nesting of the attribute.
    /// \sa UsdProperty::SetDisplayGroup(), UsdProperty::SetNestedDisplayGroup()
    bool SetDisplayGroup(
            const std::string& displayGroup) const;

    /// Get the displayGroup metadata for this interface attribute,
    /// i.e. hinting for the location and nesting of the attribute.
    /// \sa UsdProperty::GetDisplayGroup(), UsdProperty::GetNestedDisplayGroup()
    std::string GetDisplayGroup() const;

    /// @}
    // ---------------------------------------------------------------
    /// \name UsdAttribute API
    // ---------------------------------------------------------------
    /// @{

    typedef const UsdAttribute UsdShadeInterfaceAttribute::*_UnspecifiedBoolType;

    /// Speculative constructor that will produce a valid
    /// UsdShadeInterfaceAttribute when
    /// \p attr already represents an attribute that is Parameter, and
    /// produces an \em invalid Parameter otherwise (i.e. 
    /// \ref UsdShadeParameter_bool_type "unspecified-bool-type()" will return
    /// false).
    explicit UsdShadeInterfaceAttribute(const UsdAttribute &attr);

    /// Test whether a given UsdAttribute represents valid Primvar, which
    /// implies that creating a UsdShadeInterfaceAttribute from the attribute will succeed.
    ///
    /// Success implies that \c attr.IsDefined() is true.
    //static bool IsShaderInterfaceAttribute(const UsdAttribute &attr);
    
    /// Allow UsdShadeInterfaceAttribute to auto-convert to UsdAttribute, so you can
    /// pass a UsdShadeInterfaceAttribute to any function that accepts a UsdAttribute or
    /// const-ref thereto.
    operator UsdAttribute const& () const { return _attr; }

    /// Explicit UsdAttribute extractor
    UsdAttribute const &GetAttr() const { return _attr; }

    // TODO
    /// Return true if the wrapped UsdAttribute::IsDefined(), and in
    /// addition the attribute is identified as a Primvar.
    bool IsDefined() const { return 
        _attr;
        //IsShaderInterfaceAttribute(_attr); 
    }

    /// @}

    /// \anchor UsdShadeInterfaceAttribute
    /// Return true if this Primvar is valid for querying and authoring
    /// values and metadata, which is identically equivalent to IsDefined().
#ifdef doxygen
    operator unspecified-bool-type() const();
#else
    operator _UnspecifiedBoolType() const {
        return IsDefined() ? &UsdShadeInterfaceAttribute::_attr : 0;
    }
#endif // doxygen
private:
    friend class UsdShadeSubgraph;

    static std::string _GetInterfaceAttributeRelPrefix(
            const TfToken& relTarget);

    static TfToken _GetName(TfToken const& name);
    UsdShadeInterfaceAttribute(
            const UsdPrim& prim,
            TfToken const& name,
            SdfValueTypeName const& typeName);

    UsdAttribute _attr;

    // The unnamespaced nane of the interface attribute.
    TfToken _name;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSHADE_INTERFACEATTRIBUTE_H
