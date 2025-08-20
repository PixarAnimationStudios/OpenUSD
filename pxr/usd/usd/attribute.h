//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_ATTRIBUTE_H
#define PXR_USD_USD_ATTRIBUTE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/resolveInfo.h"

#include "pxr/usd/sdf/abstractData.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/gf/interval.h"

#include <string>
#include <type_traits>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class UsdAttribute;
class TsSpline;

/// A std::vector of UsdAttributes.
typedef std::vector<UsdAttribute> UsdAttributeVector;

/// \class UsdAttribute
///
/// Scenegraph object for authoring and retrieving numeric, string, and array
/// valued data, sampled over time, or animated by a spline
///
/// The allowed value types for UsdAttribute are dictated by the Sdf
/// ("Scene Description Foundations") core's data model, which we summarize in
/// \ref Usd_Page_Datatypes .
///
/// \section Usd_AttributeQualities Attribute Defining Qualities
///
/// In addition to its value type, an Attribute has two other defining
/// qualities:
/// \li <b>Variability</b> Expresses whether an attribute is intended to
/// have time samples or a spline (GetVariability() == \c SdfVariabilityVarying), 
/// or only a default (GetVariability() == \c SdfVariabilityUniform).  For more 
/// on reasoning about time samples, 
/// see \ref Usd_AttributeValueMethods "Value & Time-Sample Accessors".
///
/// \li <b>Custom</b> Determines whether an attribute belongs to a
/// schema (IsCustom() == \c false), or is a user-defined, custom attribute.
/// schema attributes will always be defined on a prim of the schema type,
/// and may possess fallback values from the schema, whereas custom 
/// attributes must always first be authored in order to be defined.  Note
/// that \em custom is actually an aspect of UsdProperty, as UsdRelationship
/// can also be custom or provided by a schema.
///
/// \section Usd_AttributeExistence Attribute Creation and Existence
///
/// One can always create an attribute generically via 
/// UsdPrim::CreateAttribute(), which ensures that an attribute "is defined"
/// in the current \ref UsdEditTarget .  In order to author any metadata or
/// a default or timesample for an attribute, <em>it must first be defined</em>.
/// It is sufficient that the attribute be defined in any one of the layers
/// participating in the stage's current composition; for \em builtin 
/// attributes (those belonging to the owning prim's defining schema, i.e.
/// the most specific subclass of UsdTypedSchema for which prim.IsA<schema>()
/// will evaluate to true) there need be no authored scene description, because
/// a definition is provided by the prim's schema definition.
///
/// <b>Creating</b> an attribute does not imply that the attribute has a value.
/// More broadly, in the following code:
/// \code
/// if (UsdAttribute attr = prim.GetAttribute(TfToken("myAttr"))){
///    ...
/// }
/// \endcode
///
/// The UsdAttribute passes the bool test, because it is defined; however, 
/// inside the clause, we have no guarantee that attr has a value.
///
/// \section Usd_AttributeInterpolation Attribute Value Interpolation
///
/// UsdAttribute supports two interpolation behaviors when retrieving
/// attribute values at times where no value is explicitly authored.
/// The desired behavior may be specified via UsdStage::SetInterpolationType.
/// That behavior will be used for all calls to UsdAttribute::Get.
///
/// Note that for attributes with spline value sources, the interpolation
/// behavior is determined by the spline itself, and the interpolation type
/// set on the stage is ignored.
///
/// The supported interpolation types are:
///
/// \li <b>Held</b> Attribute values are held constant between authored
/// values.  An attribute's value will be equal to the nearest preceding
/// authored value.  If there is no preceding authored value, the value 
/// will be equal to the nearest subsequent value.
///
/// \li <b>Linear</b> Attribute values are linearly interpolated between
/// authored values.
///
/// Linear interpolation is only supported for certain data types.  See 
/// \ref USD_LINEAR_INTERPOLATION_TYPES for the list of these types.  Types 
/// that do not support linear interpolation will use held interpolation 
/// instead.
///
/// Linear interpolation is done element-by-element for array, vector, 
/// and matrix data types.  If linear interpolation is requested for
/// two array values with different sizes, held interpolation will
/// be used instead.
///
/// \section Usd_AttributeBlocking Attribute Value Blocking
///
/// While prims can effectively be removed from a scene by
/// \ref Usd_ActiveInactive "deactivating them," properties cannot.  However,
/// it is possible to **block an attribute's value**, thus making the attribute
/// behave as if it has a definition (and possibly metadata), but no authored
/// value.  
/// 
///
/// One blocks an attribute using UsdAttribute::Block(), which will block the
/// attribute in the stage's current UsdEditTarget, by authoring an
/// SdfValueBlock in the attribute's *default*, and only values authored in
/// weaker layers than the editTarget will be blocked.  If the value block is
/// the strongest authored opinion for the attribute, the HasAuthoredValue()
/// method will return *false*, and the HasValue() and Get() methods will
/// only return *true* if the attribute possesses a fallback value from the
/// prim's schema.  "Unblocking" a blocked attribute is as simple as setting
/// a *default* or timeSample value for the attribute in the same or stronger
/// layer.
///
/// \subsection Usd_TimeVaryingAttributeBlocks Time-varying Blocks
///
/// The semantics of \ref Usd_ValueClips_Overview "Value Clips" necessitate
/// the ability to selectively block an attribute's value for only some intervals
/// in its authored range of samples.  One can block an attribute's value at
/// time *t* by calling `attr.Set(SdfValueBlock, t)` When an attribute is thusly
/// "partially blocked", UsdAttribute::Get() will succeed only for those time
/// intervals whose left/earlier bracketing timeSample is **not** SdfValueBlock.
///
/// Due to this time-varying potential of value blocking, it may be the case 
/// that an attribute's  HasAuthoredValue() and HasValue() methods both return
/// *true* (because they do not and cannot consider time-varying blocks), but
/// Get() may yet return *false* over some intervals.
///
/// \section Usd_AssetPathValuedAttributes Attributes of type SdfAssetPath and UsdAttribute::Get()
///
/// If an attribute's value type is SdfAssetPath or SdfAssetPathArray, Get()
/// does extra work to perform variable expression evaluation and compute
/// resolved asset paths. The layer that has the strongest value opinion is 
/// used as the anchor for "relative" asset paths.  The unresolved results are
/// available through SdfAssetPath::GetAssetPath. The fully resolved path
/// (including any substitutions) can be retrieved with
/// SdfAssetPath::GetResolvedPath. The authored or evaluated paths may
/// be explicitly retrieved through SdfAssetPath::GetAuthoredPath and 
/// SdfAssetPath::GetEvaluatedPath respectively.
///
/// Clients that call Get() on many asset-path-valued attributes may wish to
/// employ an ArResolverScopedCache to improve asset path resolution
/// performance.
///
class UsdAttribute : public UsdProperty {
public:
    /// Construct an invalid attribute.
    UsdAttribute() : UsdProperty(_Null<UsdAttribute>()) {}

    // --------------------------------------------------------------------- //
    /// \name Core Metadata
    // --------------------------------------------------------------------- //

    /// @{

    /// An attribute's variability expresses whether it is intended to have
    /// time-samples or splines (\c SdfVariabilityVarying), or only a single 
    /// default value (\c SdfVariabilityUniform).
    ///
    /// Variability is required meta-data of all attributes, and its fallback
    /// value is SdfVariabilityVarying.
    USD_API
    SdfVariability GetVariability() const;

    /// Set the value for variability at the current EditTarget, return true
    /// on success, false if the value can not be written.
    ///
    /// \b Note that this value should not be changed as it is typically either
    /// automatically authored or provided by a property definition. This method
    /// is provided primarily for fixing invalid scene description.
    USD_API
    bool SetVariability(SdfVariability variability) const;

    /// Return the "scene description" value type name for this attribute.
    USD_API
    SdfValueTypeName GetTypeName() const;

    /// Set the value for typeName at the current EditTarget, return true on
    /// success, false if the value can not be written.
    ///
    /// \b Note that this value should not be changed as it is typically either
    /// automatically authored or provided by a property definition. This method
    /// is provided primarily for fixing invalid scene description.
    USD_API
    bool SetTypeName(const SdfValueTypeName& typeName) const;

    /// Return the roleName for this attribute's typeName.
    USD_API
    TfToken GetRoleName() const;

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_AttributeValueMethods
    /// \name Value & Time-Sample Accessors
    // --------------------------------------------------------------------- //

    /// @{

    /// Populates a vector with authored sample times.
    /// Returns false only on error.
    ///
    /// This method uses the standard resolution semantics, so if a stronger
    /// default value is authored over weaker time samples, the default value
    /// will hide the underlying timesamples.
    ///
    /// \note This function will query all value clips that may contribute 
    /// time samples for this attribute, opening them if needed. This may be
    /// expensive, especially if many clips are involved.     
    /// 
    /// \param times - on return, will contain the \em sorted, ascending
    /// timeSample ordinates.  Any data in \p times will be lost, as this
    /// method clears \p times. 
    ///
    /// \sa UsdAttribute::GetTimeSamplesInInterval
    USD_API
    bool GetTimeSamples(std::vector<double>* times) const;

    /// Populates a vector with authored sample times in \p interval. 
    /// Returns false only on an error.
    ///
    /// \note This function will only query the value clips that may 
    /// contribute time samples for this attribute in the given interval, 
    /// opening them if necessary.
    /// 
    /// \param interval - the \ref GfInterval on which to gather time samples.     
    ///
    /// \param times - on return, will contain the \em sorted, ascending
    /// timeSample ordinates.  Any data in \p times will be lost, as this
    /// method clears \p times. 
    ///
    /// \sa UsdAttribute::GetTimeSamples
    USD_API
    bool GetTimeSamplesInInterval(const GfInterval& interval,
                                  std::vector<double>* times) const;

    /// Populates the given vector, \p times with the union of all the 
    /// authored sample times on all of the given attributes, \p attrs.
    /// 
    /// \note This function will query all value clips that may contribute 
    /// time samples for the attributes in \p attrs, opening them if needed. 
    /// This may be expensive, especially if many clips are involved.
    /// 
    /// The accumulated sample times will be in sorted (increasing) order and 
    /// will not contain any duplicates.
    /// 
    /// This clears any existing values in the \p times vector before 
    /// accumulating sample times of the given attributes.
    /// 
    /// \return false if any of the attributes in \p attr are invalid or  if 
    /// there's an error when fetching time-samples for any of the attributes.
    /// 
    /// \sa UsdAttribute::GetTimeSamples
    /// \sa UsdAttribute::GetUnionedTimeSamplesInInterval
    USD_API
    static bool GetUnionedTimeSamples(const std::vector<UsdAttribute> &attrs, 
                                      std::vector<double> *times);

    /// Populates the given vector, \p times with the union of all the 
    /// authored sample times in the GfInterval, \p interval on all of the 
    /// given attributes, \p attrs.
    /// 
    /// \note This function will only query the value clips that may 
    /// contribute time samples for the attributes in \p attrs, in the 
    /// given \p interval, opening them if necessary.
    ///
    /// The accumulated sample times will be in sorted (increasing) order and 
    /// will not contain any duplicates.
    /// 
    /// This clears any existing values in the \p times vector before 
    /// accumulating sample times of the given attributes.
    /// 
    /// \return false if any of the attributes in \p attr are invalid or if 
    /// there's an error fetching time-samples for any of the attributes.
    ///
    /// \sa UsdAttribute::GetTimeSamplesInInterval
    /// \sa UsdAttribute::GetUnionedTimeSamples
    USD_API
    static bool GetUnionedTimeSamplesInInterval(
        const std::vector<UsdAttribute> &attrs, 
        const GfInterval &interval,
        std::vector<double> *times);

    /// Returns the number of time samples that have been authored.
    ///
    /// This method uses the standard resolution semantics, so if a stronger
    /// default value is authored over weaker time samples, the default value
    /// will hide the underlying timesamples.
    ///
    /// \note This function will query all value clips that may contribute 
    /// time samples for this attribute, opening them if needed. This may be
    /// expensive, especially if many clips are involved.
    USD_API
    size_t GetNumTimeSamples() const;

    /// Populate \a lower and \a upper with the next greater and lesser
    /// value relative to the \a desiredTime. Return false if no value exists
    /// or an error occurs, true if either a default value or timeSamples exist.
    ///
    /// Use standard resolution semantics: if a stronger default value is
    /// authored over weaker time samples, the default value hides the
    /// underlying timeSamples.
    ///
    /// 1) If a sample exists at the \a desiredTime, set both upper and lower
    /// to \a desiredTime.
    ///
    /// 2) If samples exist surrounding, but not equal to the \a desiredTime,
    /// set lower and upper to the bracketing samples nearest to the
    /// \a desiredTime. 
    ///
    /// 3) If the \a desiredTime is outside of the range of authored samples, 
    /// clamp upper and lower to the nearest time sample.
    ///
    /// 4) If no samples exist, do not modify upper and lower and set
    /// \a hasTimeSamples to false.
    ///
    /// In cases (1), (2) and (3), set \a hasTimeSamples to true.
    ///
    /// All four cases above are considered to be successful, thus the return
    /// value will be true and no error message will be emitted.
    USD_API
    bool GetBracketingTimeSamples(double desiredTime, 
                                  double* lower, 
                                  double* upper, 
                                  bool* hasTimeSamples) const;

    /// Return true if this attribute has an authored default value, authored
    /// time samples or a fallback value provided by a registered schema. If
    /// the attribute has been \ref Usd_AttributeBlocking "blocked", then
    /// return `true` if and only if it has a fallback value.
    USD_API
    bool HasValue() const;

    /// \deprecated This method is deprecated because it returns `true` even when
    /// an attribute is blocked.  Please use HasAuthoredValue() instead.  If 
    /// you truly need to know whether the attribute has **any** authored
    /// value opinions, *including blocks*, you can make the following query:
    /// `attr.GetResolveInfo().HasAuthoredValueOpinion()`
    ///
    /// Return true if this attribute has either an authored default value or
    /// authored time samples.
    USD_API
    bool HasAuthoredValueOpinion() const;

    /// Return true if this attribute has either an authored default value or
    /// authored time samples.  If the attribute has been 
    /// \ref Usd_AttributeBlocking "blocked", then return `false`
    USD_API
    bool HasAuthoredValue() const;

    /// Return true if this attribute has a fallback value provided by 
    /// a registered schema.
    USD_API
    bool HasFallbackValue() const;

    /// Return true if it is possible, but not certain, that this attribute's
    /// value changes over time, false otherwise. 
    ///
    /// If this function returns false, it is certain that this attribute's
    /// value remains constant over time.
    ///
    /// This function checks if the attribute either has more than 1 time
    /// samples or is spline valued. Which is more efficient than actually
    /// counting the time samples or evaluating the spline, both of which
    /// are potentially expensive operations.
    USD_API
    bool ValueMightBeTimeVarying() const;

