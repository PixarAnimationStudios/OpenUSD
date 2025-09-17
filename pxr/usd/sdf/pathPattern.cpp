//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/predicateExpression.h"
#include "pxr/usd/sdf/pathPattern.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include "predicateExpressionParser.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

SdfPathPattern::SdfPathPattern()
    : _isProperty(false)
{
}

SdfPathPattern::SdfPathPattern(SdfPath &&prefix)
{
    SetPrefix(std::move(prefix));
}

SdfPathPattern::SdfPathPattern(SdfPath const &prefix)
    : SdfPathPattern(SdfPath(prefix))
{
}

// private ctor, does no validation.
SdfPathPattern::SdfPathPattern(SdfPath prefix,
                               std::vector<Component> &&components,
                               std::vector<SdfPredicateExpression> &&predExprs,
                               bool isProperty)
    : _prefix(std::move(prefix))
    , _components(std::move(components))
    , _predExprs(std::move(predExprs))
    , _isProperty(isProperty)
{
}

SdfPathPattern const &
SdfPathPattern::Everything()
{
    static SdfPathPattern const *theEverything =
        new SdfPathPattern(SdfPath::AbsoluteRootPath(), {{}}, {}, false);
    return *theEverything;
}

SdfPathPattern const &
SdfPathPattern::EveryDescendant()
{
    static SdfPathPattern const *theEveryDescendant =
        new SdfPathPattern(SdfPath::ReflexiveRelativePath(), {{}}, {}, false);
    return *theEveryDescendant;
}

static inline bool
IsLiteralProperty(std::string const &text)
{
    return SdfPath::IsValidNamespacedIdentifier(text);
}

static inline bool
IsLiteralPrim(std::string const &text)
{
    return SdfPath::IsValidIdentifier(text);
}

bool
SdfPathPattern::CanAppendChild(std::string const &text,
                               SdfPredicateExpression const &predExpr,
                               std::string *reason) const
{
    auto setReason = [reason](auto ...args) {
        if (reason) { *reason = TfStringPrintf(args...); }
    };
    
    if (_isProperty) {
        setReason("Cannot append child '%s' to a property path pattern '%s'",
                  text.c_str(), GetText().c_str());
        return false;
    }

    if (text.empty() && !predExpr && HasTrailingStretch()) {
        setReason("Cannot append stretch to a path pattern that has trailing "
                  "stretch '%s'", GetText().c_str());
        return false;
    }

    return true;
}

SdfPathPattern &
SdfPathPattern::AppendChild(std::string const &text)
{
    return AppendChild(text, {});
}

SdfPathPattern &
SdfPathPattern::AppendChild(std::string const &text,
                            SdfPredicateExpression const &predExpr)
{
    return AppendChild(text, SdfPredicateExpression(predExpr));
}


SdfPathPattern &
SdfPathPattern::AppendChild(std::string const &text,
                            SdfPredicateExpression &&predExpr)
{
    std::string reason;
    if (!CanAppendChild(text, predExpr, &reason)) {
        TF_WARN(reason);
        return *this;
    }
                
    if (_prefix.IsEmpty()) {
        _prefix = SdfPath::ReflexiveRelativePath();
    }
    
    bool isLiteral = IsLiteralPrim(text);
    if ((isLiteral || text == "..") && !predExpr && _components.empty()) {
        _prefix = _prefix.AppendChild(TfToken(text));
    }
    else {
        int predIndex = -1;
        if (predExpr) {
            predIndex = static_cast<int>(_predExprs.size());
            _predExprs.push_back(std::move(predExpr));
        }
        _components.push_back({ text, predIndex, isLiteral });
    }
    return *this;
}

SdfPathPattern &
SdfPathPattern::AppendStretchIfPossible() {
    if (CanAppendChild({})) {
        AppendChild({});
    }
    return *this;
}

bool
SdfPathPattern::HasLeadingStretch() const
{
    return GetPrefix().IsAbsoluteRootPath() &&
        !_components.empty() && _components.front().IsStretch();
}

bool
SdfPathPattern::HasTrailingStretch() const
{
    return !_isProperty &&
        !_components.empty() && _components.back().IsStretch();
}

SdfPathPattern &
SdfPathPattern::RemoveTrailingStretch()
{
    if (HasTrailingStretch()) {
        // We don't have to do all the other work that RemoveTrailingComponent()
        // has to do, since a stretch component doesn't have a predicate, and
        // cannot be a property pattern.
        _components.pop_back();
    }
    return *this;
}

