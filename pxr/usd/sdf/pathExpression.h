//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PATH_EXPRESSION_H
#define PXR_USD_SDF_PATH_EXPRESSION_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathPattern.h"
#include "pxr/base/tf/hash.h"

#include <iosfwd>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfPathExpression
///
/// Objects of this class represent a logical expression syntax tree consisting
/// of SdfPathPattern s, (with optionally embedded predicate expressions), and
/// Expression References joined by the set-algebraic operators `+` (union), `&`
/// (intersection), `-` (difference), `~` (complement) and an implied-union
/// operator represented by two subexpressions joined by whitespace.
///
/// An SdfPathExpression can be constructed from a string, which will parse the
/// string into an expression object.  The syntax for an expression is as
/// follows:
///
/// The fundamental building blocks are path patterns and expression references.
/// A path pattern is similar to an SdfPath, but it may contain glob-style
/// wild-card characters, embedded brace-enclosed predicate expressions (see
/// SdfPredicateExpression) and `//` elements indicating arbitrary levels of
/// prim hierarchy.  For example, consider
/// <code>/foo//bar*/baz{active:false}</code>.  This pattern matches absolute
/// paths whose first component is `foo`, that also have some descendant prim
/// whose name begins with `bar`, which in turn has a child named `baz` where
/// the predicate `active:false` evaluates to true.
///
/// An expression reference starts with `%` followed by a prim path, a `:`, and
/// a name.  There is also one "special" expression reference, `%_` which means
/// "the weaker" expression when composing expressions together.  See
/// ComposeOver() and ResolveReferences() for more information.
///
/// These building blocks may be joined as mentioned above, with `+`, `-`, `&`,
/// or whitespace, and may be complemented with `~`, and grouped with `(` and
/// `)`.
class SdfPathExpression
{
public:
    using PathPattern = SdfPathPattern;
    
    /// \class ExpressionReference
    ///
    /// Objects of this class represent references to other path expressions,
    /// which will be resolved later by a call to ResolveReferences() or
    /// ComposeOver().
    class ExpressionReference {
    public:
        /// Return the special "weaker" reference, whose syntax in an
        /// SdfPathExpression is "%_".  An ExpressionReference represents this
        /// as the empty \p path, and the name "_".
        SDF_API
        static ExpressionReference const &Weaker();
        
        // Optional path reference, can be empty for "weaker" references (name
        // is "_") or for references to local or otherwise "named" collections.
        SdfPath path;
        
        // Name is either a property name, or "_" (meaning the weaker
        // collection).  If the name is "_", the path must be empty.
        std::string name;

        template <class HashState>
        friend void TfHashAppend(HashState &h, ExpressionReference const &er) {
            h.Append(er.path, er.name);
        }

        friend bool
        operator==(ExpressionReference const &l, ExpressionReference const &r) {
            return std::tie(l.path, l.name) == std::tie(r.path, r.name);
        }
        
        friend bool
        operator!=(ExpressionReference const &l, ExpressionReference const &r) {
            return !(l == r);
        }

        friend void swap(ExpressionReference &l, ExpressionReference &r) {
            auto lt = std::tie(l.path, l.name);
            auto rt = std::tie(r.path, r.name);
            swap(lt, rt);
        }
    };

    /// Enumerant describing a subexpression operation.
    enum Op {
        // Operations on atoms.
        Complement,
        ImpliedUnion,
        Union,
        Intersection,
        Difference,

        // Atoms.
        ExpressionRef,
        Pattern
    };

    /// Default construction produces the "empty" expression.  Conversion to
    /// bool returns 'false'.  The empty expression matches nothing.
    SdfPathExpression() = default;

    /// Construct an expression by parsing \p expr.  If provided, \p
    /// parseContext appears in a parse error, if one is generated.  See
    /// GetParseError().  See the class documentation for details on expression
    /// syntax.
    SDF_API
    explicit SdfPathExpression(std::string const &expr,
                               std::string const &parseContext = {});

    /// Return the expression "//" which matches all paths.
    SDF_API
    static SdfPathExpression const &Everything();

    /// Return the relative expression ".//" which matches all paths descendant
    /// to an anchor path.
    SDF_API
    static SdfPathExpression const &EveryDescendant();

    /// Return the empty expression which matches no paths.  This is the same as
    /// a default-constructed SdfPathExpression.
    SDF_API
    static SdfPathExpression const &Nothing();

    /// Return the expression "%_", consisting solely of a reference to the
    /// "weaker" path expression, to be resolved by ComposeOver() or
    /// ResolveReferences()
    SDF_API
    static SdfPathExpression const &WeakerRef();
    
    /// Produce a new expression representing the set-complement of \p right.
    SDF_API
    static SdfPathExpression
    MakeComplement(SdfPathExpression &&right);

    /// \overload
    static SdfPathExpression
    MakeComplement(SdfPathExpression const &right) {
        return MakeComplement(SdfPathExpression(right));
    }

    /// Produce a new expression representing the set-algebraic operation \p op
    /// with operands \p left and \p right.  The \p op must be one of
    /// ImpliedUnion, Union, Intersection, or Difference.
    SDF_API
    static SdfPathExpression
    MakeOp(Op op, SdfPathExpression &&left, SdfPathExpression &&right);

    /// \overload
    static SdfPathExpression
    MakeOp(Op op,
           SdfPathExpression const &left,
           SdfPathExpression const &right) {
        return MakeOp(op, SdfPathExpression(left), SdfPathExpression(right));
    }

    /// Produce a new expression containing only the reference \p ref.
    SDF_API
    static SdfPathExpression
    MakeAtom(ExpressionReference &&ref);

    /// \overload
    static SdfPathExpression
    MakeAtom(ExpressionReference const &ref) {
        return MakeAtom(ExpressionReference(ref));
    }

    /// Produce a new expression containing only the pattern \p pattern.
    SDF_API
    static SdfPathExpression
    MakeAtom(PathPattern &&pattern);

    /// \overload
    static SdfPathExpression
    MakeAtom(PathPattern const &pattern) {
        return MakeAtom(PathPattern(pattern));
    }

    /// Produce a new expression that matches \p path exactly.
    static SdfPathExpression
    MakeAtom(SdfPath const &path) {
        return MakeAtom(PathPattern(path));
    }

    /// \overload
    static SdfPathExpression
    MakeAtom(SdfPath &&path) {
        return MakeAtom(PathPattern(path));
    }

    /// Walk this expression's syntax tree in depth-first order, calling \p
    /// pattern with the current PathPattern when one is encountered, \p ref
    /// with the current ExpressionReference when one is encountered, and \p
    /// logic multiple times for each logical operation encountered.  When
    /// calling \p logic, the logical operation is passed as the \p Op
    /// parameter, and an integer indicating "where" we are in the set of
    /// operands is passed as the int parameter. For a Complement, call \p
    /// logic(Op=Complement, int=0) to start, then after the subexpression that
    /// the Complement applies to is walked, call \p logic(Op=Complement,
    /// int=1).  For the other operators like Union and Intersection, call \p
    /// logic(Op, 0) before the first argument, then \p logic(Op, 1) after the
    /// first subexpression, then \p logic(Op, 2) after the second
    /// subexpression.  For a concrete example, consider the following
    /// expression:
    ///
    /// /foo/bar// /foo/baz// & ~/foo/bar/qux// %_
    ///
    /// logic(Intersection, 0)
    /// logic(ImpliedUnion, 0)
    /// pattern(/foo/bar//)
    /// logic(ImpliedUnion, 1)
    /// pattern(/foo/baz//)
    /// logic(ImpliedUnion, 2)
    /// logic(Intersection, 1)
    /// logic(ImpliedUnion, 0)
    /// logic(Complement, 0)
    /// pattern(/foo/bar/qux//)
    /// logic(Complement, 1)
    /// logic(ImpliedUnion, 1)
    /// ref(%_)
    /// logic(ImpliedUnion, 2)
    /// logic(Intersection, 2)
    /// 
    SDF_API
    void Walk(TfFunctionRef<void (Op, int)> logic,
              TfFunctionRef<void (ExpressionReference const &)> ref,
              TfFunctionRef<void (PathPattern const &)> pattern) const;

    /// Equivalent to Walk(), except that the \p logic function is called with a
    /// const reference to the current Op stack instead of just the top of it.
    /// The top of the Op stack is the vector's back.  This is useful in case
    /// the processing code needs to understand the context in which an Op
    /// appears.
    SDF_API
    void WalkWithOpStack(
        TfFunctionRef<void (std::vector<std::pair<Op, int>> const &)> logic,
        TfFunctionRef<void (ExpressionReference const &)> ref,
        TfFunctionRef<void (PathPattern const &)> pattern) const;

    /// Return a new expression created by replacing literal path prefixes that
    /// start with \p oldPrefix with \p newPrefix.
    SdfPathExpression
    ReplacePrefix(SdfPath const &oldPrefix,
                  SdfPath const &newPrefix) const & {
        return SdfPathExpression(*this).ReplacePrefix(oldPrefix, newPrefix);
    }

    /// Return a new expression created by replacing literal path prefixes that
    /// start with \p oldPrefix with \p newPrefix.
    SDF_API
    SdfPathExpression
    ReplacePrefix(SdfPath const &oldPrefix,
                  SdfPath const &newPrefix) &&;