    /// Perform value resolution to fetch the value of this attribute at the
    /// requested UsdTimeCode \p time, which defaults to \em default.
    ///
    /// If no value is authored at \p time but values are authored at other
    /// times, this function will return an interpolated value based on the 
    /// stage's interpolation type.
    /// See \ref Usd_AttributeInterpolation.
    ///
    /// An attribute's value may be discontinuous at time samples. This happens 
    /// when the stage is in held interpolation mode or when the sample values
    /// are not interpolatable. To obtain the attribute's value immediately
    /// before a given time sample, use 
    /// \link UsdTimeCode::PreTime() UsdTimeCode::PreTime(time)\endlink. This 
    /// evaluates the limit of the attribute's value as time approaches the 
    /// given \p time from the left. 
    ///
    /// For example, if a string-valued attribute has time samples 
    /// `{1.0: "foo", 2.0: "bar"}`, calling Get() with UsdTimeCode(2.0) 
    /// returns "bar", whereas calling Get() with UsdTimeCode::PreTime(2.0) 
    /// returns "foo". However, if the attribute's values are interpolatable, 
    /// such as `{1.0: 3.0, 2.0: 4.0}`, then calling Get() with UsdTimeCode(2.0)  
    /// and UsdTimeCode::PreTime(2.0) will both return 4.0, since the value 
    /// is continuous at time=2.0.
    ///
    /// If no value is authored and no fallback value is provided by the 
    /// schema for this attribute, this function will return false. If the 
    /// consumer's use-case requires a default value, the consumer will need
    /// to provide one, possibly using GetTypeName().GetDefaultValue().
    ///
    /// Value resolution first needs to determine the source of the strongest
    /// value opinion for this attribute at the requested UsdTimeCode \p time.
    /// But often (i.e. unless the attribute is affected by 
    /// \ref Usd_Page_ValueClips "Value Clips") the source of the resolved value
    /// does not vary over time. UsdAttributeQuery finds the source opinion and 
    /// saves it so that repeated calls to UsdAttributeQuery::Get() avoid 
    /// redundant work.
    ///
    /// This templated accessor is designed for high performance data-streaming
    /// applications, allowing one to fetch data into the same container
    /// repeatedly, avoiding memory allocations when possible (VtArray
    /// containers will be resized as necessary to conform to the size of
    /// data being read).
    ///
    /// This template is only instantiated for the valid scene description
    /// value types and their corresponding VtArray containers. See
    /// \ref Usd_Page_Datatypes for the complete list of types.
    ///
    /// Values are retrieved without regard to this attribute's variability.
    /// For example, a uniform attribute may retrieve time sample values 
    /// if any are authored. However, the USD_VALIDATE_VARIABILITY TF_DEBUG 
    /// code will cause debug information to be output if values that are 
    /// inconsistent with this attribute's variability are retrieved. 
    /// See UsdAttribute::GetVariability for more details.
    ///
    /// \return true if there was a value to be read, it was of the type T
    /// requested, and we read it successfully - false otherwise.
    ///
    /// For more details, see \ref Usd_ValueResolution , and also
    /// \ref Usd_AssetPathValuedAttributes for information on how to
    /// retrieve resolved asset paths from SdfAssetPath-valued attributes.
    template <typename T>
    bool Get(T* value, UsdTimeCode time = UsdTimeCode::Default()) const {
        static_assert(!std::is_const<T>::value, "");
        static_assert(SdfValueTypeTraits<T>::IsValueType, "");
        return _Get(value, time);
    }
    /// \overload 
    /// Type-erased access, often not as efficient as typed access.
    USD_API
    bool Get(VtValue* value, UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Perform value resolution to determine the source of the resolved
    /// value of this attribute at the requested UsdTimeCode \p time.
    USD_API
    UsdResolveInfo
    GetResolveInfo(UsdTimeCode time) const;

    /// Perform value resolution to determine the source of the resolved
    /// value of this attribute at any non-default time. 
    ///
    /// Often (i.e. unless the attribute is affected by 
    /// \ref Usd_Page_ValueClips "Value Clips") the source of the resolved value
    /// does not vary over time. See UsdAttributeQuery as an example that takes
    /// advantage of this quality of value resolution.
    USD_API
    UsdResolveInfo
    GetResolveInfo() const;

    /// Set the value of this attribute in the current UsdEditTarget to
    /// \p value at UsdTimeCode \p time, which defaults to \em default.
    ///
    /// Values are authored without regard to this attribute's variability. 
    /// For example, time sample values may be authored on a uniform
    /// attribute. However, the USD_VALIDATE_VARIABILITY TF_DEBUG code
    /// will cause debug information to be output if values that are
    /// inconsistent with this attribute's variability are authored. 
    /// See UsdAttribute::GetVariability for more details.
    ///
    /// \return false and generate an error if type \c T does not match
    /// this attribute's defined scene description type <b>exactly</b>,
    /// or if there is no existing definition for the attribute, or if the
    /// \p time is pre-time, which is only used to for querying for values at 
    /// the limit when the time is approached from the left.
    template <typename T>
    bool Set(const T& value, UsdTimeCode time = UsdTimeCode::Default()) const {
        static_assert(!std::is_pointer<T>::value, "");
        static_assert(SdfValueTypeTraits<T>::IsValueType ||
                      std::is_same<T, SdfValueBlock>::value, "");
        return _Set(value, time);
    }

    /// \overload
    /// As a convenience, we allow the setting of string value typed attributes
    /// via a C string value.
    USD_API
    bool Set(const char* value, UsdTimeCode time = UsdTimeCode::Default()) const;

    /// \overload 
    USD_API
    bool Set(const VtValue& value, UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Returns true if the attribute has a spline as a value source.
    ///
    /// That is if a stronger default value is authored over weaker spline
    /// value, the default value will hide the spline value and return false.
    USD_API
    bool HasSpline() const;

    /// Returns a copy of the spline.
    ///
    /// If a stronger default value is authored over weaker spline value, the
    /// default value will hide the spline value.
    USD_API
    TsSpline GetSpline() const;

    /// Set the spline using the current edit target.
    USD_API
    bool SetSpline(const TsSpline &spline);

    /// Clears the authored default value, all time samples and spline for this
    /// attribute at the current EditTarget and returns true on success.
    ///
    /// Calling clear when either no value is authored or no spec is present,
    /// is a silent no-op returning true.    
    ///
    /// This method does not affect any other data authored on this attribute.
    USD_API
    bool Clear() const;

    /// Clear the authored value for this attribute at the given 
    /// \a time, at the current EditTarget and return true on success. 
    /// UsdTimeCode::Default() can be used to clear the default value.
    ///
    /// Calling clear when either no value is authored or no spec is present,
    /// is a silent no-op returning true. 
    ///
    /// Issue a coding error if \p time is a pre-time, which is only used to
    /// for querying for values at the limit when the time is approached from
    /// the left.
    USD_API
    bool ClearAtTime(UsdTimeCode time) const;

    /// Shorthand for ClearAtTime(UsdTimeCode::Default()).
    USD_API
    bool ClearDefault() const;

    /// Remove all time samples on an attribute and author a *block*
    /// \c default value. This causes the attribute to resolve as 
    /// if there were no authored value opinions in weaker layers.
    ///
    /// See \ref Usd_AttributeBlocking for more information, including
    /// information on time-varying blocking.
    USD_API
    void Block() const;

    /// @}

    /// \name Querying and Editing Connections
    /// @{

    /// Adds \p source to the list of connections, in the position
    /// specified by \p position.
    ///
    /// Issue an error if \p source identifies a prototype prim or an object
    /// descendant to a prototype prim.  It is not valid to author connections
    /// to these objects. 
    ///
    /// What data this actually authors depends on what data is currently
    /// authored in the authoring layer, with respect to list-editing
    /// semantics, which we will document soon 
    USD_API
    bool AddConnection(const SdfPath& source,
           UsdListPosition position=UsdListPositionBackOfPrependList) const;

    /// Removes \p target from the list of targets.
    ///
    /// Issue an error if \p source identifies a prototype prim or an object
    /// descendant to a prototype prim.  It is not valid to author connections
    /// to these objects.
    USD_API
    bool RemoveConnection(const SdfPath& source) const;

    /// Make the authoring layer's opinion of the connection list explicit,
    /// and set exactly to \p sources.
    ///
    /// Issue an error if \p source identifies a prototype prim or an object
    /// descendant to a prototype prim.  It is not valid to author connections
    /// to these objects.
    ///
    /// If any path in \p sources is invalid, issue an error and return false.
    USD_API
    bool SetConnections(const SdfPathVector& sources) const;

    /// Remove all opinions about the connections list from the current edit
    /// target.
    USD_API
    bool ClearConnections() const;

    /// Compose this attribute's connections and fill \p sources with the
    /// result.  All preexisting elements in \p sources are lost.
    ///
    /// Returns true if any connection path opinions have been authored and no
    /// composition errors were encountered, returns false otherwise. 
    /// Note that authored opinions may include opinions that clear the 
    /// connections and a return value of true does not necessarily indicate 
    /// that \p sources will contain any connection paths.
    /// 
    /// See \ref Usd_ScenegraphInstancing_TargetsAndConnections for details on 
    /// behavior when targets point to objects beneath instance prims.
    ///
    /// The result is not cached, and thus recomputed on each query.
    USD_API
    bool GetConnections(SdfPathVector* sources) const;

    /// Return true if this attribute has any authored opinions regarding
    /// connections.  Note that this includes opinions that remove connections,
    /// so a true return does not necessarily indicate that this attribute has
    /// connections.
    USD_API
    bool HasAuthoredConnections() const;

    /// @}
    
    // ---------------------------------------------------------------------- //
    /// \anchor Usd_AttributeColorSpaceAPI
    /// \name ColorSpace API
    /// 
    /// The color space in which a given color or texture valued attribute is 
    /// authored is set as token-valued metadata 'colorSpace' on the attribute.
    /// Please refer to GfColorSpaceNames for a list of built in color space
    /// token values.
    ///
    /// For color or texture attributes that don't have an authored 'colorSpace'
    /// value, the fallback color space may be authored on the owning prim,
    /// and determined using the UsdColorSpaceAPI applied schema.
    ///
    /// \ref GfColorSpaceNames "Standard color space names"
    ///
    /// @{
    // ---------------------------------------------------------------------- //

    /// Gets the color space in which the attribute is authored if it has been
    /// explicitly set. If the color space is not authored, any color space
    /// set on the attribute's prim definiton will be returned.
    /// Use \ref UsdColorSpaceAPI in order to compute the color space taking
    /// into account any inherited color spaces.
    ///
    /// \sa SetColorSpace()
    /// \ref GfColorSpaceNames "Standard color space names"
    /// \ref UsdColorSpaceAPI "Usd Prim Color Space API"
    USD_API
    TfToken GetColorSpace() const;

    /// Sets the color space of the attribute to \p colorSpace.
    /// \param colorSpace The target color space for this attribute.
    ///
    /// \ref UsdColorSpaceAPI "Usd Prim Color Space API" provides methods 
    /// to compute an attribute's resolved color, considering any inherited 
    /// colorspaces. Standard color space names are listed in 
    /// \ref GfColorSpaceNames.
    ///
    /// \sa GetColorSpace()
    USD_API
    void SetColorSpace(const TfToken &colorSpace) const;

    /// Returns whether color space is authored on the attribute.
    /// \sa GetColorSpace()
    USD_API
    bool HasColorSpace() const;

    /// Clears authored color space value on the attribute.
    /// \sa SetColorSpace()
    USD_API
    bool ClearColorSpace() const;

    /// @}

    // ---------------------------------------------------------------------- //
    // Private Methods and Members 
    // ---------------------------------------------------------------------- //
private:
    friend class UsdAttributeQuery;
    friend class UsdObject;
    friend class UsdPrim;
    friend class UsdSchemaBase;
    friend class Usd_PrimData;
    friend struct UsdPrim_AttrConnectionFinder;
    
    UsdAttribute(const Usd_PrimDataHandle &prim,
                 const SdfPath &proxyPrimPath,
                 const TfToken &attrName)
        : UsdProperty(UsdTypeAttribute, prim, proxyPrimPath, attrName) {}

    UsdAttribute(UsdObjType objType,
                 const Usd_PrimDataHandle &prim,
                 const SdfPath &proxyPrimPath,
                 const TfToken &propName)
        : UsdProperty(objType, prim, proxyPrimPath, propName) {}

    SdfAttributeSpecHandle
    _CreateSpec(const SdfValueTypeName &typeName, bool custom,
                const SdfVariability &variability) const;

    // Like _CreateSpec(), but fail if this attribute is not built-in and there
    // isn't already existing scene description to go on rather than stamping
    // new information.
    SdfAttributeSpecHandle _CreateSpec() const;

    bool _Create(const SdfValueTypeName &typeName, bool custom,
                 const SdfVariability &variability) const;

    template <typename T>
    bool _Get(T* value, UsdTimeCode time) const;

    template <typename T>
    bool _Set(const T& value, UsdTimeCode time) const;

    SdfPath
    _GetPathForAuthoring(const SdfPath &path, std::string* whyNot) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_ATTRIBUTE_H
