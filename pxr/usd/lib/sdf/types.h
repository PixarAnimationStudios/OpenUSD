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
#ifndef SDF_TYPES_H
#define SDF_TYPES_H

/// \file sdf/types.h
/// Basic Sdf data types

#include "pxr/pxr.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/valueTypeName.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/inttypes.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/quath.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2h.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4h.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/preprocessorUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/transform_view.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/noncopyable.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/list/fold_left.hpp>
#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/list/size.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/selection/max.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/static_assert.hpp>
#include <iosfwd>
#include <list>
#include <map>
#include <stdint.h>
#include <string>
#include <typeinfo>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;

/// An enum that specifies the type of an object. Objects
/// are entities that have fields and are addressable by path.
enum SdfSpecType {
    // The unknown type has a value of 0 so that SdfSpecType() is unknown.
    SdfSpecTypeUnknown = 0,

    // Real concrete types
    SdfSpecTypeAttribute,
    SdfSpecTypeConnection,
    SdfSpecTypeExpression,
    SdfSpecTypeMapper,
    SdfSpecTypeMapperArg,
    SdfSpecTypePrim,
    SdfSpecTypePseudoRoot,
    SdfSpecTypeRelationship,
    SdfSpecTypeRelationshipTarget,
    SdfSpecTypeVariant,
    SdfSpecTypeVariantSet,

    SdfNumSpecTypes
};

/// An enum that identifies the possible specifiers for an
/// SdfPrimSpec. The SdfSpecifier enum is registered as a TfEnum
/// for converting to and from <c>std::string</c>.
///
/// <b>SdfSpecifier:</b>
/// <ul>
/// <li><b>SdfSpecifierDef.</b> Defines a concrete prim.
/// <li><b>SdfSpecifierOver.</b> Overrides an existing prim.
/// <li><b>SdfSpecifierClass.</b> Defines an abstract prim.
/// <li><b>SdfNumSpecifiers.</b> The number of specifiers.
/// </ul>
///
enum SdfSpecifier {
    SdfSpecifierDef,
    SdfSpecifierOver,
    SdfSpecifierClass,
    SdfNumSpecifiers
};

/// Returns true if the specifier defines a prim.
inline
bool
SdfIsDefiningSpecifier(SdfSpecifier spec)
{
    return (spec != SdfSpecifierOver);
}

/// An enum that defines permission levels.
///
/// Permissions control which layers may refer to or express
/// opinions about a prim.  Opinions expressed about a prim, or
/// relationships to that prim, by layers that are not allowed
/// permission to access the prim will be ignored.
///
/// <b>SdfPermission:</b>
/// <ul>
/// <li><b>SdfPermissionPublic.</b> Public prims can be referred to by
///     anything. (Available to any client.)
/// <li><b>SdfPermissionPrivate.</b> Private prims can be referred to
///     only within the local layer stack, and not across references
///     or inherits. (Not available to clients.)
/// <li><b>SdfNumPermission.</b> Internal sentinel value.
/// </ul>
///
enum SdfPermission {
    SdfPermissionPublic,         
    SdfPermissionPrivate,        

    SdfNumPermissions            
};

/// An enum that identifies variability types for attributes.
/// Variability indicates whether the attribute may vary over time and
/// value coordinates, and if its value comes through authoring or
/// or from its owner.
///
/// <b>SdfVariability:</b>
/// <ul>
///     <li><b>SdfVariabilityVarying.</b> Varying attributes may be directly 
///            authored, animated and affected on by Actions.  They are the 
///            most flexible.
///     <li><b>SdfVariabilityUniform.</b> Uniform attributes may be authored 
///            only with non-animated values (default values).  They cannot 
///            be affected by Actions, but they can be connected to other 
///            Uniform attributes.
///     <li><b>SdfVariabilityConfig.</b> Config attributes are the same as 
///            Uniform except that a Prim can choose to alter its collection 
///            of built-in properties based on the values of its Config attributes.
///     <li><b>SdNumVariabilities.</b> Internal sentinel value.
/// </ul>
///
enum SdfVariability {
    SdfVariabilityVarying,
    SdfVariabilityUniform,
    SdfVariabilityConfig,

    SdfNumVariabilities 
};

// Each category of compatible units of measurement is defined by a
// preprocessor sequence of tuples.  Each such sequence gives rise to an enum
// representing the corresponding unit category.  All the unit categories are
// listed in _SDF_UNITS where each entry is a two-tuple with the unit category
// name as the first element, and the second element is the units in that
// category.  Each tuple in a unit category sequence corresponds to a unit of
// measurement represented by an enumerant whose name is given by concatenating
// 'Sdf', the unit category name, the word 'Unit' and the first entry in the
// tuple.  (E.g. units of category 'Length' are represented by an enum named
// SdfLengthUnit with enumerants SdfLengthUnitInch, SdfLengthUnitMeter and so
// forth.)  The second element in the tuple is the display name for the unit,
// and the third element is the relative size of the unit compared to the menv
// default unit for the unit category (which has a relative size of 1.0).
// Dimensionless quantities use a special 'Dimensionless' unit category
// represented by the enum SdfDimensionlessUnit.
#define _SDF_LENGTH_UNITS       \
((Millimeter, "mm",  0.001))    \
((Centimeter, "cm",  0.01))     \
((Decimeter,  "dm",  0.1))      \
((Meter,      "m",   1.0))      \
((Kilometer,  "km",  1000.0))   \
((Inch,       "in",  0.0254))   \
((Foot,       "ft",  0.3048))   \
((Yard,       "yd",  0.9144))   \
((Mile,       "mi",  1609.344))

#define _SDF_ANGULAR_UNITS      \
((Degrees, "deg", 1.0))         \
((Radians, "rad", 57.2957795130823208768))

#define _SDF_DIMENSIONLESS_UNITS \
((Percent, "%", 0.01))           \
((Default, "default", 1.0))

#define _SDF_UNITS                          \
((Length, _SDF_LENGTH_UNITS),               \
((Angular, _SDF_ANGULAR_UNITS),             \
((Dimensionless, _SDF_DIMENSIONLESS_UNITS), \
 BOOST_PP_NIL)))

#define _SDF_UNIT_TAG(tup) BOOST_PP_TUPLE_ELEM(3, 0, tup)
#define _SDF_UNIT_NAME(tup) BOOST_PP_TUPLE_ELEM(3, 1, tup)
#define _SDF_UNIT_SCALE(tup) BOOST_PP_TUPLE_ELEM(3, 2, tup)

#define _SDF_UNITSLIST_CATEGORY(tup) BOOST_PP_TUPLE_ELEM(2, 0, tup)
#define _SDF_UNITSLIST_TUPLES(tup) BOOST_PP_TUPLE_ELEM(2, 1, tup)
#define _SDF_UNITSLIST_ENUM(elem) BOOST_PP_CAT(BOOST_PP_CAT(Sdf, \
                                    _SDF_UNITSLIST_CATEGORY(elem)), Unit)

