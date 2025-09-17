//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_UNCOMPILER_H
#define PXR_EXEC_EXEC_UNCOMPILER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/esf/editReason.h"

PXR_NAMESPACE_OPEN_SCOPE

class Exec_Program;
class Exec_Runtime;
class Exec_UncompilationRuleSet;
class SdfPath;

/// Performs uncompilation in response to scene changes.
class Exec_Uncompiler
{
public:
    Exec_Uncompiler(Exec_Program *program, Exec_Runtime *runtime)
        : _program(program)
        , _runtime(runtime)
        , _didUncompile(false)
    {}

    /// Deletes portions of the compiled network when the given \p editReasons
    /// have occurred for the scene object at \p path.
    ///
    /// This will look up all relevant rule sets from the program and process
    /// them individually. For recursive resyncs, this includes rules for all
    /// paths descending from \p path.
    ///
    void UncompileForSceneChange(
        const SdfPath &path,
        EsfEditReason editReasons);

    /// Returns `true` if uncompilation resulted in changes to the network.
    bool DidUncompile() const {
        return _didUncompile;
    }

private:
    // Process a single \p ruleSet for \p path that has been changed by the
    // given \p editReasons.
    //
    // Once processed, rules are erased from the \p ruleSet. If rules refer to
    // network objects that no longer exist, those rules are also removed from
    // the \p ruleSet. Rules that do not match the given \p editReasons are
    // skipped.
    // 
    void _ProcessUncompilationRuleSet(
        const SdfPath &path,
        EsfEditReason editReasons,
        Exec_UncompilationRuleSet *ruleSet);

    // These visitor classes perform different actions depending on the type
    // held by an Exec_UncompilationTarget variant.
    class _IsValidTargetVisitor;
    class _UncompileTargetVisitor;

private:
    Exec_Program *_program;
    Exec_Runtime *_runtime;
    bool _didUncompile;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif