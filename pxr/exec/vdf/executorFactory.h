//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTOR_FACTORY_H
#define PXR_EXEC_VDF_EXECUTOR_FACTORY_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/executorFactoryBase.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorInterface;
class VdfSpeculationExecutorBase;
class VdfSpeculationNode;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfExecutorFactory
///
/// 
///
template <
    typename ChildExecutorType,
    typename SpeculationExecutorType
    >
class VdfExecutorFactory : public VdfExecutorFactoryBase
{
public:

    /// Manufactures a new ChildExecutorType, which in an executor hierarchy
    /// can be parented underneath the executor owning this factory.
    ///
    virtual std::unique_ptr<VdfExecutorInterface>
    ManufactureChildExecutor(
        const VdfExecutorInterface *parentExecutor) const override final {
        return std::unique_ptr<VdfExecutorInterface>(
            new ChildExecutorType(parentExecutor));
    }

    /// Manufactures a new SpeculationExecutorType with the same traits as the
    /// executor owning this factory.
    ///
    virtual std::unique_ptr<VdfSpeculationExecutorBase>
    ManufactureSpeculationExecutor(
        const VdfSpeculationNode *speculationNode,
        const VdfExecutorInterface *parentExecutor) const override final {
        return std::unique_ptr<VdfSpeculationExecutorBase>(
            new SpeculationExecutorType(speculationNode, parentExecutor));
    }

};

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif