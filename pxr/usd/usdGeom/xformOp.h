//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_GEOM_XFORM_OP_H
#define PXR_USD_USD_GEOM_XFORM_OP_H

/// \file usdGeom/xformOp.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <string>
#include <variant>
#include <vector>
#include <typeinfo>

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \hideinitializer
#define USDGEOM_XFORM_OP_TYPES \
    (translateX) \
    (translateY) \
    (translateZ) \
    (translate) \
    (scaleX) \
    (scaleY) \
    (scaleZ) \
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
/// \li <b>translateX</b> - translation along the X axis
/// \li <b>translateY</b> - translation along the Y axis
/// \li <b>translateZ</b> - translation along the Z axis
/// \li <b>translate</b> - XYZ translation
/// \li <b>scaleX</b> - scale along the X axis
/// \li <b>scaleY</b> - scale along the Y axis
/// \li <b>scaleZ</b> - scale along the Z axis
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
TF_DECLARE_PUBLIC_TOKENS(UsdGeomXformOpTypes, USDGEOM_API, USDGEOM_XFORM_OP_TYPES);

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
/// are generally imposed by higher level xform API schemas.
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
        TypeTranslateX,///< Translation along the X-axis.
        TypeTranslateY,///< Translation along the Y-axis.
        TypeTranslateZ,///< Translation along the Z-axis.
        TypeTranslate, ///< XYZ translation.
        TypeScaleX,    ///< Scale along the X-axis.
        TypeScaleY,    ///< Scale along the Y-axis.
        TypeScaleZ,    ///< Scale along the Z-axis.
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

    /// Precision of the encoded transformation operation's value.
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
    /// explicit-bool conversion operator will return false).
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
    USDGEOM_API
    explicit UsdGeomXformOp(const UsdAttribute &attr, bool isInverseOp=false);

    // -------------------------------------------------------
    /// \name Static Helper API
    // -------------------------------------------------------

    /// Test whether a given UsdAttribute represents valid XformOp, which
    /// implies that creating a UsdGeomXformOp from the attribute will succeed.
    ///
    /// Success implies that \c attr.IsDefined() is true.
    USDGEOM_API
    static bool IsXformOp(const UsdAttribute &attr);

    /// Test whether a given attribute name represents a valid XformOp, which
    /// implies that creating a UsdGeomXformOp from the corresponding 
    /// UsdAttribute will succeed.
    ///
    USDGEOM_API
    static bool IsXformOp(const TfToken &attrName);

    /// Returns the TfToken used to encode the given \p opType.
    /// Note that an empty TfToken is used to represent TypeInvalid
    USDGEOM_API
    static TfToken const &GetOpTypeToken(Type const opType);

    /// Returns the Type enum associated with the given \p opTypeToken.
    USDGEOM_API
    static Type GetOpTypeEnum(TfToken const &opTypeToken);

    /// Returns the precision corresponding to the given value typeName.
    USDGEOM_API
    static Precision GetPrecisionFromValueTypeName(
        const SdfValueTypeName& typeName);

    /// Returns the value typeName token that corresponds to the given 
    /// combination of \p opType and \p precision.
    USDGEOM_API
    static const SdfValueTypeName &GetValueTypeName(const Type opType,
                                                    const Precision precision);

    /// Returns the xformOp's name as it appears in xformOpOrder, given 
    /// the opType, the (optional) suffix and whether it is an inverse 
    /// operation.
    USDGEOM_API
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
    USDGEOM_API
    Precision GetPrecision() const;

    /// Returns whether the xformOp represents an inverse operation.
    bool IsInverseOp() const { 
        return _isInverseOp;
    }

    /// Returns the opName as it appears in the xformOpOrder attribute.
    /// 
    /// This will begin with "!invert!:xformOp:" if it is an inverse xform 
    /// operation. If it is not an inverse xformOp, it will begin with 
    /// 'xformOp:'.
    /// 
    /// This will be empty for an invalid xformOp.
    /// 
    USDGEOM_API
    TfToken GetOpName() const;

    /// Does this op have the given suffix in its name.
    USDGEOM_API
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
        if (!Get(&v, time)) {
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
    /// If \p isInverseOp is true, then the inverse of the transformation 
    /// represented by the op/value pair is returned.
    /// 
    /// An error will be issued if \p opType is not one of the values in the enum
    /// \ref UsdGeomXformOp::Type or if \p opVal cannot be converted
    /// to a suitable input to \p opType
    USDGEOM_API
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
    USDGEOM_API
    GfMatrix4d GetOpTransform(UsdTimeCode time) const;

    /// Determine whether there is any possibility that this op's value
    /// may vary over time.
    ///
    /// The determination is based on a snapshot of the authored state of the
    /// op, and may become invalid in the face of further authoring.
    bool MightBeTimeVarying() const {
        return std::visit(_GetMightBeTimeVarying(), _attr);
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
        return std::visit(_GetAttr(), _attr);
    }
    
    /// Return true if the wrapped UsdAttribute::IsDefined(), and in
    /// addition the attribute is identified as a XformOp.
    bool IsDefined() const { return IsXformOp(GetAttr()); }

public:
    /// \anchor UsdGeomXformOp_explicit_bool
    /// Explicit bool conversion operator. An XformOp object converts to 
    /// \c true iff it is valid for querying and authoring values and metadata, 
    /// (which is identically equivalent to IsDefined()), and converts to 
    /// \c false otherwise.
    explicit operator bool() const {
        return IsDefined();
    }

    /// Equality comparison.  Return true if \a lhs and \a rhs represent the
    /// same underlying UsdAttribute, false otherwise.
    friend bool operator==(const UsdGeomXformOp &lhs,
                           const UsdGeomXformOp &rhs) {
        return lhs.GetAttr() == rhs.GetAttr();
    }

    /// Inequality comparison. Return false if \a lhs and \a rhs represent the
    /// same underlying UsdAttribute, true otherwise.
    friend bool operator!=(const UsdGeomXformOp &lhs,
                           const UsdGeomXformOp &rhs) {
        return !(lhs == rhs);
    }

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
        return std::visit(_Get<T>(value, time), _attr);
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
        return std::visit(_GetTimeSamples(times), _attr);
    }

    /// Populates the list of time samples within the given \p interval, 
    /// at which the associated attribute is authored.
    bool GetTimeSamplesInInterval(const GfInterval &interval, 
                                  std::vector<double> *times) const {
        return std::visit(
                _GetTimeSamplesInInterval(interval, times), _attr);
    }

    /// Returns the number of time samples authored for this xformOp.
    size_t GetNumTimeSamples() const {
        return std::visit(_GetNumTimeSamples(), _attr);
    }

private:
    struct _ValidAttributeTagType {};

public:
    // Allow clients that guarantee \p attr is valid avoid having
    // UsdGeomXformOp's ctor check again.
    USDGEOM_API
    UsdGeomXformOp(const UsdAttribute &attr, bool isInverseOp,
                   _ValidAttributeTagType);
    USDGEOM_API
    UsdGeomXformOp(UsdAttributeQuery &&query, bool isInverseOp,
                   _ValidAttributeTagType);
private:
    friend class UsdGeomXformable;

    // Shared initialization function.
    void _Init();

    // Return the op-type for the string value \p str.
    static Type _GetOpTypeEnumFromCString(char const *str, size_t len);

    // Returns the attribute belonging to \p prim that corresponds to the  
    // given \p opName. It also populates the output parameter \p isInverseOp 
    // appropriately. 
    // 
    // The attribute that's returned will be invalid if the 
    // corresponding xformOp attribute doesn't exist on the prim.
    // 
    static UsdAttribute _GetXformOpAttr(UsdPrim const& prim, 
        const TfToken &opName, bool *isInverseOp);

    // Private method for creating and using an attribute query internally for 
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
    mutable std::variant<UsdAttribute, UsdAttributeQuery> _attr;

    Type _opType;
    bool _isInverseOp;

    // Visitor for getting xformOp value.
    template <class T>
    struct _Get
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
    struct _GetAttr {

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
    struct _GetTimeSamples {

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

    // Visitor for getting all the time samples within a given interval.
    struct _GetTimeSamplesInInterval {

        _GetTimeSamplesInInterval(const GfInterval &interval_,
                                  std::vector<double> *times_) 
            : interval(interval_), times(times_) 
        {}

        bool operator()(const UsdAttribute &attr) const
        {
            return attr.GetTimeSamplesInInterval(interval, times);
        }

        bool operator()(const UsdAttributeQuery &attrQuery) const
        {
            return attrQuery.GetTimeSamplesInInterval(interval, times);
        }

        const GfInterval &interval;
        std::vector<double> *times;
    };

    // Visitor for getting the number of time samples.
    struct _GetNumTimeSamples {

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
    struct _GetMightBeTimeVarying {

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



PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_XFORMOP_H
