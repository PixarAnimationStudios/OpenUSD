//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/stringUtils.h"

#include <functional>
#include <typeindex>

PXR_NAMESPACE_OPEN_SCOPE

ArResolverContext::_Untyped::~_Untyped() { }

ArResolverContext::ArResolverContext(
    const std::vector<ArResolverContext>& ctxs)
{
    for (const ArResolverContext& ctx : ctxs) {
        _Add(ctx);
    }
}

void
ArResolverContext::_Add(const ArResolverContext& ctx)
{
    for (const auto& obj : ctx._contexts) {
        _Add(std::shared_ptr<_Untyped>(obj->Clone()));
    }
}

bool
ArResolverContext::operator==(const ArResolverContext& rhs) const
{
    if (_contexts.size() != rhs._contexts.size()) {
        return false;
    }
    
    for (size_t i = 0; i < _contexts.size(); ++i) {
        const auto& lhsContext = _contexts[i];
        const auto& rhsContext = rhs._contexts[i];
        if (!lhsContext->IsHolding(rhsContext->GetTypeid()) ||
            !lhsContext->Equals(*rhsContext)) {
            return false;
        }
    }
    
    return true;
}

bool
ArResolverContext::operator<(const ArResolverContext& rhs) const
{
    if (_contexts.size() < rhs._contexts.size()) {
        return true;
    }
    else if (_contexts.size() > rhs._contexts.size()) {
        return false;
    }

    for (size_t i = 0; i < _contexts.size(); ++i) {
        const auto& lhsContext = _contexts[i];
        const auto& rhsContext = rhs._contexts[i];
        if (lhsContext->IsHolding(rhsContext->GetTypeid())) {
            if (lhsContext->LessThan(*rhsContext)) {
                return true;
            }
            else if (!lhsContext->Equals(*rhsContext)) {
                return false;
            }
        }
        else {
            return std::type_index(lhsContext->GetTypeid()) <
                   std::type_index(rhsContext->GetTypeid());
        }
    }
    
    return false;
}

void
ArResolverContext::_Add(std::shared_ptr<_Untyped>&& context)
{
    auto insertIt = std::lower_bound(
        _contexts.begin(), _contexts.end(), context,
        [](const std::shared_ptr<_Untyped>& a,
           const std::shared_ptr<_Untyped>& b) {
            return std::type_index(a->GetTypeid()) < 
                   std::type_index(b->GetTypeid());
        });

    if (insertIt != _contexts.end() && 
        (*insertIt)->IsHolding(context->GetTypeid())) {
        return;
    }

    _contexts.insert(insertIt, std::move(context));
}

std::string
ArResolverContext::GetDebugString() const
{
    std::string s;
    for (const auto& context : _contexts) {
        s += context->GetDebugString();
        s += "\n";
    }
    return s;
}

std::string
Ar_GetDebugString(const std::type_info& info, void const* context) 
{
    return TfStringPrintf("<'%s' @ %p>", 
                          ArchGetDemangled(info).c_str(), context);
}

PXR_NAMESPACE_CLOSE_SCOPE
