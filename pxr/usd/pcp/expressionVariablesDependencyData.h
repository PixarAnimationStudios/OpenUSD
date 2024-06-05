//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_EXPRESSION_VARIABLES_DEPENDENCY_DATA_H
#define PXR_USD_PCP_EXPRESSION_VARIABLES_DEPENDENCY_DATA_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/functionRef.h"

#include <memory>
#include <unordered_set>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(PcpLayerStack);

/// \class PcpExpressionVariablesDependencyData
/// 
/// Captures the expression variables used by an associated prim index
/// during composition.
class PcpExpressionVariablesDependencyData
{
public:
    PCP_API
    PcpExpressionVariablesDependencyData();

    PCP_API
    PcpExpressionVariablesDependencyData(
        PcpExpressionVariablesDependencyData&&);

    PCP_API
    ~PcpExpressionVariablesDependencyData();

    PCP_API
    PcpExpressionVariablesDependencyData& operator=(
        PcpExpressionVariablesDependencyData&&);

    /// Returns true if any dependencies have been recorded, false otherwise.
    PCP_API
    bool IsEmpty() const;

    /// Moves dependencies in \p data and appends it to the dependencies
    /// in this object.
    PCP_API
    void AppendDependencyData(PcpExpressionVariablesDependencyData&& data);

    /// Adds dependencies on the expression variables in \p exprVarDependencies
    /// from \p layerStack.
    PCP_API
    void AddDependencies(
        const PcpLayerStackPtr& layerStack,
        std::unordered_set<std::string>&& exprVarDependencies);

    /// Runs the given \p callback on all of the dependencies in this object.
    /// \p callback must have the signature: 
    /// 
    /// void(const PcpLayerStack&, const std::unordered_set<std::string>&)
    ///
    /// The first argument is the layer stack associated with the expression
    /// variables in the second argument.
    template <class Callback>
    void ForEachDependency(const Callback& callback) const
    {
        _ForEachFunctionRef fn(callback);
        _ForEachDependency(fn);
    }

    /// Returns the expression variable dependencies associated with
    /// \p layerStack. If no such dependencies have been added, returns
    /// nullptr.
    PCP_API
    const std::unordered_set<std::string>*
    GetDependenciesForLayerStack(const PcpLayerStackPtr& layerStack) const;

private:
    using _ForEachFunctionRef = TfFunctionRef<void(
        const PcpLayerStackPtr&, const std::unordered_set<std::string>&)>;

    PCP_API
    void _ForEachDependency(const _ForEachFunctionRef& fn) const;

    class _Data;
    const _Data* _GetData() const;
    _Data& _GetWritableData();

    std::unique_ptr<_Data> _data;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
