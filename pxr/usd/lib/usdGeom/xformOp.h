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
#ifndef USDGEOM_XFORMOP_H
#define USDGEOM_XFORMOP_H

/// \file xformOp.h

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <string>
#include <vector>
#include <typeinfo>

#include <boost/variant.hpp>

#include "pxr/base/tf/staticTokens.h"

/// \hideinitializer
#define USDGEOM_XFORM_OP_TYPES \
    (translate) \
    (scale) \
    (rotateX) \
    (rotateY) \
    (rotateZ) \
    (rotateXYZ) \
    (rotateXZY) \
    (rotateYXZ) \
    (rotateYZX) \
    (rotateZXY) \
    (rotateZYX) \
    (orient) \
    (transform) \
    ((resetXformStack, "!resetXformStack!"))

/// \anchor UsdGeomXformOpTypes
/// Provides TfToken's for use in conjunction with UsdGeomXformable::Add
/// XformOp() and UsdGeomXformOp::GetOpType(), to establish op type.
///
/// The component operation names and their meanings are:
/// \li <b>translate</b> - XYZ translation
/// \li <b>scale</b> - XYZ scale
/// \li <b>rotateX</b> - rotation about the X axis, <b>in degrees</b>
/// \li <b>rotateY</b> - rotation about the Y axis, <b>in degrees</b>
/// \li <b>rotateZ</b> - rotation about the Z axis, <b>in degrees</b>
/// \li <b>rotateXYZ, rotateXZY, rotateYXZ, rotateYZX, rotateZXY, rotateZYX</b>
///     - a set of three canonical Euler rotations, packed into a single
///     Vec3, for conciseness and efficiency of reading.  The \\em first
///     axis named is the most local, so a single rotateXYZ is equivalent to
///     the ordered ops "rotateZ rotateY rotateX". See also 
///     \ref usdGeom_rotationPackingOrder "note on rotation packing order."
/// \li <b>orient</b>  - arbitrary axis/angle rotation, expressed as a quaternion
/// \li <b>transform</b> - 4x4 matrix transformation
/// \li <b>resetXformStack</b> - when appearing as the first op, instructs 
///     client that the transform stack should be cleared of any inherited
///     transformation prior to processing the rest of the prims ops.  It is
///     an error for resetXformStack to appear anywhere other than as the
///     first element in \em xformOpOrder.
TF_DECLARE_PUBLIC_TOKENS(UsdGeomXformOpTypes, USDGEOM_XFORM_OP_TYPES);

/// \class UsdGeomXformOp
///
/// Schema wrapper for UsdAttribute for authoring and computing
/// transformation operations, as consumed by UsdGeomXformable schema.
/// 
/// The semantics of an op are determined primarily by its name, which allows
/// us to decode an op very efficiently.  All ops are independent attributes,
/// which must live in the "xformOp" property namespace.  The op's primary name
/// within the namespace must be one of \ref UsdGeomXformOpTypes, which
/// determines the type of transformation operation, and its secondary name 
/// (or suffix) within the namespace (which is not required to exist), can be 
/// any name that distinguishes it from other ops of the same type. Suffixes 
/// are generally imposed by higer level xform API schemas.
///
/// \anchor usdGeom_rotationPackingOrder
/// \note 
/// <b>On packing order of rotateABC triples</b><br>
/// The order in which the axis rotations are recorded in a Vec3* for the
/// six \em rotateABC Euler triples <b>is always the same:</b> vec[0] = X,
/// vec[1] = Y, vec[2] = Z .  The \em A, \em B, \em C in the op name dictate
/// the order in which their corresponding elements are consumed by the 
/// rotation, not how they are laid out.
///
class UsdGeomXformOp
{
public:
  
    /// Enumerates the set of all transformation operation types.
    enum Type {
        TypeInvalid,   ///< Represents an invalid xformOp.
        TypeTranslate, ///< XYZ translation.
        TypeScale,     ///< XYZ scale.
        TypeRotateX,   ///< Rotation about the X-axis, <b>in degrees</b>.
        TypeRotateY,   ///< Rotation about the Y-axis, <b>in degrees</b>.
        TypeRotateZ,   ///< Rotation about the Z-axis, <b>in degrees</b>.
        TypeRotateXYZ, ///< Set of 3 canonical Euler rotations
                       ///  \ref usdGeom_rotationPackingOrder "in XYZ order"
        TypeRotateXZY, ///< Set of 3 canonical Euler rotations 
                       /// \ref usdGeom_rotationPackingOrder "in XZY order"
        TypeRotateYXZ, ///< Set of 3 canonical Euler rotations 
                       /// \ref usdGeom_rotationPackingOrder "in YXZ order"
        TypeRotateYZX, ///< Set of 3 canonical Euler rotations 
                       /// \ref usdGeom_rotationPackingOrder "in YZX order"
        TypeRotateZXY, ///< Set of 3 canonical Euler rotations 
                       /// \ref usdGeom_rotationPackingOrder "in ZXY order"
        TypeRotateZYX, ///< Set of 3 canonical Euler rotations 
                       /// \ref usdGeom_rotationPackingOrder "in ZYX order"
        TypeOrient,    ///< Arbitrary axis/angle rotation, expressed as a quaternion.
        TypeTransform  ///< A 4x4 matrix transformation.
    };

    /// Precision with which the value of the tranformation operation is encoded.
    enum Precision {
        PrecisionDouble, ///< Double precision
        PrecisionFloat,  ///< Floating-point precision 
        PrecisionHalf    ///< Half-float precision
    };

    // Default constructor returns an invalid XformOp.  Exists for 
    // container classes
    UsdGeomXformOp()
    {
        /* NOTHING */
    }
    
    /// Speculative constructor that will produce a valid UsdGeomXformOp when
    /// \p attr already represents an attribute that is XformOp, and
    /// produces an \em invalid XformOp otherwise (i.e. 
    /// unspecified_bool_type() will return false).
    ///
    /// Calling \c UsdGeomXformOp::IsXformOp(attr) will return the same truth
    /// value as this constructor, but if you plan to subsequently use the
    /// XformOp anyways, just use this constructor.
    /// 
    /// \p isInverseOp is set to true to indicate an inverse transformation 
    /// op.
    /// 
    /// This constructor exists mainly for internal use. Clients should use
    /// AddXformOp API (or one of Add*Op convenience API) to create and retain 
    /// a copy of an UsdGeomXformOp object.
    /// 
    explicit UsdGeomXformOp(const UsdAttribute &attr, bool isInverseOp=false);

    // -------------------------------------------------------
    /// \name Static Helper API
    // -------------------------------------------------------

    /// Test whether a given UsdAttribute represents valid XformOp, which
    /// implies that creating a UsdGeomXformOp from the attribute will succeed.
    ///
    /// Success implies that \c attr.IsDefined() is true.
    static bool IsXformOp(const UsdAttribute &attr);

    /// Test whether a given attrbute name represents a valid XformOp, which
    /// implies that creating a UsdGeomXformOp from the corresponding 
    /// UsdAttribute will succeed.
    ///
    static bool IsXformOp(const TfToken &attrName);

    /// Returns the TfToken used to encode the given \p opType.
    static TfToken const &GetOpTypeToken(Type const opType);

    /// Returns the Type enum associated with the given \p opTypeToken.
    static Type GetOpTypeEnum(TfToken const &opTypeToken);

    /// Returns the precision corresponding to the given value typeName.
    static Precision GetPrecisionFromValueTypeName(const SdfValueTypeName& typeName);

    /// Returns the value typeName token that corresponds to the given 
    /// combination of \p opType and \p precision.
    static const SdfValueTypeName &GetValueTypeName(const Type opType,
                                                    const Precision precision);

    /// Returns the xformOp's name as it appears in xformOpOrder, given 
    /// the opType, the (optional) suffix and whether it is an inverse 
    /// operation.
    static TfToken GetOpName(const Type opType, 
                             const TfToken &opSuffix=TfToken(),
                             bool inverse=false);

    // -------------------------------------------------------
    /// \name Data Encoding Queries
    // -------------------------------------------------------

    /// Return the operation type of this op, one of \ref UsdGeomXformOp::Type
    Type GetOpType() const {
        return _opType;
    }

    /// Returns the precision level of the xform op.
    Precision GetPrecision() const;

    /// Returns whether the xformOp represents an inverse operation.
    bool IsInverseOp() const { 
        return _isInverseOp;
    }

    /// Returns the opName as it appears in the xformOpOrder attribute.
    /// 
    /// This will begin with "!invert!:xformOp:" if it is an inverse xform 
    /// operation. If it is not an inverse xformOp, it will begin with 'xformOp:'.
    /// 
    /// This will be empty for an invalid xformOp.
    /// 
    TfToken GetOpName() const;

    /// Does this op have the given suffix in its name.
    bool HasSuffix(TfToken const &suffix) const;
    
    // ---------------------------------------------------------------
    /// \name Computing with Ops
    // ---------------------------------------------------------------
    