    /// Return true if all contained pattern prefixes are absolute, false
    /// otherwise.  Call MakeAbsolute() to anchor any relative paths and make
    /// them absolute.
    SDF_API
    bool IsAbsolute() const;

    /// Return a new expression created by making any relative path prefixes in
    /// this expression absolute by SdfPath::MakeAbsolutePath().
    SdfPathExpression
    MakeAbsolute(SdfPath const &anchor) const & {
        return SdfPathExpression(*this).MakeAbsolute(anchor);
    }

    /// Return a new expression created by making any relative path prefixes in
    /// this expression absolute by SdfPath::MakeAbsolutePath().
    SDF_API
    SdfPathExpression
    MakeAbsolute(SdfPath const &anchor) &&;

    /// Return true if this expression contains any references to other
    /// collections.
    bool ContainsExpressionReferences() const {
        return !_refs.empty();
    }

    /// Return true if this expression contains one or more "weaker" expression
    /// references, expressed as '%_' in the expression language.  Return false
    /// otherwise.
    SDF_API
    bool ContainsWeakerExpressionReference() const;

    /// Return a new expression created by resolving collection references in
    /// this expression. This function calls \p resolve to produce a
    /// subexpression from a "%" ExpressionReference. To leave an expression
    /// reference unchanged, return an expression containing the passed argument
    /// by calling MakeAtom().
    SdfPathExpression
    ResolveReferences(
        TfFunctionRef<SdfPathExpression (
                          ExpressionReference const &)> resolve) const & {
        return SdfPathExpression(*this).ResolveReferences(resolve);
    }

    /// \overload
    SDF_API
    SdfPathExpression
    ResolveReferences(
        TfFunctionRef<SdfPathExpression (
                          ExpressionReference const &)> resolve) &&;
    
    /// Return a new expression created by replacing references to the "weaker
    /// expression" (i.e. "%_") in this expression with \p weaker.  This is a
    /// restricted form of ResolveReferences() that only resolves "weaker"
    /// references, replacing them by \p weaker, leaving other references
    /// unmodified.  As a special case, if this expression IsEmpty(), return \p
    /// weaker.
    SdfPathExpression
    ComposeOver(SdfPathExpression const &weaker) const & {
        return SdfPathExpression(*this).ComposeOver(weaker);
    }

    /// \overload
    SDF_API
    SdfPathExpression
    ComposeOver(SdfPathExpression const &weaker) &&;

    /// Return true if this expression is considered "complete".  Here, complete
    /// means that the expression has all absolute paths, and contains no
    /// expression references.  This is equivalent to:
    ///
    /// \code
    /// !expr.ContainsExpressionReferences() && expr.IsAbsolute()
    /// \endcode
    ///
    /// To complete an expression, call MakeAbsolute(), ResolveReferences()
    /// and/or ComposeOver().
    bool IsComplete() const {
        return !ContainsExpressionReferences() && IsAbsolute();
    }

    /// Return a text representation of this expression that parses to the same
    /// expression.
    SDF_API
    std::string GetText() const;

    /// Return true if this is the empty expression; i.e. default-constructed or
    /// constructed from a string with invalid syntax.
    bool IsEmpty() const {
        return _ops.empty();
    }
    
    /// Return true if this expression contains any operations, false otherwise.
    explicit operator bool() const {
        return !IsEmpty();
    }

    /// Return parsing errors as a string if this function was constructed from
    /// a string and parse errors were encountered.
    std::string const &GetParseError() const & {
        return _parseError;
    }

private:
    template <class HashState>
    friend void TfHashAppend(HashState &h, SdfPathExpression const &expr) {
        h.Append(expr._ops, expr._refs, expr._patterns, expr._parseError);
    }

    SDF_API
    friend std::ostream &
    operator<<(std::ostream &, SdfPathExpression const &);

    friend bool
    operator==(SdfPathExpression const &l, SdfPathExpression const &r) {
        return std::tie(l._ops, l._refs, l._patterns, l._parseError) ==
               std::tie(r._ops, r._refs, r._patterns, r._parseError);
    }

    friend bool
    operator!=(SdfPathExpression const &l, SdfPathExpression const &r) {
        return !(l == r);
    }

    friend void swap(SdfPathExpression &l, SdfPathExpression &r) {
        auto lt = std::tie(l._ops, l._refs, l._patterns, l._parseError);
        auto rt = std::tie(r._ops, r._refs, r._patterns, r._parseError);
        swap(lt, rt);
    }

    std::vector<Op> _ops;
    std::vector<ExpressionReference> _refs;
    std::vector<PathPattern> _patterns;

    // This member holds a parsing error string if this expression was
    // constructed by the parser and errors were encountered during the parsing.
    std::string _parseError;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PATH_EXPRESSION_H
