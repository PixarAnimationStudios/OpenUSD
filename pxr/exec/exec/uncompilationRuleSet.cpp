//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/uncompilationRuleSet.h"

#include "pxr/exec/exec/uncompilationTarget.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/exec/esf/editReason.h"

#include <iterator>
#include <sstream>
#include <utility>
#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

std::string
Exec_UncompilationRule::GetDescription() const
{
    const auto visitor = [](const auto &target) -> std::string {
        return target.GetDescription();
    };
    return TfStringPrintf(
        "%s: [%s]",
        std::visit(visitor, target).c_str(),
        reasons.GetDescription().c_str());
}

Exec_UncompilationRuleSet::iterator
Exec_UncompilationRuleSet::erase(
    const Exec_UncompilationRuleSet::iterator &it)
{
    iterator last = std::prev(_rules.end());
    if (it != last) {
        *it = std::move(*last);
    }

    // To handle the case of growing the vector, resize needs a value to
    // initialize new elements. However, we are only shrinking the vector, so
    // this value will never be used.
    _rules.resize(
        _rules.size() - 1,
        Exec_UncompilationRule(
            Exec_NodeUncompilationTarget(0),
            EsfEditReason::None));
    return it;
}

std::string
Exec_UncompilationRuleSet::GetDescription() const
{
    if (_rules.empty()) {
        return "{}";
    }

    std::ostringstream str;
    str << "{\n";
    for (const Exec_UncompilationRule &rule : _rules) {
        str << rule.GetDescription() << '\n';
    }
    str << '}';

    return str.str();
}

PXR_NAMESPACE_CLOSE_SCOPE
