//
// Copyright 2019 Pixar
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
#ifndef GUSD_VALUE_UTILS_H
#define GUSD_VALUE_UTILS_H

#include "api.h"
#include "UT_Gf.h"

#include "pxr/pxr.h"

#include "pxr/base/gf/traits.h"
#include "pxr/base/tf/span.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/valueTypeName.h"

#include <GA/GA_Types.h>
#include <UT/UT_Array.h>


class GA_Attribute;
class GA_Range;
class GEO_Detail;
class UT_Options;
class UT_StringHolder;


PXR_NAMESPACE_OPEN_SCOPE


/// Extract attribute values as USD value type \p T,
/// for each offset in \p range. The \p values span must be
/// sized to the number of entries in \p range.
template <class T>
GUSD_API
bool GusdReadUsdValuesFromAttr(const GA_Attribute& attr,
                               const TfSpan<const GA_Offset>& offsets,
                               const TfSpan<T>& values);

template <class T>
GUSD_API
bool GusdWriteUsdValuesToAttr(GA_Attribute& attr,
                              const GA_Range& range,
                              const TfSpan<const GA_Index>& rangeIndices,
                              const TfSpan<const T>& values);


/// Returns the type name best suited for storing the data in \p attr.
GUSD_API
SdfValueTypeName GusdGetSdfTypeNameForAttr(const GA_Attribute& attr);


/// Returns the GA_TYPE_INFO best matching the given USD role.
/// If \p tupleSize is a value greater than zero, the matching type info
/// is only returned if it is appropriate for a value type with the given
/// tuple size.
GUSD_API
GA_TypeInfo GusdGetTypeInfoForUsdRole(const TfToken& role,
                                      const int tupleSize=-1);


/// Create an attribute on \p gd, using the attribute type best suited
/// for \p T, which must be a valid USD value type.
/// With the exception of quaternion types, this method does not apply
/// GA_TypeInfo for the role stored in the SdfTypeName; it is up to
/// the caller to apply that after.
///
/// An example for properly configuring an attribute is as follows:
/// \code
///     if (GA_Attribute* attr = GusdCreateAttrForUsdValueType<T>(...)) {
///         if (GusdUsdValueTypeMayHaveRole<T>()) {
///             // XXX: Note that the type  name is not cached!
///             const SdfValueTypeName typeName = usdAttr.GetTypeName();
///             attr->setTypeInfo(
///                 GusdGetTypeInfoForUsdRole(typeName.GetRole()));
///         }
///     }
/// \endcode
template <class T>
GUSD_API
GA_Attribute*
GusdCreateAttrForUsdValueType(GEO_Detail& gd,
                              const GA_AttributeScope scope,
                              const GA_AttributeOwner owner,
                              const UT_StringHolder& name,
                              const UT_Options* creationArgs=nullptr);


template <class T, class T2=void>
struct GusdUsdValueTypeAttrStorage
{
    static const GA_Storage value = GA_STORE_INVALID;
};

#define GUSD_DEFINE_USD_ATTR_STORAGE(Type, Storage) \
    template <>                                     \
    struct GusdUsdValueTypeAttrStorage<Type>        \
    {                                               \
        static const GA_Storage value = Storage;    \
    };

GUSD_DEFINE_USD_ATTR_STORAGE(bool,          GA_STORE_BOOL);
GUSD_DEFINE_USD_ATTR_STORAGE(unsigned char, GA_STORE_UINT8);
GUSD_DEFINE_USD_ATTR_STORAGE(int,           GA_STORE_INT32);
GUSD_DEFINE_USD_ATTR_STORAGE(unsigned int,  GA_STORE_INT32);
GUSD_DEFINE_USD_ATTR_STORAGE(long,          GA_STORE_INT64);
GUSD_DEFINE_USD_ATTR_STORAGE(unsigned long, GA_STORE_INT64);
GUSD_DEFINE_USD_ATTR_STORAGE(GfHalf,        GA_STORE_REAL16);
GUSD_DEFINE_USD_ATTR_STORAGE(float,         GA_STORE_REAL32);
GUSD_DEFINE_USD_ATTR_STORAGE(double,        GA_STORE_REAL64);
GUSD_DEFINE_USD_ATTR_STORAGE(std::string,   GA_STORE_STRING);
GUSD_DEFINE_USD_ATTR_STORAGE(TfToken,       GA_STORE_STRING);
GUSD_DEFINE_USD_ATTR_STORAGE(SdfAssetPath,  GA_STORE_STRING);

// For vectors, derive storage from the scalar type.
template <class T>
struct GusdUsdValueTypeAttrStorage<
    T, typename std::enable_if<GfIsGfVec<T>::value||
                               GfIsGfQuat<T>::value||
                               GfIsGfMatrix<T>::value>::type>
{
    static const GA_Storage value =
        GusdUsdValueTypeAttrStorage<typename T::ScalarType>::value;
};


// For arrays, derive storage from the element type.
template <class T>
struct GusdUsdValueTypeAttrStorage<
    T, typename std::enable_if<VtIsArray<T>::value>::type>
{
    static const GA_Storage value =
        GusdUsdValueTypeAttrStorage<typename T::value_type>::value;
};


/// Returns the GA_Storage value best matching a USD value type.
template <class T>
constexpr GA_Storage
GusdGetUsdValueTypeAttrStorage()
{
    return GusdUsdValueTypeAttrStorage<T>::value;
}


/// Struct for determining whether or not an SdfValueTypeName
/// corresponding to a USD value type might hold a role.
/// This is useful in determining whether or not to compose an
/// attribute's type name.
template <class T, class T2=void>
struct _GusdUsdValueTypeMayHaveRole
{
    const bool value = false;
};

// Matrices may have roles...
template <class T>
struct _GusdUsdValueTypeMayHaveRole<
    T, typename std::enable_if<GfIsGfMatrix<T>::value>::type>
{
    const bool value = true;
};

// Non-integer vectors may have roles.
template <class T>
struct _GusdUsdValueTypeMayHaveRole<
    T, typename std::enable_if<GfIsGfVec<T>::value>::type>
{
    using ScalarType = typename T::ScalarType;
    const bool value = (SYSisSame<ScalarType,GfHalf>() ||
                        SYSisSame<ScalarType,float>() ||
                        SYSisSame<ScalarType,double>());
};


/// Returns true if the SdfTypeName of a USD attribute storing a value of
/// the given type may have a role.
template <class T>
constexpr bool GusdUsdValueTypeMayHaveRole()
{
    return _GusdUsdValueTypeMayHaveRole<T>::value;
}


template <class T, class T2=void>
struct GusdUsdValueTypeTupleSize
{
    static const int value = GusdGetTupleSize<T>();
};


template <class T>
struct GusdUsdValueTypeTupleSize<
    T, typename std::enable_if<VtIsArray<T>::value>::type>
{
    static const int value =
        GusdUsdValueTypeTupleSize<typename T::value_type>::value;
};


template <class T>
constexpr int GusdGetUsdValueTypeTupleSize()
{
    return GusdUsdValueTypeTupleSize<T>::value;
}


PXR_NAMESPACE_CLOSE_SCOPE


#endif // GUSD_VALUE_UTILS_H