    /// We allow ops to be encoded with varying degrees of precision,
    /// depending on the clients needs and constraints.  GetAs() will
    /// attempt to convert the stored data to the requested datatype.
    ///
    /// Note this accessor incurs some overhead beyond Get()'ing the 
    /// value as a VtValue and dealing with the results yourself.
    ///
    /// \return true if a value was successfully read \em and converted
    /// to the requested datatype (see \ref VtValue::Cast()), false
    /// otherwise.  A problem reading or failure to convert will cause
    /// an error to be emitted.
    ///
    /// \note the requested type \p T must be constructable by assignment
    template <typename T>
    bool GetAs(T* value, UsdTimeCode time) const {
        VtValue v;
        if (not Get(&v, time)) {
            return false;
        }
        v.Cast<T>();
        if (v.IsEmpty()){
            TfType thisType = GetTypeName().GetType();
            TF_CODING_ERROR("Unable to convert xformOp %s's value from %s to "
                            "requested type %s.", GetAttr().GetPath().GetText(),
                            thisType.GetTypeName().c_str(),
                            TfType::GetCanonicalTypeName(typeid(*value)).c_str());
            return false;
        }
        *value = v.UncheckedGet<T>();
        return true;
    }

    /// Return the 4x4 matrix that applies the transformation encoded
    /// by op \p opType and data value \p opVal. 
    /// 
    /// If \p isInverseOp is true, then the inverse of the tranformation 
    /// represented by the op/value pair is returned.
    /// 
    /// An error will be issued if \p opType is not one of the values in the enum
    /// \ref UsdGeomXformOp::Type or if \p opVal cannot be converted
    /// to a suitable input to \p opType
    static GfMatrix4d GetOpTransform(Type const opType, 
                                     VtValue const &opVal,
                                     bool isInverseOp=false);

    
    /// Return the 4x4 matrix that applies the transformation encoded
    /// in this op at \p time.
    ///
    /// Returns the identity matrix and issues a coding error if the op is 
    /// invalid. 
    /// 
    /// If the op is valid, but has no authored value, the identity 
    /// matrix is returned and no error is issued.
    /// 
    GfMatrix4d GetOpTransform(UsdTimeCode time) const;

    /// Determine whether there is any possibility that this op's value
    /// may vary over time.
    ///
    /// The determination is based on a snapshot of the authored state of the
    /// op, and may become invalid in the face of further authoring.
    bool MightBeTimeVarying() const {
        return boost::apply_visitor(_GetMightBeTimeVarying(), _attr);
    }

    // ---------------------------------------------------------------
    /// \name UsdAttribute API
    // ---------------------------------------------------------------
    
    /// Allow UsdGeomXformOp to auto-convert to UsdAttribute, so you can
    /// pass a UsdGeomXformOp to any function that accepts a UsdAttribute or
    /// const-ref thereto.
    operator UsdAttribute const& () const { return GetAttr(); }

    /// Explicit UsdAttribute extractor
    UsdAttribute const &GetAttr() const { 
        return boost::apply_visitor(_GetAttr(), _attr); 
    }
    
    /// Return true if the wrapped UsdAttribute::IsDefined(), and in
    /// addition the attribute is identified as a XformOp.
    bool IsDefined() const { return IsXformOp(GetAttr()); }

private:
    typedef const Type UsdGeomXformOp::*_UnspecifiedBoolType;

public:
    /// \anchor UsdGeomXformOp_bool_type
    /// Return true if this XformOp is valid for querying and authoring
    /// values and metadata, which is identically equivalent to IsDefined().
#ifdef doxygen
    operator unspecified_bool_type() const;
#else
    operator _UnspecifiedBoolType() const {
        return IsDefined() ? &UsdGeomXformOp::_opType : NULL;
    }
#endif // doxygen

    /// \sa UsdAttribute::GetName()
    TfToken const &GetName() const {  return GetAttr().GetName(); }

    /// \sa UsdAttribute::GetBaseName()
    TfToken GetBaseName() const { return GetAttr().GetBaseName(); }

    /// \sa UsdAttribute::GetNamespace()
    TfToken GetNamespace() const { return GetAttr().GetNamespace(); }

    /// \sa UsdAttribute::SplitName()
    std::vector<std::string> SplitName() const { return GetAttr().SplitName(); };

    /// \sa UsdAttribute::GetTypeName()
    SdfValueTypeName GetTypeName() const { return GetAttr().GetTypeName(); }

    /// Get the attribute value of the XformOp at \p time.
    /// 
    /// \note For inverted ops, this returns the raw, uninverted value. 
    ///
    template <typename T>
    bool Get(T* value, UsdTimeCode time = UsdTimeCode::Default()) const {
        return boost::apply_visitor(_Get<T>(value, time), _attr);
    }

    /// Set the attribute value of the XformOp at \p time 
    /// 
    /// \note This only works on non-inverse operations. If invoked on
    /// an inverse xform operation, a coding error is issued and no value is
    /// authored.
    /// 
    template <typename T>
    bool Set(T const & value, UsdTimeCode time = UsdTimeCode::Default()) const {
        // Issue a coding error and return without setting value, 
        // if this is an inverse op.
        if (_isInverseOp) {
            TF_CODING_ERROR("Cannot set a value on the inverse xformOp '%s'. "
                "Please set value on the paired non-inverse xformOp instead.",
                GetOpName().GetText());
            return false;
        }

        return GetAttr().Set(value, time);
    }

    /// Populates the list of time samples at which the associated attribute 
    /// is authored.
    bool GetTimeSamples(std::vector<double> *times) const {
        return boost::apply_visitor(_GetTimeSamples(times), _attr);
    }

    /// Returns the number of time samples authored for this xformOp.
    size_t GetNumTimeSamples() const {
        return boost::apply_visitor(_GetNumTimeSamples(), _attr);
    }

private:

    friend class UsdGeomXformable;
    
    // Returns the attribute belonging to \p prim that corresponds to the  
    // given \p opName. It also populates the output parameter \p isInverseOp 
    // appropriately. 
    // 
    // The attribute that's returned will be invalid if the 
    // corresponding xformOp attribute doesn't exist on the prim.
    // 
    static UsdAttribute _GetXformOpAttr(UsdPrim const& prim, 
        const TfToken &opName, bool *isInverseOp);

    // Private method for creating and using an attribute query interally for 
    // this xformOp.
    void _CreateAttributeQuery() const {
        _attr = UsdAttributeQuery(GetAttr());
    }

    // Factory for UsdGeomXformable's use, so that we can encapsulate the
    // logic of what discriminates XformOp in this class, while
    // preserving the pattern that attributes can only be created
    // via their container objects.
    //
    // \p opType must be one of UsdGeomXformOp::Type
    // 
    // \p precision must be one of UsdGeomXformOp::Precision.
    // 
    // \return an invalid UsdGeomXformOp if we failed to create a valid
    // attribute, a valid UsdGeomXformOp otherwise.  It is not an
    // error to create over an existing, compatible attribute.
    //
    // It is a failed verification for \p prim to be invalid/expired
    //
    // \sa UsdPrim::CreateAttribute()
    UsdGeomXformOp(UsdPrim const& prim, Type const opType,
                   Precision const precision, TfToken const &opSuffix=TfToken(),
                   bool inverse=false);

    // UsdAttributeQuery already contains a copy of the associated UsdAttribute.
    // To minimize the memory usage, we only store one or the other. 
    // 
    // The lifetime of a UsdAttributeQuery needs to be managed very carefully as
    // it gets invalidated whenever the associated attribute is authored.
    // Hence, access to the creation of an attribute query is restricted inside 
    // a private member function named _CreateAttributeQuery().
    // 
    mutable boost::variant<UsdAttribute, UsdAttributeQuery> _attr;

    Type _opType;
    bool _isInverseOp;

    // Visitor for getting xformOp value.
    template <class T>
    struct _Get : public boost::static_visitor<bool> 
    {
        _Get(T *value_, 
             UsdTimeCode time_ = UsdTimeCode::Default()) : value (value_), time(time_)
        {}

        bool operator()(const UsdAttribute &attr) const
        {
            return attr.Get(value, time);
        }

        bool operator()(const UsdAttributeQuery &attrQuery) const
        {
            return attrQuery.Get(value, time);
        }

        T *value;
        UsdTimeCode time;
    };

    // Visitor for getting a const-reference to the UsdAttribute.
    struct _GetAttr : public boost::static_visitor<const UsdAttribute &> {

        _GetAttr() {}

        const UsdAttribute &operator()(const UsdAttribute &attr) const
        {
            return attr;
        }

        const UsdAttribute &operator()(const UsdAttributeQuery &attrQuery) const
        {
            return attrQuery.GetAttribute();
        }
    };

    // Visitor for getting all the time samples.
    struct _GetTimeSamples : public boost::static_visitor<bool> {

        _GetTimeSamples(std::vector<double> *times_) : times(times_) {}

        bool operator()(const UsdAttribute &attr) const
        {
            return attr.GetTimeSamples(times);
        }

        bool operator()(const UsdAttributeQuery &attrQuery) const
        {
            return attrQuery.GetTimeSamples(times);
        }

        std::vector<double> *times;
    };

    // Visitor for getting the number of time samples.
    struct _GetNumTimeSamples : public boost::static_visitor<size_t> {

        _GetNumTimeSamples() {}

        size_t operator()(const UsdAttribute &attr) const
        {
            return attr.GetNumTimeSamples();
        }

        size_t operator()(const UsdAttributeQuery &attrQuery) const
        {
            return attrQuery.GetNumTimeSamples();
        }
    };

    // Visitor for determining whether the op might vary over time.
    struct _GetMightBeTimeVarying : public boost::static_visitor<bool> {

        _GetMightBeTimeVarying() {}

        bool operator()(const UsdAttribute &attr) const
        {
            return attr.ValueMightBeTimeVarying();
        }

        bool operator()(const UsdAttributeQuery &attrQuery) const
        {
            return attrQuery.ValueMightBeTimeVarying();
        }
    };

};


#endif // USD_XFORMOP_H
