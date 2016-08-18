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

#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdShade/parameter.h"

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
    USDSHADE_API
    bool Get(VtValue* value, UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Returns the un-namespaced name of this interface attribute.
    USDSHADE_API
    TfToken GetName() const;

    /// Returns a list of all of the shader parameters in the specified
    /// \p renderTarget that should be driven by this InterfaceAttribute's
    /// authored value (if any).
    ///
    /// \todo Provide a way to retrieve \em all driven parameters of all
    /// render targets.
    USDSHADE_API
    std::vector<UsdShadeParameter> GetRecipientParameters(
            const TfToken& renderTarget) const;

    /// \name Authoring Values and Driving them to Recipient Parameters
    /// @{

    /// Set the value for the look attribute.
    USDSHADE_API
    bool Set(const VtValue& value, UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Make this InterfaceAttribute drive the value of UsdShadeParameter
    /// \p recipient
    /// 
    /// \p recipient should be a UsdShadeParameter on a shader for a
    /// \p renderTarget network.  This method resets the set of driven 
    /// parameters to, uniquely, \p recipient
    ///
    /// \return true if this was successfully authored.
    USDSHADE_API
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
    USDSHADE_API
    bool SetRecipient(
            const TfToken& renderTarget,
            const SdfPath& recipientPath) const;

    /// Set documentation string for this attribute.
    /// \sa UsdObject::SetDocumentation()
    USDSHADE_API
    bool SetDocumentation(
            const std::string& docs) const;

    /// Get documentation string for this attribute.
    /// \sa UsdObject::GetDocumentation()
    USDSHADE_API
    std::string GetDocumentation() const;

    /// Set the displayGroup metadata for this interface attribute,
    /// i.e. hinting for the location and nesting of the attribute.
    /// \sa UsdProperty::SetDisplayGroup(), UsdProperty::SetNestedDisplayGroup()
    USDSHADE_API
    bool SetDisplayGroup(
            const std::string& displayGroup) const;

    /// Get the displayGroup metadata for this interface attribute,
    /// i.e. hinting for the location and nesting of the attribute.
    /// \sa UsdProperty::GetDisplayGroup(), UsdProperty::GetNestedDisplayGroup()
    USDSHADE_API
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
    USDSHADE_API
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
    TfToken _name;
};

#endif // USDSHADE_INTERFACEATTRIBUTE_H
