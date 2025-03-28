//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_SCHEMA_BASE_H
#define PXR_USD_USD_SCHEMA_BASE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/references.h"

PXR_NAMESPACE_OPEN_SCOPE


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
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind in usd/common.h
    static const UsdSchemaKind schemaKind = UsdSchemaKind::AbstractBase;

    /// Returns whether or not this class corresponds to a concrete instantiable
    /// prim type in scene description.  If this is true,
    /// GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    bool IsConcrete() const {
        return GetSchemaKind() == UsdSchemaKind::ConcreteTyped;
    }

    /// Returns whether or not this class inherits from UsdTyped. Types which
    /// inherit from UsdTyped can impart a typename on a UsdPrim.
    bool IsTyped() const {
        return GetSchemaKind() == UsdSchemaKind::ConcreteTyped
            || GetSchemaKind() == UsdSchemaKind::AbstractTyped;
    }

    /// Returns whether this is an API schema or not.
    bool IsAPISchema() const {
        return GetSchemaKind() == UsdSchemaKind::NonAppliedAPI
            || GetSchemaKind() == UsdSchemaKind::SingleApplyAPI
            || GetSchemaKind() == UsdSchemaKind::MultipleApplyAPI;
    }

    /// Returns whether this is an applied API schema or not. If this returns
    /// true this class will have an Apply() method
    bool IsAppliedAPISchema() const {
        return GetSchemaKind() == UsdSchemaKind::SingleApplyAPI
            || GetSchemaKind() == UsdSchemaKind::MultipleApplyAPI;
    }

    /// Returns whether this is an applied API schema or not. If this returns
    /// true the constructor, Get and Apply methods of this class will take
    /// in the name of the API schema instance.
    bool IsMultipleApplyAPISchema() const {
        return GetSchemaKind() == UsdSchemaKind::MultipleApplyAPI;
    }

    /// Returns the kind of schema this class is.
    UsdSchemaKind GetSchemaKind() const {
        return _GetSchemaKind();
    }

    /// Construct and store \p prim as the held prim.
    USD_API
    explicit UsdSchemaBase(const UsdPrim& prim = UsdPrim());

    /// Construct and store for the same prim held by \p otherSchema
    USD_API
    explicit UsdSchemaBase(const UsdSchemaBase& otherSchema);

    /// Destructor.
    USD_API
    virtual ~UsdSchemaBase();

    /// \name Held prim access.
    //@{

    /// Return this schema object's held prim.
    UsdPrim GetPrim() const { return UsdPrim(_primData, _proxyPrimPath); }

    /// Shorthand for GetPrim()->GetPath().
    SdfPath GetPath() const { 
        if (!_proxyPrimPath.IsEmpty()) {
            return _proxyPrimPath;
        }
        else if (Usd_PrimDataConstPtr p = get_pointer(_primData)) {
            return p->GetPath();
        }
        return SdfPath::EmptyPath();
    }

    //@}

    /// \name PrimDefinition access.
    //@{

    /// Return the prim definition associated with this schema instance if one
    /// exists, otherwise return null.  This does not use the held prim's type.
    /// To get the held prim instance's definition, use
    /// UsdPrim::GetPrimDefinition().  \sa UsdPrim::GetPrimDefinition()
    USD_API
    const UsdPrimDefinition *GetSchemaClassPrimDefinition() const;

    //@}

    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true)
    {
        /* This only exists for consistency */
        static TfTokenVector names;
        return names;
    }

    /// \anchor UsdSchemaBase_bool
    /// Return true if this schema object is compatible with its held prim,
    /// false otherwise.  For untyped schemas return true if the held prim is
    /// not expired, otherwise return false.  For typed schemas return true if
    /// the held prim is not expired and its type is the schema's type or a
    /// subtype of the schema's type.  Otherwise return false.  This method
    /// invokes polymorphic behavior.
    /// 
    /// \sa UsdSchemaBase::_IsCompatible()
    USD_API
    explicit operator bool() const {
        return _primData && _IsCompatible();
    }

protected:
    /// Returns the kind of schema this class is.
    ///
    /// \sa UsdSchemaBase::schemaKind
    virtual UsdSchemaKind _GetSchemaKind() const {
        return schemaKind;
    }

    /// \deprecated
    /// This has been replace with _GetSchemaKind but is around for now for 
    /// backwards compatibility while schemas are being updated.
    ///
    /// Leaving this around for one more release as schema classes up until now
    /// have been generated with an override of this function. We don't want 
    /// those classes to immediately not compile before a chance is given to 
    /// regenerate the schemas.
    virtual UsdSchemaKind _GetSchemaType() const {
        return schemaKind;
    }

    // Helper for subclasses to get the TfType for this schema object's dynamic
    // C++ type.
    const TfType &_GetType() const {
        return _GetTfType();
    }

    USD_API
    UsdAttribute _CreateAttr(TfToken const &attrName,
                             SdfValueTypeName const & typeName,
                             bool custom, SdfVariability variability,
                             VtValue const &defaultValue, 
                             bool writeSparsely) const;
    
    /// Subclasses may override _IsCompatible to do specific compatibility
    /// checking with the given prim, such as type compatibility or value
    /// compatibility.  This check is performed when clients invoke the
    /// explicit bool operator.
    USD_API
    virtual bool _IsCompatible() const;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USD_API
    static const TfType &_GetStaticTfType();

    // Subclasses should not override _GetTfType.  It is implemented by the
    // schema class code generator.
    USD_API
    virtual const TfType &_GetTfType() const;

    // The held prim and proxy prim path.
    Usd_PrimDataHandle _primData;
    SdfPath _proxyPrimPath;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_SCHEMA_BASE_H
