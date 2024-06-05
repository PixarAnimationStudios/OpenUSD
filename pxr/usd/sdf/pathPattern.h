//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PATH_PATTERN_H
#define PXR_USD_SDF_PATH_PATTERN_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/predicateExpression.h"
#include "pxr/base/tf/hash.h"

#include <iosfwd>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfPathPattern
///
/// Objects of this class represent SdfPath matching patterns, consisting of
/// an SdfPath prefix followed by a sequence of components, which may
/// contain wildcards and optional embedded predicate expressions (see
/// SdfPredicateExpression).
class SdfPathPattern
{
public:
    /// Construct the empty pattern whose bool-conversion operator returns
    /// false.
    SDF_API
    SdfPathPattern();

    /// Construct a SdfPathPattern with the \p prefix path.
    SDF_API
    explicit SdfPathPattern(SdfPath const &prefix);
    
    /// Construct a SdfPathPattern with the \p prefix path.
    SDF_API
    explicit SdfPathPattern(SdfPath &&prefix);

    /// Return the pattern "//" which matches all paths.
    SDF_API
    static SdfPathPattern const &Everything();

    /// Return the pattern ".//" which matches all paths descendant to an anchor
    /// path.
    SDF_API
    static SdfPathPattern const &EveryDescendant();

    /// Return a default constructed SdfPathPattern that matches nothing.
    static SdfPathPattern Nothing() {
        return {};
    }

    /// A component represents a pattern matching component past the initial
    /// SdfPath prefix.  A component's text can contain wildcard characters, and
    /// if the component references a predicate expression, its predicateIndex
    /// indicates which one in the owning SdfPathPattern's list of expressions.
    /// A component that returns true for IsStretch() represents an "arbitrary
    /// levels of hierarchy" element (the "//") in a path pattern.
    struct Component {
        bool IsStretch() const {
            return predicateIndex == -1 && text.empty();
        }

        std::string text;
        int predicateIndex = -1;
        bool isLiteral = false;

        friend bool operator==(Component const &l, Component const &r) {
            return std::tie(l.text, l.predicateIndex, l.isLiteral) ==
                std::tie(r.text, r.predicateIndex, r.isLiteral);
        }
        
        friend bool operator!=(Component const &l, Component const &r) {
            return !(l == r);
        }

        template <class HashState>
        friend void TfHashAppend(HashState &h, Component const &c) {
            h.Append(c.text, c.predicateIndex, c.isLiteral);
        }

        friend void swap(Component &l, Component &r) {
            auto lt = std::tie(l.text, l.predicateIndex, l.isLiteral);
            auto rt = std::tie(r.text, r.predicateIndex, r.isLiteral);
            swap(lt, rt);
        }
    };

    /// Return true if it is valid to append the child element \p text to this
    /// pattern.  If not, and \p reason is not nullptr, set it to an explanatory
    /// reason why not.
    bool CanAppendChild(std::string const &text,
                        std::string *reason = nullptr) const {
        return CanAppendChild(text, {}, reason);
    }

    /// Return true if it is valid to append the child element \p text and \p
    /// predExpr to this pattern.  If not, and \p reason is not nullptr, set it
    /// to an explanatory reason why not.
    SDF_API
    bool CanAppendChild(std::string const &text,
                        SdfPredicateExpression const &predExpr,
                        std::string *reason = nullptr) const;

    /// Append a prim child component to this pattern, with optional
    /// predicate expression \p predExpr.  If this pattern does not yet
    /// contain any wildcards or components with predicate expressions, and
    /// the input text does not contain wildcards, and \p predExpr is empty,
    /// then append a child component to this pattern's prefix path (see
    /// GetPrefix()).  Otherwise append this component to the sequence of
    /// components.  Return *this.
    SDF_API
    SdfPathPattern &AppendChild(std::string const &text,
                                SdfPredicateExpression &&predExpr);
    /// \overload
    SDF_API
    SdfPathPattern &AppendChild(std::string const &text,
                                SdfPredicateExpression const &predExpr);
    /// \overload
    SDF_API
    SdfPathPattern &AppendChild(std::string const &text);

    /// Return true if it is valid to append the property element \p text to
    /// this pattern.  If not, and \p reason is not nullptr, set it to an
    /// explanatory reason why not.
    bool CanAppendProperty(std::string const &text,
                           std::string *reason = nullptr) const {
        return CanAppendProperty(text, {}, reason);
    }

    /// Return true if it is valid to append the property element \p text and \p
    /// predExpr to this pattern.  If not, and \p reason is not nullptr, set it
    /// to an explanatory reason why not.
    SDF_API
    bool CanAppendProperty(std::string const &text,
                           SdfPredicateExpression const &predExpr,
                           std::string *reason = nullptr) const;

