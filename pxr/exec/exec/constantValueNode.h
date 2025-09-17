//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_CONSTANT_VALUE_NODE_H
#define PXR_EXEC_EXEC_CONSTANT_VALUE_NODE_H

#include "pxr/pxr.h"

#include "pxr/exec/esf/object.h"
#include "pxr/exec/vdf/node.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

/// A node that computes a constant value.
class Exec_ConstantValueNode final : public VdfNode
{
public:
    /// Create a node that provides the constant \p value.
    Exec_ConstantValueNode(
        VdfNetwork *network,
        const VtValue &value);

    ~Exec_ConstantValueNode() override;

    // VdfNode override
    void Compute(const VdfContext& ctx) const override;

private:
    VtValue _value;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
