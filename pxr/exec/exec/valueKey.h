//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_VALUE_KEY_H
#define PXR_EXEC_EXEC_VALUE_KEY_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/exec/esf/object.h"

#include "pxr/base/tf/token.h"

#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// Specifies a computed value.
///
/// Clients identify computations to evaluate using a scene description
/// object that provides computations and the name of the computation.
/// 
class ExecValueKey
{
public:
    ExecValueKey(EsfObject&& provider, const TfToken& computationName)
        : _provider(std::move(provider))
        , _computationName(computationName)
    {}

    /// Returns the provider object of the requested value.
    const EsfObject& GetProvider() const {
        return _provider;
    }

    /// Returns the name of the requested computation.
    const TfToken& GetComputationName() const {
        return _computationName;
    }

    /// Return a human-readable description of this value key for diagnostic
    /// purposes.
    /// 
    EXEC_API
    std::string GetDebugName() const;

private:
    EsfObject _provider;
    TfToken _computationName;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
