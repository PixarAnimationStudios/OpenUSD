//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTOR_FACTORY_BASE_H
#define PXR_EXEC_VDF_EXECUTOR_FACTORY_BASE_H

///\file

#include "pxr/pxr.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorInterface;
class VdfSpeculationExecutorBase;
class VdfSpeculationNode;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfExecutorFactoryBase
///
/// 
///
class VdfExecutorFactoryBase
{
public:

    /// Manufactures a new executor, which in an executor hierarchy can be
    /// parented underneath the executor owning this factory.
    ///
    virtual std::unique_ptr<VdfExecutorInterface> ManufactureChildExecutor(
        const VdfExecutorInterface *parentExecutor) const = 0;

    /// Manufactures a new speculation executor with the same traits as the
    /// executor owning this factory.
    ///
    virtual std::unique_ptr<VdfSpeculationExecutorBase>
    ManufactureSpeculationExecutor(
        const VdfSpeculationNode *speculationNode,
        const VdfExecutorInterface *parentExecutor) const = 0;

protected:

    /// Protected destructor. We don't allow deletion of instances of this
    /// class via polymorphic base pointers.
    ///
    ~VdfExecutorFactoryBase() = default;

};

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