    /// Append a prim property component to this pattern, with optional
    /// predicate expression \p predExpr.  If this pattern does not yet
    /// contain any wildcards or components with predicate expressions, and
    /// the input text does not contain wildcards, and \p predExpr is empty,
    /// then append a property component to this pattern's prefix path (see
    /// GetPrefix()). Otherwise append this component to the sequence of
    /// components.  Return *this.
    SDF_API
    SdfPathPattern &AppendProperty(std::string const &text,
                                   SdfPredicateExpression &&predExpr);
    /// \overload
    SDF_API
    SdfPathPattern &AppendProperty(std::string const &text,
                                   SdfPredicateExpression const &predExpr);
    /// \overload
    SDF_API
    SdfPathPattern &AppendProperty(std::string const &text);

    /// Return this pattern's non-speculative prefix (leading path
    /// components with no wildcards and no predicates).
    SdfPath const &GetPrefix() const & {
        return _prefix;
    }

    /// \overload
    SdfPath GetPrefix() && {
        return std::move(_prefix);
    }

    /// Set this pattern's non-speculative prefix (leading path
    /// components with no wildcards and no predicates).  Return *this.
    SDF_API
    SdfPathPattern &SetPrefix(SdfPath &&p);

    /// \overload
    SdfPathPattern &SetPrefix(SdfPath const &p) {
        return SetPrefix(SdfPath(p));
    }

    /// Return true if this pattern's prefix is the absolute root path and and
    /// its first component is a stretch component -- that is, the pattern
    /// starts with `//`, false otherwise.
    SDF_API
    bool HasLeadingStretch() const;

    /// Return true if this pattern ends with a stretch component: `//`, false
    /// otherwise.
    SDF_API
    bool HasTrailingStretch() const;

    /// Append a stretch component (i.e. `//`) to this pattern if it's possible
    /// to do so.  Otherwise do nothing.  It is not possible to append a stretch
    /// component to a pattern that already ends in a stretch component, or a
    /// pattern that identifies a property.  Return *this.
    SDF_API
    SdfPathPattern &AppendStretchIfPossible();

    /// Remove trailing stretch from this pattern if it has trailing stretch.
    /// Otherwise do nothing.  Return *this.  See HasTrailingStretch().
    SDF_API
    SdfPathPattern &RemoveTrailingStretch();

    /// If this pattern has components, remove the final component.  Otherwise
    /// do nothing.  Return *this.  To inspect and modify the prefix path, use
    /// GetPrefix(), SetPrefix().
    SDF_API
    SdfPathPattern &RemoveTrailingComponent();

    /// Return the string representation of this pattern.
    SDF_API
    std::string GetText() const;

    /// Return this pattern's components that follow its non-speculative prefix
    /// path.
    std::vector<Component> const &GetComponents() const & {
        return _components;
    }

    /// \overload
    std::vector<Component> GetComponents() && {
        return std::move(_components);
    }

    /// Return the predicate expressions used by this pattern.  These are
    /// indexed by a Component's predicateIndex member, if it is not -1.
    std::vector<SdfPredicateExpression> const &
    GetPredicateExprs() const & {
        return _predExprs;
    }

    /// \overload
    std::vector<SdfPredicateExpression>
    GetPredicateExprs() && {
        return std::move(_predExprs);
    }

    /// Return true if this pattern identifies properties exclusively, false
    /// otherwise.
    bool IsProperty() const {
        return _isProperty;
    }

    /// Return true if this pattern is not empty, false if it is.
    explicit operator bool() const {
        return !_prefix.IsEmpty();
    }

private:
    SdfPathPattern(SdfPath prefix,
                   std::vector<Component> &&components,
                   std::vector<SdfPredicateExpression> &&predExprs,
                   bool isProperty);
    
    template <class HashState>
    friend void TfHashAppend(HashState &h, SdfPathPattern const &pat) {
        h.Append(pat._prefix, pat._components,
                 pat._predExprs, pat._isProperty);
    }

    friend bool
    operator==(SdfPathPattern const &l, SdfPathPattern const &r) {
        return std::tie(l._prefix, l._components,
                        l._predExprs, l._isProperty) ==
               std::tie(r._prefix, r._components,
                        r._predExprs, r._isProperty);
    }

    friend bool
    operator!=(SdfPathPattern const &l, SdfPathPattern const &r) {
        return !(l == r);
    }

    friend void swap(SdfPathPattern &l, SdfPathPattern &r) {
        auto lt = std::tie(
            l._prefix, l._components, l._predExprs, l._isProperty);
        auto rt = std::tie(
            r._prefix, r._components, r._predExprs, r._isProperty);
        swap(lt, rt);
    }
    
    SdfPath _prefix;
    std::vector<Component> _components;
    std::vector<SdfPredicateExpression> _predExprs;
    bool _isProperty;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PATH_PATTERN_H
