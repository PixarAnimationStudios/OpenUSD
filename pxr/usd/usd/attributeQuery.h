//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_ATTRIBUTE_QUERY_H
#define PXR_USD_USD_ATTRIBUTE_QUERY_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/resolveInfo.h"
#include "pxr/usd/usd/resolveTarget.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/base/tf/token.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class TsSpline;

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
/// \section UsdAttributeQuery_Resolve_targets Resolve targets
/// An attribute query can also be constructed for an attribute along with a 
/// UsdResolveTarget. A resolve target allows value resolution to consider only
/// a subrange of the prim stack instead of the entirety of it. All of the methods 
/// of an attribute query created with a resolve target will perform value 
/// resolution within that resolve target. This can be useful for finding the
/// value of an attribute resolved up to a particular layer or for determining
/// if a value authored on layer would be overridden by a stronger opinion.
///
/// \section UsdAttributeQuery_Thread_safety Thread safety
/// This object provides the basic thread-safety guarantee.  Multiple threads
/// may call the value accessor functions simultaneously.
///
/// \section UsdAttributeQuery_Invalidation Invalidation
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

    /// Copy constructor.
    USD_API
    UsdAttributeQuery(const UsdAttributeQuery &other);

    /// Move constructor.
    USD_API
    UsdAttributeQuery(UsdAttributeQuery &&other) = default;

    /// Construct a new query for the attribute \p attr.
    USD_API
    explicit UsdAttributeQuery(const UsdAttribute& attr);

    /// Construct a new query for the attribute named \p attrName under
    /// the prim \p prim.
    USD_API
    UsdAttributeQuery(const UsdPrim& prim, const TfToken& attrName);

    /// Construct a new query for the attribute \p attr with the given 
    /// resolve target \p resolveTarget.
    ///
    /// Note that a UsdResolveTarget is associated with a particular prim so 
    /// only resolve targets for the attribute's owning prim are allowed.
    USD_API
    UsdAttributeQuery(const UsdAttribute &attr, 
        const UsdResolveTarget &resolveTarget);

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

public:
    /// Returns \c true if the query object is valid, \c false otherwise.
    explicit operator bool() const {
        return IsValid();
    }

    /// Copy assignment.
    USD_API
    UsdAttributeQuery &operator=(const UsdAttributeQuery &other);

    /// Move assignment.
    USD_API
    UsdAttributeQuery &operator=(UsdAttributeQuery &&other) = default;

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

    /// Returns a copy of the TsSpline associated with the resolved value.
    ///
    /// If the resolve value source is not a Spline, an empty Spline is 
    /// returned.
    USD_API
    TsSpline GetSpline() const;

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
    /// authored default value, authored time samples, authored spline or a 
    /// fallback value provided by a registered schema.
    ///
    /// \sa UsdAttribute::HasValue
    USD_API
    bool HasValue() const;

    /// Return true if the attribute associated with this query has an
    /// a spline value as the strongest opinion.
    ///
    /// \sa UsdAttribute::HasSpline
    /// \sa UsdAttributeQuery::GetSpline
    /// \sa UsdAttributeQuery::ValueMightBeTimeVarying
    USD_API
    bool HasSpline() const;

    /// \deprecated This method is deprecated because it returns `true` even when
    /// an attribute is blocked.  Please use HasAuthoredValue() instead. If 
    /// you truly need to know whether the attribute has **any** authored
    /// value opinions, *including blocks*, you can make the following query:
    /// `query.GetAttribute().GetResolveInfo().HasAuthoredValueOpinion()`
    ///
    ///
    /// Return true if this attribute has either an authored default value or
    /// authored time samples.
    USD_API
    bool HasAuthoredValueOpinion() const;

    /// Return true if this attribute has either an authored default value or
    /// authored time samples.  If the attribute has been 
    /// \ref Usd_AttributeBlocking "blocked", then return `false`
    /// \sa UsdAttribute::HasAuthoredValue()
    USD_API
    bool HasAuthoredValue() const;

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
    void _Initialize();

    void _Initialize(const UsdResolveTarget &resolveTarget);

    template <typename T>
    USD_API
    bool _Get(T* value, UsdTimeCode time) const;

private:
    UsdAttribute _attr;
    UsdResolveInfo _resolveInfo;
    std::unique_ptr<UsdResolveTarget> _resolveTarget;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_ATTRIBUTE_QUERY_H
