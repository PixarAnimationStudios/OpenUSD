//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_EDIT_REASON_H
#define PXR_EXEC_ESF_EDIT_REASON_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/esf/api.h"

#include <cstdint>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Set of scene changes that should trigger edits to the exec network.
///
/// The set of change types contained in an EsfEdtReason is stored as a
/// bitmask, where each bit represents a different type of scene change.
/// EsfEditReason%s can be manipulated with standard bitwise operators.
///
/// Users can only construct bitmasks from the provided set of supported edit
/// reasons.
///
class EsfEditReason
{
public:

    /// \name Supported edit reasons
    /// @{

    static const EsfEditReason None;

    /// Something about an object has changed.
    /// 
    /// This includes recursive resyncs on namespace ancestors.
    ///
    static const EsfEditReason ResyncedObject;

    /// The list of properties on a prim has changed.
    ///
    /// This includes renames to the prim's properties.
    ///
    static const EsfEditReason ChangedPropertyList;

    /// The list of target paths on a relationship has changed.
    ///
    static const EsfEditReason ChangedTargetPaths;

    /// @}

    /// \name Bitwise operations
    /// @{

    /// Return true if this object contains any edit reasons.
    constexpr explicit operator bool() const {
        return _bits;
    }

    constexpr bool operator==(EsfEditReason other) const {
        return _bits == other._bits;
    }

    constexpr bool operator!=(EsfEditReason other) const {
        return _bits != other._bits;
    }

    constexpr EsfEditReason& operator&=(EsfEditReason other) {
        _bits &= other._bits;
        return *this;
    }

    constexpr EsfEditReason& operator|=(EsfEditReason other) {
        _bits |= other._bits;
        return *this;
    }

    constexpr EsfEditReason operator&(EsfEditReason other) const {
        return {_bits & other._bits};
    }

    constexpr EsfEditReason operator|(EsfEditReason other) const {
        return {_bits | other._bits};
    }

    /// Return true if \p other's reasons are entirely contained by this set
    /// of reasons.
    ///
    constexpr bool Contains(EsfEditReason other) const {
        return (_bits & other._bits) == other._bits;
    }

    /// @}

    /// Enables consistent sorting of EsfEditReasons.
    bool operator<(const EsfEditReason &other) const {
        return _bits < other._bits;
    }

    /// Equivalent to EsfEditReason::None.
    constexpr EsfEditReason() = default;

    /// Get a string describing the contents of this edit reason.
    ///
    /// The string is a comma-separated list of pre-defined edit reasons that
    /// make up this value.
    ///
    ESF_API std::string GetDescription() const;

private:
    using _BitsType = uint32_t;

    // Private methods use this constructor to initialize from a raw bitmask.
    constexpr EsfEditReason(_BitsType bits) : _bits(bits) {}

    // By using an enum class value for each bit position, we make it less
    // error-prone to define new edit reasons.
    enum class _BitIndex : uint8_t {
        ResyncedObject,
        ChangedPropertyList,
        ChangedTargetPaths,
        Max
    };

    // Predefined edit reasons use this constructor.
    constexpr EsfEditReason(_BitIndex bit)
        : _bits(1 << static_cast<int>(bit))
    {}

    // Returns a string describing this _BitIndex enum value.
    static const char *_GetBitDescription(_BitIndex bit);

    _BitsType _bits = 0;
};

inline constexpr EsfEditReason EsfEditReason::None(0);

inline constexpr EsfEditReason EsfEditReason::ResyncedObject(
    EsfEditReason::_BitIndex::ResyncedObject);

inline constexpr EsfEditReason EsfEditReason::ChangedPropertyList(
    EsfEditReason::_BitIndex::ChangedPropertyList);

inline constexpr EsfEditReason EsfEditReason::ChangedTargetPaths(
    EsfEditReason::_BitIndex::ChangedTargetPaths);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
