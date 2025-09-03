//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_UNCOMPILATION_RULE_SET_H
#define PXR_EXEC_EXEC_UNCOMPILATION_RULE_SET_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"
#include "pxr/exec/exec/uncompilationTarget.h"

#include "pxr/exec/esf/editReason.h"

#include <tbb/concurrent_vector.h>

#include <initializer_list>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// A rule for uncompiling an object in the VdfNetwork.
///
/// The rule is applicable to scene changes whose reasons intersect the
/// `reasons` field. These reasons implicitly correspond to an SdfPath
/// maintained by the Exec_UncompliationTable. If the reasons match, then
/// `target` should be uncompiled.
///
struct Exec_UncompilationRule
{
    Exec_UncompilationTarget target;
    EsfEditReason reasons;

    /// Constructs a new rule.
    ///
    /// The type of \p target_ must be a type held by the
    /// Exec_UncompilationTarget variant.
    ///
    template <class TargetType>
    Exec_UncompilationRule(TargetType &&target_, const EsfEditReason reasons_)
        : target(std::forward<TargetType>(target_))
        , reasons(reasons_)
    {}

    /// Returns a string describing the rule's target and reasons.
    EXEC_API
    std::string GetDescription() const;
};

/// Contains a set of rules for uncompiling objects in the VdfNetwork.
///
/// The rules are instances of Exec_UncompilationRule, stored in arbitrary
/// order, and may contain duplicates. To locate a particular rule in the set,
/// clients need to scan all contained rules.
///
class Exec_UncompilationRuleSet
{
private:
    using _ConcurrentVector = tbb::concurrent_vector<Exec_UncompilationRule>;

public:
    Exec_UncompilationRuleSet() = default;
    
    /// Constructs a set pre-filled with the given \p rules.
    Exec_UncompilationRuleSet(
        std::initializer_list<Exec_UncompilationRule> rules)
        : _rules(rules)
    {}

    /// Inserts a rule into the set.
    ///
    /// The \p args are forwarded to the Exec_UncompilationRule constructor,
    /// allowing insertion of "node" rules or "input" rules.
    ///
    /// \note
    /// This method can be called concurrently with other calls to emplace_back,
    /// but cannot be called concurrently with any other method.
    ///
    template <class... Args>
    void emplace_back(Args &&...args) {
        _rules.emplace_back(std::forward<Args>(args)...);
    }

    /// \name Uncompilation API
    ///
    /// These methods are called on rule sets during change processing in order
    /// to determine which network objects need to be uncompiled.
    ///
    /// @{

    /// Returns the number of items in the set, including any duplicates.
    size_t size() const { 
        return _rules.size();
    }

    using iterator = _ConcurrentVector::iterator;
    using const_iterator = _ConcurrentVector::const_iterator;

    /// Returns an iterator to the beginning of the rule set.
    iterator begin() {
        return _rules.begin();
    }

    /// Returns a const iterator to the beginning of the rule set.
    const_iterator begin() const {
        return _rules.begin();
    }

    /// Returns an iterator to the end of the rule set.
    ///
    /// The iterator returned by this method becomes invalid if a client erases
    /// an element.
    ///
    iterator end() {
        return _rules.end();
    }

    /// Returns a const iterator to the end of the rule set.
    ///
    /// The iterator returned by this method becomes invalid if a client erases
    /// an element.
    ///
    const_iterator end() const {
        return _rules.end();
    }

    /// Removes the item referred to by \p it from the set.
    ///
    /// \p it must be a non-end iterator to this set.
    ///
    /// This returns an iterator to the next item for iteration, which may be
    /// end() if \p it was the last element. Upon returning, the size of the
    /// set is decreased by 1.
    ///
    /// Erasing elements does not alter the capacity of the set. After multiple
    /// calls to erase, clients may choose to call shrink_to_fit to reclaim
    /// unused memory.
    ///
    /// \note
    /// This method is not thread-safe.
    ///
    EXEC_API iterator erase(const iterator &it);

    /// Reduces the capacity to match the size of the set.
    ///
    /// \note
    /// This method is not thread-safe.
    ///
    void shrink_to_fit() {
        _rules.shrink_to_fit();
    }

    /// @}

    /// Returns a string describing all rules in the set.
    EXEC_API std::string GetDescription() const;

private:
    _ConcurrentVector _rules;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif