//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_ITERATOR_H
#define PXR_EXEC_VDF_ITERATOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;
class VdfConnection;
class VdfExecutorInterface;
class VdfMask;
class VdfNode;
class VdfOutput;
class VdfVector;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfIterator
///
/// Base class for libVdf iterators.  Iterators can derive from this class
/// to have access to restricted API from the VdfContext.
///
class VdfIterator
{
protected:

    /// Disallow destruction via base class pointers.
    ///
    ~VdfIterator() = default;

    /// Returns the current node being run.
    ///
    const VdfNode &_GetNode(const VdfContext &context) const {
        return context._GetNode();
    }

    /// Returns the executor for the given context.
    ///
    const VdfExecutorInterface &_GetExecutor(const VdfContext &context) const {
        return context._GetExecutor();
    }

    /// Returns a vector for reading an input value. This will return \c NULL
    /// if no value is flowing across the given \p connection, or if the data
    /// requested with \p mask is unavailable.
    ///
    VDF_API
    const VdfVector *_GetInputValue(
        const VdfContext &context,
        const VdfConnection &connection,
        const VdfMask &mask) const;

    /// Returns the cached output value for a given \p output, or issues an
    /// error message if a cache value is not available.
    ///
    VDF_API
    const VdfVector &_GetRequiredInputValue(
        const VdfContext &context,
        const VdfConnection &connection,
        const VdfMask &mask) const;

    /// Returns the output for writing based on the \p name provided. This
    /// returns the associated output of the input named \p name, if it
    /// exists. Otherwise, returns the output name \p name. If \p name is the
    /// empty token, returns the single output on the current node.
    /// Issues a coding error if the required output does not exist.
    ///
    VDF_API
    const VdfOutput *_GetRequiredOutputForWriting(
        const VdfContext &context,
        const TfToken &name) const;

    /// Returns a vector for writing an output value into. Note that if this
    /// method returns \c NULL a data entry has not been created for the given
    /// \p output. This is not necessarily an error condition, if for example
    /// the \p output is simply not scheduled.
    ///
    VDF_API
    VdfVector *_GetOutputValueForWriting(
        const VdfContext &context,
        const VdfOutput &output) const;

    /// Retrieves the request and affects masks of the given \p output. Note,
    /// that output must be an output on the current node. Returns \c true if
    /// the output is scheduled, and \c false otherwise.
    ///
    /// The \p requestMask is the mask of the elements requested of a
    /// particular output.
    ///
    /// The \p affectsMask is the mask of the elements that are potentially 
    /// going to be modified by a particular output.
    ///
    VDF_API
    bool _GetOutputMasks(
        const VdfContext &context,
        const VdfOutput &output,
        const VdfMask **requestMask,
        const VdfMask **affectsMask) const;

    /// Returns \c true when the \p connection is scheduled and required,
    /// and \c false otherwise.  Used by special iterators to avoid computing
    /// outputs that aren't necessary.
    ///
    VDF_API
    bool _IsRequiredInput(
        const VdfContext &context,
        const VdfConnection &connection) const;

    /// Returns the request mask of \p output, if the output has been scheduled
    /// and \c NULL otherwise.
    /// 
    VDF_API
    const VdfMask *_GetRequestMask(
        const VdfContext &context,
        const VdfOutput &output) const;

    /// Loops over each scheduled output of \p node and calls \p callback 
    /// with the output and request mask in an efficient manner.
    /// 
    VDF_API
    void _ForEachScheduledOutput(
        const VdfContext &context,
        const VdfNode &node,
        const VdfScheduledOutputCallback &callback) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
