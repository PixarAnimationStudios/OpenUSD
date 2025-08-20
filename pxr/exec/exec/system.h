//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_SYSTEM_H
#define PXR_EXEC_EXEC_SYSTEM_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/exec/esf/stage.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class EfTime;
class Exec_Program;
class Exec_RequestImpl;
class Exec_RequestTracker;
class Exec_Runtime;
class ExecValueKey;
class SdfPath;
template <typename> class TfFunctionRef;
template <typename> class TfSpan;
class VdfMaskedOutput;
class VdfRequest;
class VdfSchedule;

/// Base implementation of a system to procedurally compute values based on
/// scene description and computation definitions.
///
/// ExecSystem owns all the structures necessary to compile, schedule and
/// evaluate requested computation values.  Derived classes are responsible
/// for interfacing with the underlying scene description.
///
class ExecSystem
{
public:
    /// Diagnostic utility class.
    class Diagnostics;

protected:
    /// Construct an exec system for computing values on \p stage.
    EXEC_API
    explicit ExecSystem(EsfStage &&stage);

    ExecSystem(const ExecSystem &) = delete;
    ExecSystem& operator=(const ExecSystem &) = delete;

    EXEC_API
    ~ExecSystem();

    /// Changes time on the system.
    /// 
    /// This stores the new time value in the time input node output,
    /// invalidates all time dependent computed values, and notifies requests of
    /// the change in time.
    /// 
    EXEC_API
    void _ChangeTime(const EfTime &time);

    /// Computes the values in the \p computeRequest using the provided
    /// \p schedule.
    /// 
    EXEC_API
    void _Compute(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest);

    /// Invoke \p f on each outstanding exec request.
    ///
    /// \p f is executed with the request tracker mutex held so it must not
    /// re-enter the request tracker.  This method may execute \p f for
    /// multiple requests concurrently.
    ///
    EXEC_API
    void _ParallelForEachRequest(
        TfFunctionRef<void(Exec_RequestImpl&)> f) const;

    /// Derived systems instantiate this class to deliver scene changes to exec.
    class _ChangeProcessor;

private:
    // Requires access to _Compute, _Compile, and _HasPendingRecompilation.
    friend class Exec_RequestImpl;
    std::vector<VdfMaskedOutput> _Compile(TfSpan<const ExecValueKey> valueKeys);

    // Returns true if the program has inputs requiring recompilation.
    EXEC_API
    bool _HasPendingRecompilation() const;

    // Discards all internal state, and constructs new internal data structures
    // leaving the system in the same state as if it was newly constructed.
    // 
    EXEC_API
    void _InvalidateAll();

    // Notifies the system of invalidation due to uncompilation.
    EXEC_API
    void _InvalidateDisconnectedInputs();

    // Notifies the system of authored value invalidation.
    EXEC_API
    void _InvalidateAuthoredValues(TfSpan<const SdfPath> invalidProperties);

private:
    EsfStage _stage;

    std::unique_ptr<Exec_Program> _program;
    std::unique_ptr<Exec_Runtime> _runtime;

    std::unique_ptr<Exec_RequestTracker> _requestTracker;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
