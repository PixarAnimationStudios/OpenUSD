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
#ifndef USDSHADE_INPUT_H
#define USDSHADE_INPUT_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdShade/utils.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdShadeConnectableAPI;

/// \class UsdShadeInput
/// 
/// This class encapsulates a shader or subgraph input, which is a 
/// connectable property representing a typed value.
/// 
class UsdShadeInput
{
public:
    /// Default constructor returns an invalid Input.  Exists for the sake of
    /// container classes
    UsdShadeInput()
    {
        // nothing
    }

    /// Get the name of the attribute associated with the Input. 
    /// 
    TfToken const &GetFullName() const { 
        return _attr.GetName(); 
    }

    /// Returns the name of the input. 
    /// 
    /// We call this the base name since it strips off the "inputs:" namespace 
    /// prefix from the attribute name, and returns it.
    /// 
    TfToken GetBaseName() const;

    /// Get the "scene description" value type name of the attribute associated 
    /// with the Input.
    /// 
    SdfValueTypeName GetTypeName() const;
    
    /// Get the prim that the input belongs to.
    UsdPrim GetPrim() const {
        return _attr.GetPrim();
    }

    /// Set a value for the Input at \p time.
    /// 
    bool Set(const VtValue& value, 
             UsdTimeCode time = UsdTimeCode::Default()) const;

    /// \overload 
    /// Set a value of the Input at \p time.
    /// 
    template <typename T>
    bool Set(const T& value, UsdTimeCode time = UsdTimeCode::Default()) const {
        return _attr.Set(value, time);
    }

    /// Hash functor.
    struct Hash {
        inline size_t operator()(const UsdShadeInput &input) const {
            return hash_value(input._attr);
        }
    };

    /// \name Configuring the Input's Type
    /// @{

    /// Specify an alternative, renderer-specific type to use when
    /// emitting/translating this Input, rather than translating based
    /// on its GetTypeName()
    ///
    /// For example, we set the renderType to "struct" for Inputs that
    /// are of renderman custom struct types.
    ///
    /// \return true on success.
    ///
    bool SetRenderType(TfToken const& renderType) const;

    /// Return this Input's specialized renderType, or an empty
    /// token if none was authored.
    ///
    /// \sa SetRenderType()
    TfToken GetRenderType() const;

    /// Return true if a renderType has been specified for this Input.
    ///
    /// \sa SetRenderType()
    bool HasRenderType() const;

    /// @}

    // ---------------------------------------------------------------
    /// \name UsdAttribute API
    // ---------------------------------------------------------------

    /// @{

    typedef const UsdAttribute UsdShadeInput::*_UnspecifiedBoolType;

    /// Speculative constructor that will produce a valid UsdShadeInput when
    /// \p attr already represents a shade Input, and produces an \em invalid 
    /// UsdShadeInput otherwise (i.e. \ref UsdShadeInput_bool_type 
    /// "unspecified-bool-type()" will return false).
    explicit UsdShadeInput(const UsdAttribute &attr);

    /// Test whether a given UsdAttribute represents a valid Input, which
    /// implies that creating a UsdShadeInput from the attribute will succeed.
    ///
    /// Success implies that \c attr.IsDefined() is true.
    static bool IsInput(const UsdAttribute &attr);

    /// Explicit UsdAttribute extractor.
    const UsdAttribute &GetAttr() const { return _attr; }

    /// Allow UsdShadeInput to auto-convert to UsdAttribute, so you can
    /// pass a UsdShadeInput to any function that accepts a UsdAttribute or
    /// const-ref thereto.
    operator const UsdAttribute & () const { return GetAttr(); }

    /// Return true if the wrapped UsdAttribute is defined, and in addition the 
    /// attribute is identified as an input.
    bool IsDefined() const {
        return _attr and IsInput(_attr);
    }

    /// @}

    /// Return true if this Input is valid for querying and authoring
    /// values and metadata, which is identically equivalent to IsDefined().
    explicit operator bool() { 
        return IsDefined(); 
    }

    /// Equality comparison. Returns true if \a lhs and \a rhs represent the 
    /// same UsdShadeInput, false otherwise.
    friend bool operator==(const UsdShadeInput &lhs, const UsdShadeInput &rhs) {
        return lhs.GetAttr() == rhs.GetAttr();
    }

private:
    friend class UsdShadeConnectableAPI;

    // Constructor that creates a UsdShadeInput with the given name on the 
    // given prim.
    // \p name here is the unnamespaced name of the input.
    UsdShadeInput(UsdPrim prim,
                  TfToken const &name,
                  SdfValueTypeName const &typeName);
    
    UsdAttribute _attr;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSHADE_INPUT_H