SdfPathPattern &
SdfPathPattern::RemoveTrailingComponent()
{
    if (!_components.empty()) {
        // If this component has a predicate, it must be the last one.
        const int predIndex = _components.back().predicateIndex;
        if (predIndex >= 0) {
            if (TF_VERIFY(
                    static_cast<size_t>(predIndex) == _predExprs.size()-1)) {
                _predExprs.pop_back();
            }
        }
        // If this pattern identified a property, it no longer will.
        if (_isProperty) {
            _isProperty = false;
        }
        // Remove the final component.
        _components.pop_back();
    }
    return *this;
}

bool
SdfPathPattern::CanAppendProperty(std::string const &text,
                                  SdfPredicateExpression const &predExpr,
                                  std::string *reason) const
{
    auto setReason = [reason](auto ...args) {
        if (reason) { *reason = TfStringPrintf(args...); }
    };
    
    if (_isProperty) {
        setReason("Cannot append additional property '%s' to property path "
                  "pattern '%s'", text.c_str(), GetText().c_str());
        return false;
    }

    if (text.empty() && !predExpr) {
        setReason("Cannot append empty property element to path pattern '%s'",
                  GetText().c_str());
        return false;
    }

    return true;
}

SdfPathPattern &
SdfPathPattern::AppendProperty(std::string const &text)
{
    return AppendProperty(text, {});
}

SdfPathPattern &
SdfPathPattern::AppendProperty(std::string const &text,
                               SdfPredicateExpression const &predExpr)
{
    return AppendProperty(text, SdfPredicateExpression(predExpr));
}

SdfPathPattern &
SdfPathPattern::AppendProperty(std::string const &text,
                               SdfPredicateExpression &&predExpr)
{
    std::string reason;
    if (!CanAppendProperty(text, predExpr, &reason)) {
        TF_WARN(reason);
        return *this;
    }

    if (_prefix.IsEmpty()) {
        _prefix = SdfPath::ReflexiveRelativePath();
    }

    bool isLiteral = IsLiteralProperty(text);
    if (isLiteral && !predExpr && _components.empty()) {
        _prefix = _prefix.AppendProperty(TfToken(text));
    }
    else {
        // If this path ends with a stretch component, we have to append a
        // wildcard prim child component first.  That is, AppendProperty('/x//',
        // 'foo') -> '/x//*.foo'.
        if (HasTrailingStretch()) {
            AppendChild("*");
        }
        int predIndex = -1;
        if (predExpr) {
            predIndex = static_cast<int>(_predExprs.size());
            _predExprs.push_back(std::move(predExpr));
        }
        _components.push_back({ text, predIndex, isLiteral });
    }
    _isProperty = true;
    return *this;
}

SdfPathPattern &
SdfPathPattern::SetPrefix(SdfPath &&p)
{
    // If we have any components at all, then p must be a prim path or the
    // absolute root path.  Otherwise it can be a prim or prim property path.
    if (!_components.empty()) {
        if (!p.IsAbsoluteRootOrPrimPath()) {
            TF_WARN("Path patterns with match components require "
                    "prim paths or the absolute root path ('/') as a prefix: "
                    "<%s> -- ignoring.", p.GetAsString().c_str());
            return *this;
        }
    }
    else {
        if (!(p.IsAbsoluteRootOrPrimPath() || p.IsPrimPropertyPath())) { 
            TF_WARN("Path pattern prefixes must be prim paths or prim-property "
                    "paths: <%s> -- ignoring.", p.GetAsString().c_str());
            return *this;
        }
    }
    _prefix = std::move(p);
    if (_components.empty()) {
        _isProperty = _prefix.IsPrimPropertyPath();
    }
    return *this;
}

std::string
SdfPathPattern::GetText() const
{
    std::string result;

    if (_prefix == SdfPath::ReflexiveRelativePath()) {
        if (_components.empty() || _components.front().IsStretch()) {
            // If components is empty, or first component is a stretch, then we
            // emit a leading '.', otherwise nothing.
            result = ".";
        }
    }
    else {
        result = _prefix.GetAsString();
    }

    const bool prefixIsAbsRoot = _prefix == SdfPath::AbsoluteRootPath();
    for (size_t i = 0, end = _components.size(); i != end; ++i) {
        if (_components[i].IsStretch()) {
            result += (i == 0 && prefixIsAbsRoot) ? "/" : "//";
            continue;
        }
        if ((i + 1 == end) && _isProperty) {
            result.push_back('.');
        }
        else if (!result.empty() && result.back() != '/') {
            result.push_back('/');
        }
        result += _components[i].text;
        if (_components[i].predicateIndex != -1) {
            result += "{" + _predExprs[
                _components[i].predicateIndex].GetText() + "}";
        }
    }
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
