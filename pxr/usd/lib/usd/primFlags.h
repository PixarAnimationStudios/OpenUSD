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
#ifndef USD_PRIMFLAGS_H
#define USD_PRIMFLAGS_H

/// \file usd/primFlags.h
///
/// \anchor Usd_PrimFlags
///
/// Provides terms for UsdPrim flags that can be combined to form either a
/// conjunction (via &&) or a disjunction (via ||).  The result is a
/// predicate functor object that tests those flags on the passed prim.
/// Currently UsdPrim::GetFilteredChildren(), UsdPrim::GetNextFilteredSibling(),
/// UsdPrim::GetFilteredDescendants(), and UsdPrimRange() accept these
/// predicates to filter out unwanted prims.
///
/// For example:
/// \code
/// // Get only loaded model children.
/// prim.GetFilteredChildren(UsdPrimIsModel && UsdPrimIsLoaded)
/// \endcode
///
/// For performance, these predicates are implemented by a bitwise test, so
/// arbitrary boolean expressions cannot be represented.  The set of boolean
/// expressions that can be represented are conjunctions with possibly negated
/// terms (or disjunctions, by De Morgan's law).  Here are some examples of
/// valid expressions:
/// \code
/// // simple conjunction.
/// (UsdPrimIsLoaded && UsdPrimIsGroup)
/// // conjunction with negated term.
/// (UsdPrimIsDefined && !UsdPrimIsAbstract)
/// // disjunction with negated term.
/// (!UsdPrimIsDefined || !UsdPrimIsActive)
/// // negated conjunction gives a disjunction.
/// !(UsdPrimIsLoaded && UsdPrimIsModel)
/// // negated conjunction gives a disjunction, which is further extended.
/// (!(UsdPrimIsLoaded && UsdPrimIsModel) || UsdPrimIsAbstract)
/// // equivalent to above.
/// (!UsdPrimIsLoaded || !UsdPrimIsModel || UsdPrimIsAbstract)
/// \endcode
/// Here are some examples of invalid expressions:
/// \code
/// // error: cannot || a term with a conjunction.
/// (UsdPrimIsLoaded && UsdPrimIsModel) || UsdPrimIsAbstract
/// // error: cannot && disjunctions.
/// (!UsdPrimIsDefined || UsdPrimIsAbstract) && (UsdPrimIsModel || !UsdPrimIsActive)
/// \endcode
///
///
/// The following variables provide the clauses that can be combined and 
/// negated to produce predicates:

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/bitUtils.h"

#include <boost/functional/hash.hpp>

#include <bitset>

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;

// Enum for cached flags on prims.
enum Usd_PrimFlags {
    // Flags for use with predicates.
    Usd_PrimActiveFlag,
    Usd_PrimLoadedFlag,
    Usd_PrimModelFlag,
    Usd_PrimGroupFlag,
    Usd_PrimAbstractFlag,
    Usd_PrimDefinedFlag,
    Usd_PrimHasDefiningSpecifierFlag,
    Usd_PrimInstanceFlag,

    // Flags for internal use.
    Usd_PrimHasPayloadFlag,
    Usd_PrimClipsFlag,
    Usd_PrimDeadFlag,
    Usd_PrimMasterFlag,
    Usd_PrimInstanceProxyFlag,

    Usd_PrimNumFlags
};

typedef std::bitset<Usd_PrimNumFlags> Usd_PrimFlagBits;

// Term class.  This class exists merely to allow building up conjunctions or
// disjunctions of terms.  See Usd_PrimFlagsPredicate, Usd_PrimFlagsConjunction,
// Usd_PrimFlagsDisjunction which provide the logcial operators.
struct Usd_Term {
    Usd_Term(Usd_PrimFlags flag) : flag(flag), negated(false) {}
    Usd_Term(Usd_PrimFlags flag, bool negated) : flag(flag), negated(negated) {}
    Usd_Term operator!() const { return Usd_Term(flag, !negated); }
    bool operator==(Usd_Term other) const {
        return flag == other.flag && negated == other.negated;
    }
    bool operator!=(Usd_Term other) const {
        return !(*this == other);
    }
    Usd_PrimFlags flag;
    bool negated;
};

inline Usd_Term
operator!(Usd_PrimFlags flag) {
    return Usd_Term(flag, /*negated=*/true);
}

// Predicate functor class that tests a prim's flags against desired values.
class Usd_PrimFlagsPredicate
{
public:
    // Functor result type.
    typedef bool result_type;

    // Default ctor produces a tautology.
    Usd_PrimFlagsPredicate() : _negate(false) {}

    Usd_PrimFlagsPredicate(Usd_PrimFlags flag)
        : _negate(false) {
        _mask[flag] = 1;
        _values[flag] = true;
    }

    // Implicit conversion from a single term.
    Usd_PrimFlagsPredicate(Usd_Term term)
        : _negate(false) {
        _mask[term.flag] = 1;
        _values[term.flag] = !term.negated;
    }

    // Convenience to produce a tautological predicate.  Returns a
    // default-constructed predicate.
    static Usd_PrimFlagsPredicate Tautology() {
        return Usd_PrimFlagsPredicate();
    }

    // Convenience to produce a contradictory predicate.  Returns a negated
    // default-constructed predicate.
    static Usd_PrimFlagsPredicate Contradiction() {
        return Usd_PrimFlagsPredicate()._Negate();
    }

    // Set flag to indicate whether prim traversal functions using this
    // predicate should traverse beneath instances and return descendants
    // that pass this predicate as instance proxy prims.
    Usd_PrimFlagsPredicate &TraverseInstanceProxies(bool traverse) {
        if (traverse) {
            _mask[Usd_PrimInstanceProxyFlag] = 0;
            _values[Usd_PrimInstanceProxyFlag] = 1;
        }
        else {
            _mask[Usd_PrimInstanceProxyFlag] = 1;
            _values[Usd_PrimInstanceProxyFlag] = 0;
        }
        return *this;
    }

    // Returns true if this predicate was explicitly set to include
    // instance proxies, false otherwise.
    bool IncludeInstanceProxiesInTraversal() const {
        return !_mask[Usd_PrimInstanceProxyFlag] && 
            _values[Usd_PrimInstanceProxyFlag];
    }

    // Invoke boolean predicate on UsdPrim \p prim.
    USD_API
    bool operator()(const class UsdPrim &prim) const;

protected:

    // Return true if this predicate is a tautology, false otherwise.
    bool _IsTautology() const { return *this == Tautology(); }

    // Set this predicate to be a tautology.
    void _MakeTautology() { *this = Tautology(); }

    // Return true if this predicate is a contradiction, false otherwise.
    bool _IsContradiction() const { return *this == Contradiction(); }

    // Set this predicate to be a contradiction.
    void _MakeContradiction() { *this = Contradiction(); }

    // Negate this predicate.
    Usd_PrimFlagsPredicate &_Negate() {
        _negate = !_negate;
        return *this;
    }

    // Return a negated copy of this predicate.
    Usd_PrimFlagsPredicate _GetNegated() const {
        return Usd_PrimFlagsPredicate(*this)._Negate();
    }

    // Mask indicating which flags are of interest.
    Usd_PrimFlagBits _mask;

    // Desired values for prim flags.
    Usd_PrimFlagBits _values;

private:
    // Evaluate this predicate with prim data \p prim. \p isInstanceProxy
    // should be true if this is being evaluated for an instance proxy prim.
    template <class PrimPtr>
    bool _Eval(const PrimPtr &prim, bool isInstanceProxy) const {
        // Manually set the instance proxy bit, since instance proxy
        // state is never stored in Usd_PrimData's flags.
        const Usd_PrimFlagBits primFlags = Usd_PrimFlagBits(prim->_GetFlags())
            .set(Usd_PrimInstanceProxyFlag, isInstanceProxy);

        // Mask the prim's flags, compare to desired values, then optionally
        // negate the result.
        return ((primFlags & _mask) == (_values & _mask)) ^ _negate;
    }

    // Evaluate the predicate \p pred with prim data \p prim. \p isInstanceProxy
    // should be true if this is being evaluated for an instance proxy prim.
    template <class PrimPtr>
    friend bool 
    Usd_EvalPredicate(const Usd_PrimFlagsPredicate &pred, const PrimPtr &prim,
                      bool isInstanceProxy) {
        return pred._Eval(prim, isInstanceProxy);
    }

    // Convenience method for evaluating \p pred using \p prim and 
    // \p proxyPrimPath to determine whether this is for an instance proxy 
    // prim.
    template <class PrimPtr>
    friend bool 
    Usd_EvalPredicate(const Usd_PrimFlagsPredicate &pred, const PrimPtr &prim,
                      const SdfPath &proxyPrimPath) {
        return pred._Eval(prim, Usd_IsInstanceProxy(prim, proxyPrimPath));
    }

    // Equality comparison.
    friend bool
    operator==(const Usd_PrimFlagsPredicate &lhs,
               const Usd_PrimFlagsPredicate &rhs) {
        return lhs._mask == rhs._mask && 
            lhs._values == rhs._values &&
            lhs._negate == rhs._negate;
    }
    // Inequality comparison.
    friend bool
    operator!=(const Usd_PrimFlagsPredicate &lhs,
               const Usd_PrimFlagsPredicate &rhs) {
        return !(lhs == rhs);
    }

    // hash overload.
    friend size_t hash_value(const Usd_PrimFlagsPredicate &p) {
        size_t hash = p._mask.to_ulong();
        boost::hash_combine(hash, p._values.to_ulong());
        boost::hash_combine(hash, p._negate);
        return hash;
    }

    // Whether or not to negate the predicate's result.
    bool _negate;

};


/// Conjunction of prim flag predicate terms.
///
/// Usually clients will implicitly create conjunctions by &&-ing together flag
/// predicate terms.  For example:
/// \code
/// // Get all loaded model children.
/// prim.GetFilteredChildren(UsdPrimIsModel && UsdPrimIsLoaded)
/// \endcode
///
/// See primFlags.h for more details.
class Usd_PrimFlagsConjunction : public Usd_PrimFlagsPredicate {
public:
    /// Default constructed conjunction is a tautology.
    Usd_PrimFlagsConjunction() {};

    /// Construct with a term.
    explicit Usd_PrimFlagsConjunction(Usd_Term term) {
        *this &= term;
    }

    /// Add an additional term to this conjunction.
    Usd_PrimFlagsConjunction &operator&=(Usd_Term term) {
        // If this conjunction is a contradiction, do nothing.
        if (ARCH_UNLIKELY(_IsContradiction()))
            return *this;

        // If we don't have the bit, set it in _mask and _values (if needed).
        if (!_mask[term.flag]) {
            _mask[term.flag] = 1;
            _values[term.flag] = !term.negated;
        } else if (_values[term.flag] != !term.negated) {
            // If we do have the bit and the values disagree, then this entire
            // conjunction becomes a contradiction.  If the values agree, it's
            // redundant and we do nothing.
            _MakeContradiction();
        }
        return *this;
    }

    /// Negate this conjunction, producing a disjunction by De Morgan's law.
    /// For instance:
    ///
    /// \code
    /// !(UsdPrimIsLoaded && UsdPrimIsModel)
    /// \endcode
    ///
    /// Will negate the conjunction in parens to produce a disjunction
    /// equivalent to:
    ///
    /// \code
    /// (!UsdPrimIsLoaded || !UsdPrimIsModel)
    /// \endcode
    ///
    /// Every expression may be formulated as either a disjunction or a
    /// conjuction, but allowing both affords increased expressiveness.
    ///
    USD_API
    class Usd_PrimFlagsDisjunction operator!() const;

private:

    // Let Usd_PrimFlagsDisjunction produce conjunctions when negated
    friend class Usd_PrimFlagsDisjunction;
    Usd_PrimFlagsConjunction(const Usd_PrimFlagsPredicate &base) :
        Usd_PrimFlagsPredicate(base) {}

    /// Combine two terms to make a conjunction.
    friend Usd_PrimFlagsConjunction
    operator&&(Usd_Term lhs, Usd_Term rhs);

    /// Create a new conjunction with the term \p rhs added.
    friend Usd_PrimFlagsConjunction
    operator&&(const Usd_PrimFlagsConjunction &conjunction, Usd_Term rhs);

    /// Create a new conjunction with the term \p lhs added.
    friend Usd_PrimFlagsConjunction
    operator&&(Usd_Term lhs, const Usd_PrimFlagsConjunction &conjunction);
};

inline Usd_PrimFlagsConjunction
operator&&(Usd_Term lhs, Usd_Term rhs) {
    // Apparently gcc 4.8.x doesn't like this as:
    // return (Usd_PrimFlagsConjunction() && lhs) && rhs;
    Usd_PrimFlagsConjunction tmp;
    return (tmp && lhs) && rhs;
}

