//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_FIRST_VALID_INPUT_VALUE_H
#define PXR_EXEC_EF_FIRST_VALID_INPUT_VALUE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/iterator.h"
#include "pxr/exec/vdf/node.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// A function that may be used as a callback (or in a callback) to return the
/// first valid input value.
/// 
/// This will iterate over the inputs in the order they have been registered
/// and return the value of the first valid input, i.e. the first input that
/// provides a value. If no valid input value exists, return the fallback value
/// for TYPE. 
///
template <typename Type>
Type
EfGetFirstValidInputValue(const VdfContext &context)
{
    struct ContextAccess final : public VdfIterator {
        const VdfNode &GetNode(const VdfContext &context) {
            return _GetNode(context);
        }
    };

    // Iterate over all inputs, and return the first valid input value.
    const VdfNode &node = ContextAccess().GetNode(context);
    for (const std::pair<TfToken, VdfInput *> &in : node.GetInputsIterator()) {
        if (const Type *value = context.GetInputValuePtr<Type>(in.first)) {
            return *value;
        }
    }

    // Ask the type registry for the fallback value to use.
    return VdfExecutionTypeRegistry::GetInstance().GetFallback<Type>();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif