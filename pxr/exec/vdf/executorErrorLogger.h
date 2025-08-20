//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTOR_ERROR_LOGGER_H
#define PXR_EXEC_VDF_EXECUTOR_ERROR_LOGGER_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include <tbb/concurrent_unordered_map.h>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

/// \file

class VdfNode;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfExecutorErrorLogger
///
/// A client may instantiate an object of this class and set it in an executor,
/// to collect errors that may have been reported by nodes during a call to
/// Run() on the executor.
///
/// Since since API is used by executors it is partly thread-safe as noted
/// below.
///
class VdfExecutorErrorLogger 
{
public:

    /// Ctor.
    ///
    VDF_API
    VdfExecutorErrorLogger();

    /// Dtor.
    ///
    VDF_API
    ~VdfExecutorErrorLogger();

    /// A thread safe map from node pointer to string for logging warnings.
    ///    
    using NodeToStringMap =
        tbb::concurrent_unordered_map<const VdfNode *, std::string>;

    /// \name Accessors API
    /// @{

    /// Returns a map that maps nodes to warning strings that were
    /// encountered during a call to Run().
    ///
    VDF_API
    const NodeToStringMap &GetWarnings() const;

    /// @}
    

    /// \name Reporting API
    /// @{

    /// Reports warnings using node debug names.  Usually a client will
    /// want to use GetWarnings() and report more meaningful messages.
    ///
    VDF_API
    void ReportWarnings() const;

    /// Prints out default warning message based on \p node's debug name.
    ///
    VDF_API
    static void IssueDefaultWarning(
        const VdfNode &node,
        const std::string &warning);

    /// @}


    /// \name Logging API
    /// @{

    /// Logs a warning against \p node.  Concatenates existing warnings, if any.
    /// This is thread-safe.
    ///
    VDF_API
    void LogWarning(const VdfNode &node, const std::string &warning) const;

    /// @}

private:

    // Holds warnings emitted by nodes during execution.
    mutable std::atomic<NodeToStringMap *> _warnings;
};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