inline Usd_PrimFlagsConjunction
operator&&(const Usd_PrimFlagsConjunction &conjunction, Usd_Term rhs) {
    return Usd_PrimFlagsConjunction(conjunction) &= rhs;
}

inline Usd_PrimFlagsConjunction
operator&&(Usd_Term lhs, const Usd_PrimFlagsConjunction &conjunction) {
    return Usd_PrimFlagsConjunction(conjunction) &= lhs;
}

inline Usd_PrimFlagsConjunction
operator&&(Usd_PrimFlags lhs, Usd_PrimFlags rhs) {
    return Usd_Term(lhs) && Usd_Term(rhs);
}


/// Disjunction of prim flag predicate terms.
///
/// Usually clients will implicitly create disjunctions by ||-ing together flag
/// predicate terms.  For example:
/// \code
/// // Get all deactivated or undefined children.
/// prim.GetFilteredChildren(!UsdPrimIsActive || !UsdPrimIsDefined)
/// \endcode
///
/// See primFlags.h for more details.
class Usd_PrimFlagsDisjunction : public Usd_PrimFlagsPredicate {
public:
    // Default constructed disjunction is a contradiction.
    Usd_PrimFlagsDisjunction() { _Negate(); };

    // Construct with a term.
    explicit Usd_PrimFlagsDisjunction(Usd_Term term) {
        _Negate();
        *this |= term;
    }

    /// Add an additional term to this disjunction.
    Usd_PrimFlagsDisjunction &operator|=(Usd_Term term) {
        // If this disjunction is a tautology, do nothing.
        if (ARCH_UNLIKELY(_IsTautology()))
            return *this;

        // If we don't have the bit, set it in _mask and _values (if needed).
        if (!_mask[term.flag]) {
            _mask[term.flag] = 1;
            _values[term.flag] = term.negated;
        } else if (_values[term.flag] != term.negated) {
            // If we do have the bit and the values disagree, then this entire
            // disjunction becomes a tautology.  If the values agree, it's
            // redundant and we do nothing.
            _MakeTautology();
        }
        return *this;
    }

    /// Negate this disjunction, producing a disjunction by De Morgan's law.
    /// For instance:
    ///
    /// \code
    /// !(UsdPrimIsLoaded || UsdPrimIsModel)
    /// \endcode
    ///
    /// Will negate the disjunction in parens to produce a conjunction
    /// equivalent to:
    ///
    /// \code
    /// (!UsdPrimIsLoaded && !UsdPrimIsModel)
    /// \endcode
    ///
    /// Every expression may be formulated as either a disjunction or a
    /// conjuction, but allowing both affords increased expressiveness.
    ///
    USD_API
    class Usd_PrimFlagsConjunction operator!() const;

private:

    // Let Usd_PrimFlagsDisjunction produce conjunctions when negated.
    friend class Usd_PrimFlagsConjunction;
    Usd_PrimFlagsDisjunction(const Usd_PrimFlagsPredicate &base) :
        Usd_PrimFlagsPredicate(base) {}

    /// Combine two terms to make a disjunction.
    friend Usd_PrimFlagsDisjunction operator||(Usd_Term lhs, Usd_Term rhs);

    /// Create a new disjunction with the term \p rhs added.
    friend Usd_PrimFlagsDisjunction
    operator||(const Usd_PrimFlagsDisjunction &disjunction, Usd_Term rhs);

    /// Create a new disjunction with the term \p lhs added.
    friend Usd_PrimFlagsDisjunction
    operator||(Usd_Term lhs, const Usd_PrimFlagsDisjunction &disjunction);
};

inline Usd_PrimFlagsDisjunction
operator||(Usd_Term lhs, Usd_Term rhs) {
    return (Usd_PrimFlagsDisjunction() || lhs) || rhs;
}

inline Usd_PrimFlagsDisjunction
operator||(const Usd_PrimFlagsDisjunction &disjunction, Usd_Term rhs) {
    return Usd_PrimFlagsDisjunction(disjunction) |= rhs;
}

inline Usd_PrimFlagsDisjunction
operator||(Usd_Term lhs, const Usd_PrimFlagsDisjunction &disjunction) {
    return Usd_PrimFlagsDisjunction(disjunction) |= lhs;
}

inline Usd_PrimFlagsDisjunction
operator||(Usd_PrimFlags lhs, Usd_PrimFlags rhs) {
    return Usd_Term(lhs) || Usd_Term(rhs);
}

