//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_USD_ATTRIBUTE_LIMITS_H
#define PXR_USD_USD_ATTRIBUTE_LIMITS_H

#include "pxr/usd/usd/attribute.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <optional>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

#define USD_LIMITS_KEYS                 \
    ((Soft,         "soft"))            \
    ((Hard,         "hard"))            \
    ((Minimum,      "minimum"))         \
    ((Maximum,      "maximum"))

TF_DECLARE_PUBLIC_TOKENS(
    UsdLimitsKeys, USD_API, USD_LIMITS_KEYS);

/// \class UsdAttributeLimits
///
/// Provides API for retrieving and authoring values within a particular
/// sub-dictionary of the \c limits dictionary metadata field on a UsdAttribute
/// instance. Within a given sub-dictionary, minimum and maximum values are
/// encoded under the \c UsdLimitsKeys->Minimum and \c UsdLimitsKeys->Maximum
/// keys, respectively.
///
/// For example, to express that an attribute's typical useful value range is
/// between 5 and 10 (the "soft limits"), but that it _must_ be between 0 and 15
/// (the "hard limits"), a typical limits dictionary might look like the
/// following:
///
/// \code
/// def "MyPrim"
/// {
///     int attr = 7 (
///         limits = {
///             dictionary soft = {
///                 int minimum = 5
///                 int maximum = 10
///             }
///             dictionary hard = {
///                 int minimum = 0
///                 int maximum = 15
///             }
///         }
///     )
/// }
/// \endcode
///
/// To work with these values, use the UsdAttributeLimits objects returned by
/// UsdAttribute::GetSoftLimits() and UsdAttribute::GetHardLimits(), which edit
/// and interpret the "soft" and "hard" sub-dictionaries, respectively.
///
/// \code
/// UsdAttributeLimits softLimits = attr.GetSoftLimits();
/// if (x >= softLimits.GetMinimumOr(0) && x <= softLimits.GetMaximumOr(100)) {
///     // x is within the soft limits, 5 and 10. The passed-in defaults
///     // (0 and 100) are ignored since both min and max values are authored
/// }
/// \endcode
///
/// You can also create custom sub-dictionaries:
///
/// \code
/// UsdAttributeLimits customLimits = attr.GetLimits(TfToken("myCustomSubDict"));
/// customLimits.SetMinimum(50);
/// customLimits.SetMaximum(100);
/// customLimits.Set(TfToken("customKey"), 42.5));
/// \endcode
///
/// Combined with the starting limits dictionary above, this would produce:
///
/// \code
/// def "MyPrim"
/// {
///     int attr = 7 (
///         limits = {
///             dictionary soft = {
///                 int minimum = 5
///                 int maximum = 10
///             }
///             dictionary hard = {
///                 int minimum = 0
///                 int maximum = 15
///             }
///             dictionary myCustomSubDict = {
///                 int minimum = 50,
///                 int maximum = 100,
///                 double customKey = 42.5
///             }
///         }
///     )
/// }
/// \endcode
class UsdAttributeLimits
{
public:
    /// Construct an invalid limits object.
    ///
    /// Calling "set" operations on an invalid limits object will post errors.
    /// "Get" operations will return empty.
    UsdAttributeLimits() = default;

    /// Construct a limits object for the sub-dictionary given by \p subDictKey
    /// in \p attr's limits dictionary.
    USD_API
    UsdAttributeLimits(
        const UsdAttribute& attr,
        const TfToken& subDictKey);

    /// \name Object Information
    ///
    /// Information about the limits object itself.
    ///
    /// @{

    /// Return whether the limits object is valid.
    ///
    /// Calling "set" operations on an invalid limits object will post errors.
    /// "Get" operations will return empty.
    USD_API
    bool IsValid() const;

    /// Return the limits object's attribute.
    USD_API
    UsdAttribute GetAttribute() const;

    /// Return the sub-dictionary key the limits object is using.
    USD_API
    TfToken GetSubDictKey() const;

    /// @}

    /// \name Opinions API
    ///
    /// API for determining whether authored opinions exist, and clearing them.
    ///
    /// @{

    /// Return whether any authored opinions exist for the limits
    /// sub-dictionary.
    USD_API
    bool HasAuthored() const;

    /// Clear all authored opinions for the limits sub-dictionary at the current
    /// edit target. Return \c true if successful.
    USD_API
    bool Clear();

    /// Return whether an authored opinion for \p key exists in the limits
    /// sub-dictionary.
    USD_API
    bool HasAuthored(const TfToken& key) const;

    /// Clear the authored opinion for \p key in the limits sub-dictionary.
    /// Return \c true if successful.
    USD_API
    bool Clear(const TfToken& key);

    /// Return whether an authored minimum value opinion exists in the limits
    /// sub-dictionary.
    USD_API
    bool HasAuthoredMinimum() const;

    /// Clear the authored minimum value opinion in the limits sub-dictionary.
    /// Return \c true if successful.
    USD_API
    bool ClearMinimum();

    /// Return whether an authored maximum value opinion exists in the limits
    /// sub-dictionary.
    USD_API
    bool HasAuthoredMaximum() const;

    /// Clear the authored maximum value opinion in the limits sub-dictionary.
    /// Return \c true if successful.
    USD_API
    bool ClearMaximum();

    /// @}

    /// \name Sub-dictionary Operations
    ///
    /// Operations related to the entire limits sub-dictionary.
    /// @{

    /// Validation information for a limits sub-dictionary.
    ///
    /// \sa Validate()
    class ValidationResult
    {
    public:
        /// Construct an empty result.
        ValidationResult() = default;

        /// Return whether validation was successful.
        bool Success() const {
            return _success;
        }

        /// Return a dictionary containing values from the source sub-dictionary
        /// that did not match (and could not be casted to) the attribute's
        /// type.
        ///
        /// Only minimum and maximum values must match the attribute's value
        /// type. Other keys are not validated.
        const VtDictionary& GetInvalidValuesDict() const {
            return _invalidValuesDict;
        }

        /// Return the conformed limits sub-dictionary. Any values from the
        /// source sub-dictionary that were of an unexpected type will have been
        /// casted to the attribute's value type, if possible, in the conformed
        /// sub-dictionary.
        ///
        /// If any unexpected values from the source sub-dictionary could not be
        /// casted, they will appear in the invalid values dictionary, and the
        /// conformed sub-dictionary will be empty.
        const VtDictionary& GetConformedSubDict() const {
            return _conformedSubDict;
        }

        /// Return a formatted error string describing the keys and values in
        /// the invalid values dictionary. Return an empty string if there are
        /// no invalid values.
        USD_API
        std::string GetErrorString() const;

        /// Return \c true if validation was successful.
        ///
        /// \sa Success()
        explicit operator bool() const {
            return Success();
        }

        /// Equality operator.
        bool operator==(const ValidationResult& rhs) const {
            return _success == rhs._success &&
                   _invalidValuesDict == rhs._invalidValuesDict &&
                   _conformedSubDict == rhs._conformedSubDict &&
                   _attrPath == rhs._attrPath &&
                   _attrTypeName == rhs._attrTypeName;
        }

        /// Inequality operator.
        bool operator!=(const ValidationResult& rhs) const {
            return !(*this == rhs);
        }

    private:
        friend class UsdAttributeLimits;
        ValidationResult(
            bool success,
            const VtDictionary& invalidValuesDict,
            const VtDictionary& conformedSubDict,
            const SdfPath& attrPath,
            const std::string& attrTypeName)
            : _success(success),
              _invalidValuesDict(invalidValuesDict),
              _conformedSubDict(conformedSubDict),
              _attrPath(attrPath),
              _attrTypeName(attrTypeName) {}

    private:
        bool _success = false;
        VtDictionary _invalidValuesDict;
        VtDictionary _conformedSubDict;
        SdfPath _attrPath;
        std::string _attrTypeName;
    };

    /// Return whether \p subDict is a valid limits sub-dictionary. If \p result
    /// is provided, fill it in.
    //
    /// To be valid, the types of encoded minimum and maximum values must match
    /// the value type of the attribute.
    ///
    /// \sa ValidationResult
    USD_API
    bool Validate(
        const VtDictionary& subDict,
        ValidationResult* result = nullptr) const;

    /// Set the entire limits sub-dictionary to \p subDict. Return \c true
    /// if successful.
    ///
    /// The types of encoded minimum and maximum values must match the value
    /// type of the attribute.
    USD_API
    bool Set(const VtDictionary& subDict);

    /// @}

    /// \name Typed Value API
    ///
    /// Templated API for retrieving and authoring individual limits values.
    ///
    /// @{

    /// Return the value encoded under \p key in the limits sub-dictionary.
    /// Return an empty \c std::optional if no authored or fallback
    /// value is present, or if the template type \c T does not match the type
    /// of the stored value.
    template <typename T>
    std::optional<T> Get(const TfToken& key) const;

    /// Return the value encoded under \p key in the limits sub-dictionary.
    /// Return \p defaultValue if no authored or fallback value is present, or
    /// if the template type \c T does not match the type of the stored value.
    template <typename T>
    T GetOr(const TfToken& key, const T& defaultValue) const;

    /// Set the value encoded under \p key in the limits sub-dictionary to
    /// \p value. Return \c true if successful.
    template <typename T>
    bool Set(const TfToken& key, const T& value);

