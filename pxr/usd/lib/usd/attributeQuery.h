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
#ifndef USD_ATTRIBUTE_QUERY_H
#define USD_ATTRIBUTE_QUERY_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/resolveInfo.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/base/tf/token.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdAttributeQuery
///
/// Object for efficiently making repeated queries for attribute values.
///
/// Retrieving an attribute's value at a particular time requires determining
/// the source of strongest opinion for that value.  Often (i.e.  unless the
/// attribute is affected by \ref Usd_Page_ValueClips "Value Clips") this
/// source does not vary over time.  UsdAttributeQuery uses this fact to
/// speed up repeated value queries by caching the source information for an
/// attribute.  It is safe to use a UsdAttributeQuery for any attribute - if
/// the attribute \em is affected by Value Clips, the performance gain will
/// just be less.
///
/// \section Thread safety
/// This object provides the basic thread-safety guarantee.  Multiple threads
/// may call the value accessor functions simultaneously.
///
/// \section Invalidation
/// This object does not listen for change notification.  If a consumer is
/// holding on to a UsdAttributeQuery, it is their responsibility to dispose
/// of it in response to a resync change to the associated attribute. 
/// Failing to do so may result in incorrect values or crashes due to 
/// dereferencing invalid objects.
///
class UsdAttributeQuery
{
public:
    /// Construct an invalid query object.
    USD_API
    UsdAttributeQuery();

    /// Construct a new query for the attribute \p attr.
    USD_API
    explicit UsdAttributeQuery(const UsdAttribute& attr);

    /// Construct a new query for the attribute named \p attrName under
    /// the prim \p prim.
    USD_API
    UsdAttributeQuery(const UsdPrim& prim, const TfToken& attrName);

    /// Construct new queries for the attributes named in \p attrNames under
    /// the prim \p prim. The objects in the returned vector will line up
    /// 1-to-1 with \p attrNames.
    USD_API
    static std::vector<UsdAttributeQuery> CreateQueries(
        const UsdPrim& prim, const TfTokenVector& attrNames);

    // --------------------------------------------------------------------- //
    /// \name Query information
    // --------------------------------------------------------------------- //

    /// @{

    /// Return the attribute associated with this query.
    USD_API
    const UsdAttribute& GetAttribute() const;

    /// Return true if this query is valid (i.e. it is associated with a
    /// valid attribute), false otherwise.
    bool IsValid() const {
        return GetAttribute().IsValid();
    }

#ifdef doxygen
    /// Safe bool-conversion operator.  Equivalent to IsValid().
    operator unspecified-bool-type() const();
#else
private:
    typedef const UsdAttribute UsdAttributeQuery::*_UnspecifiedBoolType;
public:
    operator _UnspecifiedBoolType() const {
        return IsValid() ? &UsdAttributeQuery::_attr : NULL;
    }
#endif // doxygen

    /// @}

    // --------------------------------------------------------------------- //
    /// \name Value & Time-Sample Accessors
    // --------------------------------------------------------------------- //

    /// @{

    /// Perform value resolution to fetch the value of the attribute associated
    /// with this query at the requested UsdTimeCode \p time.
    ///
    /// \sa UsdAttribute::Get
    template <typename T>
    bool Get(T* value, UsdTimeCode time = UsdTimeCode::Default()) const {
        static_assert(SdfValueTypeTraits<T>::IsValueType,
                      "T must be an SdfValueType.");
        return _Get(value, time);
    }
    /// \overload
    /// Type-erased access, often not as efficient as typed access.
    USD_API
    bool Get(VtValue* value, UsdTimeCode time = UsdTimeCode::Default()) const;
    
    /// Populates a vector with authored sample times. 
    /// Returns false only on error. 
    //
    /// Behaves identically to UsdAttribute::GetTimeSamples()
    ///
    /// \sa UsdAttributeQuery::GetTimeSamplesInInterval
    USD_API
    bool GetTimeSamples(std::vector<double>* times) const;

    /// Populates a vector with authored sample times in \p interval.
    /// Returns false only on an error.
    ///
    /// Behaves identically to UsdAttribute::GetTimeSamplesInInterval()
    USD_API
    bool GetTimeSamplesInInterval(const GfInterval& interval,
                                  std::vector<double>* times) const;

    /// Populates the given vector, \p times with the union of all the 
    /// authored sample times on all of the given attribute-query objects, 
    /// \p attrQueries.
    /// 
    /// Behaves identically to UsdAttribute::GetUnionedTimeSamples()
    /// 
    /// \return false if one or more attribute-queries in \p attrQueries are 
    /// invalid or if there's an error fetching time-samples for any of 
    /// the attribute-query objects.
    ///
    /// \sa UsdAttribute::GetUnionedTimeSamples
    /// \sa UsdAttributeQuery::GetUnionedTimeSamplesInInterval
    USD_API
    static bool GetUnionedTimeSamples(
        const std::vector<UsdAttributeQuery> &attrQueries, 
        std::vector<double> *times);

    /// Populates the given vector, \p times with the union of all the 
    /// authored sample times in the GfInterval, \p interval on all of the 
    /// given attribute-query objects, \p attrQueries.
    /// 
    /// Behaves identically to UsdAttribute::GetUnionedTimeSamplesInInterval()
    /// 
    /// \return false if one or more attribute-queries in \p attrQueries are 
    /// invalid or if there's an error fetching time-samples for any of 
    /// the attribute-query objects.
    ///
    /// \sa UsdAttribute::GetUnionedTimeSamplesInInterval
    USD_API
    static bool GetUnionedTimeSamplesInInterval(
        const std::vector<UsdAttributeQuery> &attrQueries, 
        const GfInterval &interval,
        std::vector<double> *times);

    /// Returns the number of time samples that have been authored.
    /// 
    /// \sa UsdAttribute::GetNumTimeSamples
    USD_API
    size_t GetNumTimeSamples() const;

    /// Populate \a lower and \a upper with the next greater and lesser
    /// value relative to the \a desiredTime.
    ///
    /// \sa UsdAttribute::GetBracketingTimeSamples
    USD_API
    bool GetBracketingTimeSamples(double desiredTime, 
                                  double* lower, 
                                  double* upper, 
                                  bool* hasTimeSamples) const;

    /// Return true if the attribute associated with this query has an 
    /// authored default value, authored time samples or a fallback value 
    /// provided by a registered schema.
    ///
    /// \sa UsdAttribute::HasValue
    USD_API
    bool HasValue() const;

    /// Return true if the attribute associated with this query has either an 
    /// authored default value or authored time samples.
    ///
    /// \sa UsdAttribute::HasAuthoredValueOpinion
    USD_API
    bool HasAuthoredValueOpinion() const;

    /// Return true if the attribute associated with this query has a 
    /// fallback value provided by a registered schema.
    ///
    /// \sa UsdAttribute::HasFallbackValue
    USD_API
    bool HasFallbackValue() const;

    /// Return true if it is possible, but not certain, that this attribute's
    /// value changes over time, false otherwise. 
    ///
    /// \sa UsdAttribute::ValueMightBeTimeVarying
    USD_API
    bool ValueMightBeTimeVarying() const;

    /// @}

private:
    void _Initialize(const UsdAttribute& attr);

    template <typename T>
    USD_API
    bool _Get(T* value, UsdTimeCode time) const;

private:
    UsdAttribute _attr;
    UsdResolveInfo _resolveInfo;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_ATTRIBUTE_QUERY_H
