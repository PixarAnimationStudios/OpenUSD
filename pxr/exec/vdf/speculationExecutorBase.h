//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SPECULATION_EXECUTOR_BASE_H
#define PXR_EXEC_VDF_SPECULATION_EXECUTOR_BASE_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/executorInterface.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfSpeculationExecutor
///
/// \brief A common base class for all speculation executors.
///
class VDF_API_TYPE VdfSpeculationExecutorBase : public VdfExecutorInterface
{
public:

    /// Destructor.
    ///
    VDF_API
    virtual ~VdfSpeculationExecutorBase();

    /// Returns true if the given node is a not that this executor (or any
    /// one of its parents) is speculating about
    ///
    inline bool IsSpeculatingNode(const VdfNode *node) const;

    /// Returns the first executor in the executor hierarchy that is NOT a
    /// speculation executor.
    ///
    const VdfExecutorInterface *GetNonSpeculationParentExecutor() const {
        return _parentNonSpeculationExecutor;
    }

protected:

    /// Constructor.
    ///
    VDF_API
    explicit VdfSpeculationExecutorBase(
        const VdfExecutorInterface *parentExecutor);

    /// Set this executors speculating node.
    ///
    void _SetSpeculationNode(const VdfNode *speculationNode) {
        _speculationNode = speculationNode;
    }

private:

    // The speculation node using this executor.
    const VdfNode *_speculationNode;

    // The parent speculation executor.
    const VdfSpeculationExecutorBase *_parentSpeculationExecutor;

    // The first parent executor that is not a speculation executor.
    const VdfExecutorInterface *_parentNonSpeculationExecutor;

};

///////////////////////////////////////////////////////////////////////////////

bool
VdfSpeculationExecutorBase::IsSpeculatingNode(const VdfNode *node) const
{
    bool isSpeculatingNode = false;
    const VdfSpeculationExecutorBase *speculationExecutor = this;
    while (speculationExecutor) {
        isSpeculatingNode |= (node == speculationExecutor->_speculationNode);
        speculationExecutor = speculationExecutor->_parentSpeculationExecutor;
    }
    return isSpeculatingNode;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