    /// Return the minimum value from the limits sub-dictionary. Return an
    /// empty \c std::optional if no authored or fallback value is present, or
    /// if the template type \c T does not match the type of the stored value.
    template <typename T>
    std::optional<T> GetMinimum() const;

    /// Return the minimum value from the limits sub-dictionary. Return
    /// \p defaultValue if no authored or fallback value is present, or
    /// if the template type \c T does not match the type of the stored value.
    template <typename T>
    T GetMinimumOr(const T& defaultValue) const;

    /// Set the minimum value in the limits sub-dictionary. Return \c true if
    /// successful.
    ///
    /// The type of \p value must match the attribute's value type.
    template <typename T>
    bool SetMinimum(const T& value);

    /// Return the maximum value from the limits sub-dictionary. Return an
    /// empty \c std::optional if no authored or fallback value is present, or
    /// if the template type \c T does not match the type of the stored value.
    template <typename T>
    std::optional<T> GetMaximum() const;

    /// Return the maximum value from the limits sub-dictionary. Return
    /// \p defaultValue if no authored or fallback value is present, or if
    /// the template type \c T does not match the type of the stored value.
    template <typename T>
    T GetMaximumOr(const T& defaultValue) const;

    /// Set the maximum value in the limits sub-dictionary. Return \c true if
    /// successful.
    ///
    /// The type of \p value must match the attribute's value type.
    template <typename T>
    bool SetMaximum(const T& value);

    /// @}

    /// \name Type-erased Value API
    ///
    /// VtValue-based overloads, mainly for use by the Python wrapping.
    ///
    /// @{

    /// \overload
    USD_API
    VtValue Get(const TfToken& key) const;

    /// \overload
    USD_API
    bool Set(const TfToken& key, const VtValue& value);

    /// \overload
    USD_API
    VtValue GetMinimum() const;

    /// \overload
    USD_API
    bool SetMinimum(const VtValue& value);

    /// \overload
    USD_API
    VtValue GetMaximum() const;

    /// \overload
    USD_API
    bool SetMaximum(const VtValue& value);

    /// @}

    /// Return \c true if this limits object is valid.
    ///
    /// \sa IsValid()
    explicit operator bool() const {
        return IsValid();
    }

    /// Equality operator.
    bool operator==(const UsdAttributeLimits& rhs) const {
        return _attr == rhs._attr && _subDictKey == rhs._subDictKey;
    }

    /// Inequality operator.
    bool operator!=(const UsdAttributeLimits& rhs) const {
        return !(*this == rhs);
    }

private:
    USD_API
    static TfToken _MakeKeyPath(const TfToken&, const TfToken&);

private:
    UsdAttribute _attr;
    TfToken _subDictKey;
};

template <typename T>
inline std::optional<T>
UsdAttributeLimits::Get(const TfToken& key) const
{
    if (!IsValid() || key.IsEmpty()) {
        return {};
    }

    T value;
    if (_attr.GetMetadataByDictKey(
            SdfFieldKeys->Limits,
            _MakeKeyPath(_subDictKey, key),
            &value)) {
        return value;
    }
    return {};
}

template <typename T>
inline T
UsdAttributeLimits::GetOr(
    const TfToken& key,
    const T& defaultValue) const
{
    if (!IsValid() || key.IsEmpty()) {
        return defaultValue;
    }

    T value;
    if (_attr.GetMetadataByDictKey(
            SdfFieldKeys->Limits,
            _MakeKeyPath(_subDictKey, key),
            &value)) {
        return value;
    }
    return defaultValue;
}

template <typename T>
inline bool
UsdAttributeLimits::Set(
    const TfToken& key,
    const T& value)
{
    return Set(key, VtValue(value));
}

template <typename T>
inline std::optional<T>
UsdAttributeLimits::GetMinimum() const
{
    return Get<T>(UsdLimitsKeys->Minimum);
}

template <typename T>
inline T
UsdAttributeLimits::GetMinimumOr(const T& defaultValue) const
{
    return GetOr<T>(UsdLimitsKeys->Minimum, defaultValue);
}

template <typename T>
inline bool
UsdAttributeLimits::SetMinimum(const T& value)
{
    return Set(UsdLimitsKeys->Minimum, value);
}

template <typename T>
inline std::optional<T>
UsdAttributeLimits::GetMaximum() const
{
    return Get<T>(UsdLimitsKeys->Maximum);
}

template <typename T>
inline T
UsdAttributeLimits::GetMaximumOr(const T& defaultValue) const
{
    return GetOr<T>(UsdLimitsKeys->Maximum, defaultValue);
}

template <typename T>
inline bool
UsdAttributeLimits::SetMaximum(const T& value)
{
    return Set(UsdLimitsKeys->Maximum, value);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