#ifdef doxygen

/// Tests UsdPrim::IsActive()
extern unspecified UsdPrimIsActive;
/// Tests UsdPrim::IsLoaded()
extern unspecified UsdPrimIsLoaded;
/// Tests UsdPrim::IsModel()
extern unspecified UsdPrimIsModel;
/// Tests UsdPrim::IsGroup()
extern unspecified UsdPrimIsGroup;
/// Tests UsdPrim::IsAbstract()
extern unspecified UsdPrimIsAbstract;
/// Tests UsdPrim::IsDefined()
extern unspecified UsdPrimIsDefined;
/// Tests UsdPrim::IsInstance()
extern unspecified UsdPrimIsInstance;
/// Tests UsdPrim::HasDefiningSpecifier()
extern unspecified UsdPrimHasDefiningSpecifier;

/// The default predicate used for prim traversals in methods like
/// UsdPrim::GetChildren, UsdStage::Traverse, and by UsdPrimRange.
/// This is a conjunction that includes all active, loaded, defined, 
/// non-abstract prims, equivalent to:
/// \code
/// UsdPrimIsActive && UsdPrimIsDefined && UsdPrimIsLoaded && !UsdPrimIsAbstract
/// \endcode
///
/// This represents the prims on a stage that a processor would typically 
/// consider present, meaningful, and needful of consideration.
///
/// See \ref Usd_PrimFlags "Prim predicate flags" for more information.
extern unspecified UsdPrimDefaultPredicate;

#else

static const Usd_PrimFlags UsdPrimIsActive = Usd_PrimActiveFlag;
static const Usd_PrimFlags UsdPrimIsLoaded = Usd_PrimLoadedFlag;
static const Usd_PrimFlags UsdPrimIsModel = Usd_PrimModelFlag;
static const Usd_PrimFlags UsdPrimIsGroup = Usd_PrimGroupFlag;
static const Usd_PrimFlags UsdPrimIsAbstract = Usd_PrimAbstractFlag;
static const Usd_PrimFlags UsdPrimIsDefined = Usd_PrimDefinedFlag;
static const Usd_PrimFlags UsdPrimIsInstance = Usd_PrimInstanceFlag;
static const Usd_PrimFlags UsdPrimHasDefiningSpecifier 
    = Usd_PrimHasDefiningSpecifierFlag;

USD_API extern const Usd_PrimFlagsConjunction UsdPrimDefaultPredicate;

#endif // doxygen

/// This function is used to allow the prim traversal functions listed under
/// \ref Usd_PrimFlags "Prim predicate flags" to traverse beneath instance
/// prims and return descendants that pass the specified \p predicate
/// as instance proxy prims.  For example:
///
/// \code
/// // Return all children of the specified prim.  
/// // If prim is an instance, return all children as instance proxy prims.
/// prim.GetFilteredChildren(UsdTraverseInstanceProxies())
///
/// // Return children of the specified prim that pass the default predicate.
/// // If prim is an instance, return the children that pass this predicate
/// // as instance proxy prims.
/// prim.GetFilteredChildren(UsdTraverseInstanceProxies(UsdPrimDefaultPredicate));
///
/// // Return all model or group children of the specified prim.
/// // If prim is an instance, return the children that pass this predicate 
/// // as instance proxy prims.
/// prim.GetFilteredChildren(UsdTraverseInstanceProxies(UsdPrimIsModel || UsdPrimIsGroup));
/// \endcode
///
/// Users may also call Usd_PrimFlagsPredicate::TraverseInstanceProxies to
/// enable traversal beneath instance prims.  This function is equivalent to:
/// \code
/// predicate.TraverseInstanceProxies(true);
/// \endcode
///
/// However, this function may be more convenient, especially when calling
/// a prim traversal function with a default-constructed tautology predicate.
inline Usd_PrimFlagsPredicate
UsdTraverseInstanceProxies(Usd_PrimFlagsPredicate predicate)
{
    return predicate.TraverseInstanceProxies(true);
}

/// \overload
/// Convenience method equivalent to calling UsdTraverseInstanceProxies with a
/// default-constructed tautology predicate.
inline Usd_PrimFlagsPredicate
UsdTraverseInstanceProxies()
{
    return UsdTraverseInstanceProxies(Usd_PrimFlagsPredicate::Tautology());
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_PRIMFLAGS_H