#define _SDF_DECLARE_UNIT_ENUMERANT(r, tag, elem) \
    BOOST_PP_CAT(Sdf ## tag ## Unit, _SDF_UNIT_TAG(elem)),

#define _SDF_DECLARE_UNIT_ENUM(r, unused, elem)          \
enum _SDF_UNITSLIST_ENUM(elem) {                         \
    BOOST_PP_SEQ_FOR_EACH(_SDF_DECLARE_UNIT_ENUMERANT,   \
                          _SDF_UNITSLIST_CATEGORY(elem), \
                          _SDF_UNITSLIST_TUPLES(elem))   \
};
BOOST_PP_LIST_FOR_EACH(_SDF_DECLARE_UNIT_ENUM, ~, _SDF_UNITS)

// Compute the max number of enumerants over all unit enums
#define _SDF_MAX_UNITS_OP(d, state, list) \
    BOOST_PP_MAX_D(d, state, BOOST_PP_SEQ_SIZE(_SDF_UNITSLIST_TUPLES(list)))
#define _SDF_UNIT_MAX_UNITS \
    BOOST_PP_LIST_FOLD_LEFT(_SDF_MAX_UNITS_OP, 0, _SDF_UNITS)

// Compute the number of unit enums
#define _SDF_UNIT_NUM_TYPES BOOST_PP_LIST_SIZE(_SDF_UNITS)

// Compute the number of bits needed to hold _SDF_UNIT_MAX_UNITS and
// _SDF_UNIT_NUM_TYPES.
#define _SDF_UNIT_MAX_UNITS_BITS TF_BITS_FOR_VALUES(_SDF_UNIT_MAX_UNITS)
#define _SDF_UNIT_TYPES_BITS     TF_BITS_FOR_VALUES(_SDF_UNIT_NUM_TYPES)

/// A map of mapper parameter names to parameter values.
typedef std::map<std::string, VtValue> SdfMapperParametersMap;

/// A map of reference variant set names to variants in those sets.
typedef std::map<std::string, std::string> SdfVariantSelectionMap;

/// A map of variant set names to list of variants in those sets.
typedef std::map<std::string, std::vector<std::string> > SdfVariantsMap;

/// A map of source SdfPaths to target SdfPaths for relocation.
//  Note: This map needs to be lexicographically sorted for Csd composition
//        implementation, so SdfPath::FastLessThan is explicitly omitted as
//        the Compare template parameter.
typedef std::map<SdfPath, SdfPath> SdfRelocatesMap;

/// A map from sample times to sample values.
typedef std::map<double, VtValue> SdfTimeSampleMap;

/// Gets the show default unit for the given /a typeName.
TfEnum SdfDefaultUnit( TfToken const &typeName );

/// Gets the show default unit for the given /a unit.
const TfEnum &SdfDefaultUnit( const TfEnum &unit );

/// Gets the unit category for a given /a unit.
const std::string &SdfUnitCategory( const TfEnum &unit );

/// Gets the type/unit pair for a unit enum.
std::pair<uint32_t, uint32_t> Sdf_GetUnitIndices( const TfEnum &unit );

/// Converts from one unit of measure to another. The \a fromUnit and \a toUnit
/// units must be of the same type (for example, both of type SdfLengthUnit).
double SdfConvertUnit( const TfEnum &fromUnit, const TfEnum &toUnit );

/// Gets the name for a given /a unit.
const std::string &SdfGetNameForUnit( const TfEnum &unit );

/// Gets a unit for the given /a name
const TfEnum &SdfGetUnitFromName( const std::string &name );

/// Converts a string to a bool.
/// Accepts case insensitive "yes", "no", "false", true", "0", "1".
/// Defaults to "true" if the string is not recognized.
///
/// If parseOK is supplied, the pointed-to bool will be set to indicate
/// whether the parse was succesful.
bool SdfBoolFromString( const std::string &, bool *parseOk = NULL );

/// Given a value, returns if there is a valid corresponding valueType.
bool SdfValueHasValidType(VtValue const& value);

/// Given an sdf valueType name, produce TfType if the type name specifies a
/// valid sdf value type.
TfType SdfGetTypeForValueTypeName(TfToken const &name);

/// Given a value, produce the sdf valueType name.  If you provide a value that
/// does not return true for SdfValueHasValidType, the return value is
/// unspecified.
SdfValueTypeName SdfGetValueTypeNameForValue(VtValue const &value);

/// Return role name for \p typeName.  Return empty token if \p typeName has no
/// associated role name.
TfToken SdfGetRoleNameForValueTypeName(TfToken const &typeName);

// Sdf allows a specific set of types for attribute and metadata values.
// These types and some additional metadata are listed in the preprocessor
// sequence of tuples below. First element is a tag name that is appended to
// 'SdfValueType' to produce the C++ traits type for the value type.
// Second element is the value type name, third element is the corresponding
// C++ type, and the fourth element is the tuple of tuple dimensions.
//
// Libraries may extend this list and define additional value types.
// When doing so, the type must be declared using the SDF_DECLARE_VALUE_TYPE
// macro below. The type must also be registered in the associated schema using
// SdfSchema::_RegisterValueType(s).
#define _SDF_SCALAR_VALUE_TYPES                     \
((Bool,       bool,       bool,           ()     )) \
((UChar,      uchar,      unsigned char,  ()     )) \
((Int,        int,        int,            ()     )) \
((UInt,       uint,       unsigned int,   ()     )) \
((Int64,      int64,      int64_t,        ()     )) \
((UInt64,     uint64,     uint64_t,       ()     )) \
((Half,       half,       half,           ()     )) \
((Float,      float,      float,          ()     )) \
((Double,     double,     double,         ()     )) \
((String,     string,     std::string,    ()     )) \
((Token,      token,      TfToken,        ()     )) \
((AssetPath,  asset,      SdfAssetPath,   ()     ))

#define _SDF_DIMENSIONED_VALUE_TYPES                \
((Matrix2d,   Matrix2d,   GfMatrix2d,     (2, 2) )) \
((Matrix3d,   Matrix3d,   GfMatrix3d,     (3, 3) )) \
((Matrix4d,   Matrix4d,   GfMatrix4d,     (4, 4) )) \
((Quatd,      Quatd,      GfQuatd,        (4)    )) \
((Quatf,      Quatf,      GfQuatf,        (4)    )) \
((Quath,      Quath,      GfQuath,        (4)    )) \
((Vec2d,      Vec2d,      GfVec2d,        (2)    )) \
((Vec2f,      Vec2f,      GfVec2f,        (2)    )) \
((Vec2h,      Vec2h,      GfVec2h,        (2)    )) \
((Vec2i,      Vec2i,      GfVec2i,        (2)    )) \
((Vec3d,      Vec3d,      GfVec3d,        (3)    )) \
((Vec3f,      Vec3f,      GfVec3f,        (3)    )) \
((Vec3h,      Vec3h,      GfVec3h,        (3)    )) \
((Vec3i,      Vec3i,      GfVec3i,        (3)    )) \
((Vec4d,      Vec4d,      GfVec4d,        (4)    )) \
((Vec4f,      Vec4f,      GfVec4f,        (4)    )) \
((Vec4h,      Vec4h,      GfVec4h,        (4)    )) \
((Vec4i,      Vec4i,      GfVec4i,        (4)    ))

#define SDF_VALUE_TYPES _SDF_SCALAR_VALUE_TYPES _SDF_DIMENSIONED_VALUE_TYPES

// Accessors for individual elements in the value types tuples.
#define SDF_VALUE_TAG(tup) BOOST_PP_TUPLE_ELEM(4, 0, tup)
#define SDF_VALUE_TYPENAME(tup) BOOST_PP_TUPLE_ELEM(4, 1, tup)
#define SDF_VALUE_CPP_TYPE(tup) BOOST_PP_TUPLE_ELEM(4, 2, tup)
#define SDF_VALUE_TUPLE_DIM(tup) \
    TF_PP_TUPLE_TO_LIST(BOOST_PP_TUPLE_ELEM(4, 3, tup))
#define SDF_VALUE_TRAITS_TYPE(tup) \
    BOOST_PP_CAT(SdfValueType, SDF_VALUE_TAG(tup))

template <class T>
struct SdfValueTypeTraits {
    static const bool IsValueType = false;
};

#define _SDF_ADD_DIMENSION(r, unused, elem) \
        dimensions.d[dimensions.size++] = elem;

#define SDF_DECLARE_VALUE_TYPE(r, unused, elem)                             \
struct SDF_VALUE_TRAITS_TYPE(elem) {                                        \
    typedef SDF_VALUE_CPP_TYPE(elem) Type;                                  \
    typedef VtArray< SDF_VALUE_CPP_TYPE(elem) > ShapedType;                 \
    static std::string Name() {                                             \
        return BOOST_PP_STRINGIZE(SDF_VALUE_TYPENAME(elem));                \
    }                                                                       \
    static std::string ShapedName() {                                       \
        return Name() + std::string("[]");                                  \
    }                                                                       \
    static SdfTupleDimensions Dimensions() {                                \
        SdfTupleDimensions dimensions;                                      \
        BOOST_STATIC_ASSERT(                                                \
            BOOST_PP_LIST_SIZE(SDF_VALUE_TUPLE_DIM(elem)) <= 2);            \
        BOOST_PP_LIST_FOR_EACH(                                             \
            _SDF_ADD_DIMENSION, ~, SDF_VALUE_TUPLE_DIM(elem));              \
        return dimensions;                                                  \
    }                                                                       \
};                                                                          \
                                                                            \
template <>                                                                 \
struct SdfValueTypeTraits<SDF_VALUE_CPP_TYPE(elem)>                         \
    : public SDF_VALUE_TRAITS_TYPE(elem) {                                  \
    static const bool IsValueType = true;                                   \
};                                                                          \
template <>                                                                 \
struct SdfValueTypeTraits<VtArray<SDF_VALUE_CPP_TYPE(elem)> >               \
    : public SDF_VALUE_TRAITS_TYPE(elem) {                                  \
    static const bool IsValueType = true;                                   \
};                                                                          \

BOOST_PP_SEQ_FOR_EACH(SDF_DECLARE_VALUE_TYPE, ~, SDF_VALUE_TYPES);
#undef _SDF_ADD_DIMENSION

// Allow character arrays to be treated as Sdf value types.
// Sdf converts character arrays to strings for scene description.
template <int N>
struct SdfValueTypeTraits<char[N]> 
    : public SdfValueTypeString {
    static const bool IsValueType = true;
};

#define _SDF_MAKE_SCALAR_TYPENAME_TOKEN(r, unused, elem)                \
    (SDF_VALUE_TAG(elem), BOOST_PP_STRINGIZE(SDF_VALUE_TYPENAME(elem)))

#define _SDF_MAKE_SHAPED_TYPENAME_TOKEN(r, unused, elem)                       \
    (BOOST_PP_CAT(SDF_VALUE_TAG(elem), Array),                                 \
     BOOST_PP_STRINGIZE(SDF_VALUE_TYPENAME(elem)) "[]")

#define _SDF_VALUE_TYPE_NAME_TOKENS                                            \
    (Point) ((PointArray, "Point[]"))                                          \
    (PointFloat) ((PointFloatArray, "PointFloat[]"))                           \
    (Normal) ((NormalArray, "Normal[]"))                                       \
    (NormalFloat) ((NormalFloatArray, "NormalFloat[]"))                        \
    (Vector) ((VectorArray, "Vector[]"))                                       \
    (VectorFloat) ((VectorFloatArray, "VectorFloat[]"))                        \
    (Color) ((ColorArray, "Color[]"))                                          \
    (ColorFloat) ((ColorFloatArray, "ColorFloat[]"))                           \
    (Frame) ((FrameArray, "Frame[]"))                                          \
    (Transform) ((TransformArray, "Transform[]"))                              \
    (PointIndex) ((PointIndexArray, "PointIndex[]"))                           \
    (EdgeIndex) ((EdgeIndexArray, "EdgeIndex[]"))                              \
    (FaceIndex) ((FaceIndexArray, "FaceIndex[]"))                              \
    (Schema) ((SchemaArray, "Schema[]"))

#define SDF_VALUE_TYPE_NAME_TOKENS                                             \
    _SDF_VALUE_TYPE_NAME_TOKENS                                                \
    BOOST_PP_EXPAND(BOOST_PP_SEQ_TRANSFORM(                                    \
                        _SDF_MAKE_SCALAR_TYPENAME_TOKEN, ~, SDF_VALUE_TYPES))  \
    BOOST_PP_EXPAND(BOOST_PP_SEQ_TRANSFORM(                                    \
                        _SDF_MAKE_SHAPED_TYPENAME_TOKEN, ~, SDF_VALUE_TYPES))

#define SDF_VALUE_ROLE_NAME_TOKENS              \
    (Point)                                     \
    (Normal)                                    \
    (Vector)                                    \
    (Color)                                     \
    (Frame)                                     \
    (Transform)                                 \
    (PointIndex)                                \
    (EdgeIndex)                                 \
    (FaceIndex)                                 \
    (Schema)

TF_DECLARE_PUBLIC_TOKENS(SdfValueRoleNames, SDF_VALUE_ROLE_NAME_TOKENS);

#define _SDF_WRITE_VALUE_TRAITS_TYPE(r, unused, elem) \
BOOST_PP_COMMA() SDF_VALUE_TRAITS_TYPE(elem)

/// An mpl sequence of all available traits types for value types
/// defined by Sdf.
typedef ::boost::mpl::vector<
    SDF_VALUE_TRAITS_TYPE(BOOST_PP_SEQ_HEAD(_SDF_SCALAR_VALUE_TYPES))
    BOOST_PP_SEQ_FOR_EACH(_SDF_WRITE_VALUE_TRAITS_TYPE, ~,
                          BOOST_PP_SEQ_TAIL(_SDF_SCALAR_VALUE_TYPES))
    > Sdf_ScalarValueTraitsTypesVector;
typedef ::boost::mpl::vector<
    SDF_VALUE_TRAITS_TYPE(BOOST_PP_SEQ_HEAD(_SDF_DIMENSIONED_VALUE_TYPES))
    BOOST_PP_SEQ_FOR_EACH(_SDF_WRITE_VALUE_TRAITS_TYPE, ~,
                          BOOST_PP_SEQ_TAIL(_SDF_DIMENSIONED_VALUE_TYPES))
    > Sdf_DimensionedValueTraitsTypesVector;

typedef ::boost::mpl::joint_view<
    Sdf_ScalarValueTraitsTypesVector, Sdf_DimensionedValueTraitsTypesVector
    > SdfValueTraitsTypesVector;

#undef _SDF_WRITE_VALUE_TRAITS_TYPE

/// A metafunction that returns the underlying C++ type given a traits type.
struct SdfGetCppType {
    template <class TraitsType>
    struct apply { typedef typename TraitsType::Type type; };
};

/// A metafunction that returns the underlying shaped C++ type given a traits
/// type.
struct SdfGetShapedCppType {
    template <class TraitsType>
    struct apply { typedef typename TraitsType::ShapedType type; };
};


/// A sequence of the C++ types associated with the value types defined
/// by Sdf.
typedef boost::mpl::transform_view<
    SdfValueTraitsTypesVector, SdfGetCppType
    > SdfValueCppTypesVector;

/// A sequence of the shaped C++ types associated with the value types.
typedef boost::mpl::transform_view<
    SdfValueTraitsTypesVector, SdfGetShapedCppType
    > SdfValueShapedCppTypesVector;

/// A sequence of all the underlying C++ types associated with the value
/// types.
typedef boost::mpl::joint_view<
    SdfValueCppTypesVector, SdfValueShapedCppTypesVector
    > SdfValueAllCppTypesVector;

SDF_DECLARE_HANDLES(SdfLayer);

SDF_DECLARE_HANDLES(SdfAttributeSpec);
SDF_DECLARE_HANDLES(SdfMapperArgSpec);
SDF_DECLARE_HANDLES(SdfMapperSpec);
SDF_DECLARE_HANDLES(SdfPrimSpec);
SDF_DECLARE_HANDLES(SdfPropertySpec);
SDF_DECLARE_HANDLES(SdfSpec);
SDF_DECLARE_HANDLES(SdfRelationshipSpec);
SDF_DECLARE_HANDLES(SdfVariantSetSpec);
SDF_DECLARE_HANDLES(SdfVariantSpec);

typedef std::map<std::string, SdfVariantSetSpecHandle> 
    SdfVariantSetSpecHandleMap;

/// Writes the string representation of \c SdfSpecifier to \a out.
std::ostream & operator<<( std::ostream &out, const SdfSpecifier &spec );

/// Writes the string representation of \c SdfRelocatesMap to \a out.
std::ostream & operator<<( std::ostream &out,
                           const SdfRelocatesMap &reloMap );

/// Writes the string representation of \c SdfTimeSampleMap to \a out.
std::ostream & operator<<( std::ostream &out,
                           const SdfTimeSampleMap &sampleMap );

std::ostream &VtStreamOut(const SdfVariantSelectionMap &, std::ostream &);

/// \class SdfUnregisteredValue
/// Stores a representation of the value for an unregistered metadata
/// field encountered during text layer parsing. 
/// 
/// This provides the ability to serialize this data to a layer, as
/// well as limited inspection and editing capabilities (e.g., moving
/// this data to a different spec or field) even when the data type
/// of the value isn't known.
class SdfUnregisteredValue : 
    public boost::equality_comparable<SdfUnregisteredValue>
{
public:
    /// Wraps an empty VtValue
    SdfUnregisteredValue();

    /// Wraps a std::string
    explicit SdfUnregisteredValue(const std::string &value);

    /// Wraps a VtDictionary
    explicit SdfUnregisteredValue(const VtDictionary &value);

    /// Wraps a SdfUnregisteredValueListOp
    explicit SdfUnregisteredValue(const SdfUnregisteredValueListOp &value);

    /// Returns the wrapped VtValue specified in the constructor
    const VtValue& GetValue() const {
        return _value;
    }

    /// Hash.
    friend size_t hash_value(const SdfUnregisteredValue &uv) {
        return uv._value.GetHash();
    }

    /// Returns true if the wrapped VtValues are equal
    bool operator==(const SdfUnregisteredValue &other) const;

private:
    VtValue _value;
};

/// Writes the string representation of \c SdfUnregisteredValue to \a out.
std::ostream &operator << (std::ostream &out, const SdfUnregisteredValue &value);

class Sdf_ValueTypeNamesType : boost::noncopyable {
public:
    SdfValueTypeName Bool;
    SdfValueTypeName UChar, Int, UInt, Int64, UInt64;
    SdfValueTypeName Half, Float, Double;
    SdfValueTypeName String, Token, Asset;
    SdfValueTypeName Int2,     Int3,     Int4;
    SdfValueTypeName Half2,    Half3,    Half4;
    SdfValueTypeName Float2,   Float3,   Float4;
    SdfValueTypeName Double2,  Double3,  Double4;
    SdfValueTypeName Point3h,  Point3f,  Point3d;
    SdfValueTypeName Vector3h, Vector3f, Vector3d;
    SdfValueTypeName Normal3h, Normal3f, Normal3d;
    SdfValueTypeName Color3h,  Color3f,  Color3d;
    SdfValueTypeName Color4h,  Color4f,  Color4d;
    SdfValueTypeName Quath,    Quatf,    Quatd;
    SdfValueTypeName Matrix2d, Matrix3d, Matrix4d;
    SdfValueTypeName Frame4d;

    SdfValueTypeName BoolArray;
    SdfValueTypeName UCharArray, IntArray, UIntArray, Int64Array, UInt64Array;
    SdfValueTypeName HalfArray, FloatArray, DoubleArray;
    SdfValueTypeName StringArray, TokenArray, AssetArray;
    SdfValueTypeName Int2Array,     Int3Array,     Int4Array;
    SdfValueTypeName Half2Array,    Half3Array,    Half4Array;
    SdfValueTypeName Float2Array,   Float3Array,   Float4Array;
    SdfValueTypeName Double2Array,  Double3Array,  Double4Array;
    SdfValueTypeName Point3hArray,  Point3fArray,  Point3dArray;
    SdfValueTypeName Vector3hArray, Vector3fArray, Vector3dArray;
    SdfValueTypeName Normal3hArray, Normal3fArray, Normal3dArray;
    SdfValueTypeName Color3hArray,  Color3fArray,  Color3dArray;
    SdfValueTypeName Color4hArray,  Color4fArray,  Color4dArray;
    SdfValueTypeName QuathArray,    QuatfArray,    QuatdArray;
    SdfValueTypeName Matrix2dArray, Matrix3dArray, Matrix4dArray;
    SdfValueTypeName Frame4dArray;

    ~Sdf_ValueTypeNamesType();
    struct _Init {
        static const Sdf_ValueTypeNamesType* New();
    };

    // For Pixar internal backwards compatibility.
    TfToken GetSerializationName(const SdfValueTypeName&) const;
    TfToken GetSerializationName(const VtValue&) const;
    TfToken GetSerializationName(const TfToken&) const;

private:
    friend class SdfSchema;
    Sdf_ValueTypeNamesType();
};

extern TfStaticData<const Sdf_ValueTypeNamesType,
                    Sdf_ValueTypeNamesType::_Init> SdfValueTypeNames;

/// \class SdfValueBlock
/// A special value type that can be used to explicitly author an
/// opinion for an attribute's default value or time sample value
/// that represents having no value. Note that this is different
/// from not having a value authored.
///
/// One could author such a value in two ways.
/// 
/// \code
/// attribute->SetDefaultValue(VtValue(SdfValueBlock());
/// ...
/// layer->SetTimeSample(attribute->GetPath(), 101, VtValue(SdfValueBlock()));
/// \endcode
///
struct SdfValueBlock { 
    bool operator==(const SdfValueBlock& block) const { return true; }
    bool operator!=(const SdfValueBlock& block) const { return false; }

private:
    friend inline size_t hash_value(const SdfValueBlock &block) { return 0; }
};

// Write out the string representation of a block.
std::ostream& operator<<(std::ostream&, SdfValueBlock const&); 

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_TYPES_H
