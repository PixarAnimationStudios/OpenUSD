//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_ATTRIBUTE_INPUT_NODE_H
#define PXR_EXEC_EXEC_ATTRIBUTE_INPUT_NODE_H

#include "pxr/pxr.h"

#include "pxr/exec/esf/attributeQuery.h"
#include "pxr/exec/vdf/node.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/ts/spline.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

class EfTime;

#define EXEC_ATTRIBUTE_INPUT_NODE_TOKENS  \
    (time)

TF_DECLARE_PUBLIC_TOKENS(
    Exec_AttributeInputNodeTokens, EXEC_ATTRIBUTE_INPUT_NODE_TOKENS);

/// Node that computes attribute resolved values.
class Exec_AttributeInputNode final : public VdfNode
{
public:
    /// Create a node that provides the resolved value through \p attributeQuery
    /// at the current evaluation time.
    ///
    Exec_AttributeInputNode(
        VdfNetwork *network,
        EsfAttributeQuery &&attributeQuery,
        TfType valueType);

    ~Exec_AttributeInputNode() override;

    /// Update the internal state to ensure that resolved values are sourced
    /// correctly.
    /// 
    /// Where resolved values for the corresponding attribute come from can be
    /// affected by scene changes, such as info changes.
    /// 
    void UpdateValueResolutionState();

    /// Returns the scene path to the attribute that the input value is sourced
    /// from.
    /// 
    SdfPath GetAttributePath() const {
        return _attributeQuery->GetPath();
    }

    /// Updates the input's time dependence.
    ///
    /// This queries the corresponding attribute to determine whether it is
    /// time dependent and returns `true` if there was a change in time
    /// dependence.
    ///
    bool UpdateTimeDependence();

    /// Returns `true` if the input is time dependent.
    bool IsTimeDependent() const {
        return _isTimeDependent;
    }

    /// Returns `true` if the resolved input value at time \p from is different
    /// from the value at time \p to.
    /// 
    bool IsTimeVarying(const EfTime &from, const EfTime &to) const;

    /// Returns the corresponding attribute's spline, if the strongest opinion
    /// resolves to a spline.
    /// 
    std::optional<TsSpline> GetSpline() const {
        return _attributeQuery->GetSpline();
    }

    /// VdfNode::Compute() override.
    void Compute(VdfContext const& ctx) const override;

private:
    // Computes dependencies in the output-to-input traversal direction.
    VdfMask::Bits _ComputeInputDependencyMask(
        const VdfMaskedOutput &maskedOutput,
        const VdfConnection &inputConnection) const override;

    // Computes dependencies in the input-to-output traversal direction.
    VdfMask _ComputeOutputDependencyMask(
        const VdfConnection &inputConnection,
        const VdfMask &inputDependencyMask,
        const VdfOutput &output) const override;

private:
    // TODO: Once we stop treating namespace edits as resyncs, we will need to
    // re-initialize the attribute query in response to edits like rename and
    // reparent.
    EsfAttributeQuery _attributeQuery;
    bool _isTimeDependent;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
