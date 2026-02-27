//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_INTERRUPT_STATE_H
#define PXR_EXEC_EXEC_INTERRUPT_STATE_H

/// \file

#include "pxr/pxr.h"

#include <tbb/concurrent_vector.h>

#include <atomic>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

class Exec_CompilationState;
class VdfInput;
class VdfNode;

/// Maintains state related to the interruption of a round of compilation.
///
/// An instance of this class is owned by the Exec_CompilationState for a given
/// round of compilation. When task cycles are detected, exec will interrupt the
/// compilation process by invoking the Interrupt() method of this class.
///
/// This class also tracks exec network objects that are affected by the
/// interruption. This includes inputs that need to be recompiled in the next
/// round because they never finished recompilation, and nodes that are
/// potentially isolated because their downstream nodes were never compiled.
///
class Exec_InterruptState
{
public:
    explicit Exec_InterruptState(Exec_CompilationState *compilationState);

    Exec_InterruptState(const Exec_InterruptState &) = delete;
    Exec_InterruptState &operator=(const Exec_InterruptState &) = delete;

    /// Interrupts the current round of compilation.
    void Interrupt();

    /// Returns true if the current round of compilation was interrupted.
    bool WasInterrupted() const {
        return _wasInterrupted.load(std::memory_order_relaxed);
    }

    /// Stores into \p out the set of nodes that are potentially isolated
    /// because the interrupt prevented their downstream nodes from being
    /// compiled.
    ///
    void GetPotentiallyIsolatedNodes(
        std::unordered_set<VdfNode *> *out) const;

    /// Stores into \p out the set of inputs that could not complete
    /// recompilation due to the interrupt.
    ///
    void GetInputsRequiringRecompilation(
        std::unordered_set<VdfInput *> *out) const;

    /// Records that \p node is potentially isolated, because the interruption
    /// prevented the compilation of consuming nodes.
    ///
    void AddPotentiallyIsolatedNode(VdfNode *node);

    /// Records that the interruption prevented \p input from completing
    /// recompilation.
    ///
    void AddInputRequiringRecompilation(VdfInput *input);

private:
    // The set of potentially isolated nodes is stored as a vector to make
    // insertions fast. Any duplicates will be removed by
    // GetPotentiallyIsolatedNodes.
    tbb::concurrent_vector<VdfNode *> _potentiallyIsolatedNodes;

    // The set of inputs requiring recompilation is stored as a vector to make
    // insertions fast. Any duplicates will be removed by
    // GetInputsRequiringRecompilation, but in practice there are no duplicates.
    tbb::concurrent_vector<VdfInput *> _inputsRequiringRecompilation;

    // A back-pointer to the current compilation state.
    Exec_CompilationState *const _compilationState;

    // True if the current round of compilation has been interrupted.
    std::atomic<bool> _wasInterrupted;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif