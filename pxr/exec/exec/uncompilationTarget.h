//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_UNCOMPILATION_TARGET_H
#define PXR_EXEC_EXEC_UNCOMPILATION_TARGET_H

/// \file
///
/// Exec_UncompilationTargets refer to compiled objects in the exec network that
/// may need to be uncompiled in response to a scene change. Targets may refer
/// nodes or individual inputs of a node. To describe either of these cases,
/// Exec_UncompilationTarget is implemented as a std::variant of
/// Exec_NodeUncompilationTarget and Exec_InputUncompilationTarget.
///

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/base/tf/delegatedCountPtr.h"
#include "pxr/base/tf/token.h"
#include "pxr/exec/vdf/types.h"

#include <atomic>
#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

/// Describes a VdfNode in the network that may later be uncompiled.
///
/// VdfNodes are identified by their VdfIds, which may refer to nodes that no
/// longer exist in the network.
///
class Exec_NodeUncompilationTarget
{
public:
    explicit Exec_NodeUncompilationTarget(const VdfId nodeId)
        : _nodeId(nodeId)
    {}

    /// Returns the id of the targeted node.
    VdfId GetNodeId() const {
        return _nodeId;
    }

    /// Returns a string describing the targeted node.
    EXEC_API
    std::string GetDescription() const;

private:
    VdfId _nodeId;
};

/// Describes a VdfInput in the network that should be uncompiled.
///
/// VdfInputs are identified by the VdfId of their owning node, and an token
/// for the input name on that node. The VdfId may refer to a node that no
/// longer exists in the network.
///
/// The target can be invalidated after it has been uncompiled. This signals
/// other rules for this target to skip uncompiling the input, even if the
/// input still exists in the network.
///
class Exec_InputUncompilationTarget
{
public:
    Exec_InputUncompilationTarget(const VdfId nodeId, const TfToken &inputName)
        : _identity(TfMakeDelegatedCountPtr<_Identity>(nodeId, inputName))
    {}

    /// Returns a pointer to the node's id, or nullptr if this rule has been
    /// moved-from.
    ///
    const VdfId *GetNodeId() const {
        return _identity ? &_identity->nodeId : nullptr;
    }

    /// Returns a pointer to the input's name, or nullptr if this rule has been
    /// moved-from.
    ///
    const TfToken *GetInputName() const {
        return _identity ? &_identity->inputName : nullptr;
    }

    /// Returns true if the target is valid for uncompilation, or false if this
    /// target has been invalidated due to an earlier scene change.
    ///
    bool IsValid() const {
        return _identity && _identity->isValid.load(std::memory_order_relaxed);
    }

    /// Mark this target as invalid, so other rules for the same target do not
    /// attempt to uncompile the same input in a later round of change
    /// processing.
    ///
    void Invalidate() {
        if (_identity) {
            _identity->isValid.store(false, std::memory_order_relaxed);
        }
    }

    /// Returns a string describing the targeted input.
    EXEC_API
    std::string GetDescription() const;

private:
    // Exec_InputUncompilationTarget is implemented as a reference-counted
    // pointer to an _Identity, which identifies the input and stores the
    // tombstone flag.
    struct _Identity
    {
        VdfId nodeId;
        TfToken inputName;
        mutable std::atomic<int> refCount;
        std::atomic<bool> isValid;

        _Identity(const VdfId nodeId_, const TfToken &inputName_)
            : nodeId(nodeId_)
            , inputName(inputName_)
            , refCount(0)
            , isValid(true)
        {}
    };

    friend void TfDelegatedCountIncrement(
        const _Identity *const identity) noexcept {
        identity->refCount.fetch_add(1, std::memory_order_relaxed);
    }

    friend void TfDelegatedCountDecrement(
        const _Identity *const identity) noexcept {
        if (identity->refCount.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete identity;
        }
    }
    
private:
    TfDelegatedCountPtr<_Identity> _identity;
};

/// Describes a network object targeted by an uncompilation rule.
using Exec_UncompilationTarget = std::variant<
    Exec_NodeUncompilationTarget,
    Exec_InputUncompilationTarget>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif