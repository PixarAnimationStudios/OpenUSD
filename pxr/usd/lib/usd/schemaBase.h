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
#ifndef USD_SCHEMABASE_H
#define USD_SCHEMABASE_H

#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/references.h"

/// \class UsdSchemaBase
///
/// The base class for all schema types in Usd.
/// 
/// Schema objects hold a ::UsdPrim internally and provide a layer of specific
/// named API atop the underlying scene graph.
///
/// Schema objects are polymorphic but they are intended to be created as
/// automatic local variables, so they may be passed and returned by-value.
/// This leaves them subject to <a
/// href="http://en.wikipedia.org/wiki/Object_slicing">slicing</a>.  This means
/// that if one passes a <tt>SpecificSchema</tt> instance to a function that
/// takes a UsdSchemaBase \e by-value, all the polymorphic behavior specific to
/// <tt>SpecificSchema</tt> is lost.
///
/// To avoid slicing, it is encouraged that functions taking schema object
/// arguments take them by <tt>const &</tt> if const access is sufficient,
/// otherwise by non-const pointer.
///
class UsdSchemaBase {
    typedef const Usd_PrimDataHandle UsdSchemaBase::*_UnspecifiedBoolType;

public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Construct and store \p prim as the held prim.
	USD_API explicit UsdSchemaBase(const UsdPrim& prim = UsdPrim());

    /// Construct and store for the same prim held by \p otherSchema
	USD_API explicit UsdSchemaBase(const UsdSchemaBase& otherSchema);

    /// Destructor.
	USD_API virtual ~UsdSchemaBase();

    /// \name Held prim access.
    //@{

    /// Return this schema object's held prim.
    UsdPrim GetPrim() const { return _primData; }

    /// Shorthand for GetPrim()->GetPath().
    SdfPath GetPath() const { return _primData->GetPath(); }

    //@}

    /// \name PrimDefinition access.
    //@{

    /// Return the prim definition associated with this schema instance if one
    /// exists, otherwise return null.  This does not use the held prim's type.
    /// To get the held prim instance's definition, use
    /// UsdPrim::GetPrimDefinition().  \sa UsdPrim::GetPrimDefinition()
	USD_API SdfPrimSpecHandle GetSchemaClassPrimDefinition() const;

    //@}

    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true)
    {
        /* This only exists for consistency */
        static TfTokenVector names;
        return names;
    }

    /// Return true if this schema object is compatible with its held prim,
    /// false otherwise.  For untyped schemas return true if the held prim is
    /// not expired, otherwise return false.  For typed schemas return true if
    /// the held prim is not expired and its type is the schema's type or a
    /// subtype of the schema's type.  Otherwise return false.  This method
    /// invokes polymorphic behavior.
#ifdef doxygen
    operator unspecified-bool-type() const();
#else
    operator _UnspecifiedBoolType() const {
        return (_primData and
                _IsCompatible(_primData)) ? &UsdSchemaBase::_primData : NULL;
    }
#endif // doxygen

protected:
    // Helper for subclasses to get the TfType for this schema object's dynamic
    // C++ type.
    const TfType &_GetType() const {
        return _GetTfType();
    }

    USD_API UsdAttribute _CreateAttr(TfToken const &attrName,
                             SdfValueTypeName const & typeName,
                             bool custom, SdfVariability variability,
                             VtValue const &defaultValue, 
                             bool writeSparsely) const;
    
private:
    // Subclasses may override _IsCompatible to do specific compatibility
    // checking with the given prim, such as type compatibility or value
    // compatibility.  This check is performed when clients invoke the
    // _UnspecifiedBoolType operator.
    USD_API virtual bool _IsCompatible(const UsdPrim &prim) const;

    // Subclasses should not override _GetTfType.  It is implemented by the
    // schema class code generator.
	USD_API virtual const TfType &_GetTfType() const;

    // The held prim.
    Usd_PrimDataHandle _primData;
};

#endif //USD_SCHEMABASE_H
