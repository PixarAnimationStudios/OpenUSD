//
// Copyright 2017 Pixar
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
#ifndef __GUSD_GT_UTILS_H__
#define __GUSD_GT_UTILS_H__

#include "gusd/api.h"

#include <GT/GT_Primitive.h>
#include <GEO/GEO_Primitive.h>
#include <UT/UT_Map.h>
#include <UT/UT_Options.h>
#include <UT/UT_Set.h>
#include <UT/UT_Variadic.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/timeCode.h>

#include <boost/function.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class GfMatrix4d;
class GusdContext;
class SdfPath;
class TfToken;
class UsdAttribute;
class UsdGeomImageable;
class UsdGeomBoundable;
class UsdGeomXformable;

//------------------------------------------------------------------------------
// class GusdGT_AttrFilter
//------------------------------------------------------------------------------
class GusdGT_AttrFilter
{
public:
    typedef UT_VariadicT<GT_Owner> OwnerArgs;

public:

    GUSD_API
    explicit GusdGT_AttrFilter(const std::string& pattern="*");

    GUSD_API
    GusdGT_AttrFilter( const GusdGT_AttrFilter& rhs );

    GUSD_API
    void setPattern(GT_Owner owner, const std::string& pattern);

    GUSD_API
    void appendPattern(GT_Owner owner, const std::string& pattern);
    
    GUSD_API
    bool matches(const std::string& str) const;

    GUSD_API
    void setActiveOwners(const OwnerArgs& owners) const;

private:
    UT_Map<GT_Owner, std::string> m_patterns;

    std::string m_overridePattern;

    mutable OwnerArgs m_activeOwners;
};

//------------------------------------------------------------------------------
// class GusdGT_Utils
//------------------------------------------------------------------------------
class GusdGT_Utils
{
public:
    enum TransformLevel {
        TransformLevelObject,
        TransformLevelIntrinsic,

        TransformLevelCount
    };

public:

    static bool setUsdAttribute(UsdAttribute& destAttr,
                                const GT_DataArrayHandle& sourceAttr,
                                UsdTimeCode time=UsdTimeCode::Default());


    static GT_DataArrayHandle getExtentsArray(const GT_PrimitiveHandle& gtPrim);

    static void setPrimvarSample( UsdGeomImageable& usdPrim, 
                                  const TfToken &name, 
                                  const GT_DataArrayHandle& data, 
                                  const TfToken& interpolation,
                                  UsdTimeCode time );

    static bool isDataConstant( const GT_DataArrayHandle& data );

    static void setCustomAttributesFromGTPrim(
                       UsdGeomImageable &usdGeomPrim,
                       const GT_AttributeListHandle& gtAttrs,
                       std::set<std::string>& excludeSet,
                       UsdTimeCode time=UsdTimeCode::Default());


    // TODO remove
    static bool setTransformFromGTArray(UsdGeomXformable& usdGeom,
                                        const GT_DataArrayHandle& xform,
                                        const TransformLevel transformLevel,
                                        UsdTimeCode time=UsdTimeCode::Default());

    static GT_DataArrayHandle
    getTransformArray(const GT_PrimitiveHandle& gtPrim);

    static GT_DataArrayHandle
    getPackedTransformArray(const GT_PrimitiveHandle& gtPrim);

    static GfMatrix4d getMatrixFromGTArray(const GT_DataArrayHandle& xform);

    static GT_DataArrayHandle 
    transformPoints( 
        GT_DataArrayHandle pts, 
        const GfMatrix4d& objXform );

    static GT_DataArrayHandle 
    transformPoints( 
        GT_DataArrayHandle pts, 
        const UT_Matrix4D& objXform );

    static GT_AttributeListHandle
    getAttributesFromPrim( const GEO_Primitive *prim );

    static std::string
    makeValidIdentifier(const TfToken& usdFilePath, const SdfPath& nodePath);


    /** Struct for querying storage by POD type.
        XXX: replace this with a constexpr in C++11 */
    template <typename T>
    struct StorageByType;
}; 
//------------------------------------------------------------------------------


template <>
struct GusdGT_Utils::StorageByType<uint8>
{ static const GT_Storage value = GT_STORE_UINT8; };

template <>
struct GusdGT_Utils::StorageByType<int32>
{ static const GT_Storage value = GT_STORE_INT32; };

template <>
struct GusdGT_Utils::StorageByType<int64>
{ static const GT_Storage value = GT_STORE_INT64; };

template <>
struct GusdGT_Utils::StorageByType<fpreal16>
{ static const GT_Storage value = GT_STORE_REAL16; };

template <>
struct GusdGT_Utils::StorageByType<fpreal32>
{ static const GT_Storage value = GT_STORE_REAL32; };

template <>
struct GusdGT_Utils::StorageByType<fpreal64>
{ static const GT_Storage value = GT_STORE_REAL64; };

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_GT_UTILS_H__

