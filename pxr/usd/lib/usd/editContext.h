//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef USD_EDITCONTEXT_H
#define USD_EDITCONTEXT_H

#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/editTarget.h"
#include "pxr/base/tf/declarePtrs.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <utility>

TF_DECLARE_WEAK_PTRS(UsdStage);

/// \class UsdEditContext
///
/// A utility class to temporarily modify a stage's current EditTarget during
/// an execution scope.
///
/// This is an "RAII"-like object meant to be used as an automatic local
/// variable.  Upon construction, it sets a given stage's EditTarget, and upon
/// destruction it restores the stage's EditTarget to what it was previously.
///
/// Example usage, temporarily overriding a stage's EditTarget to direct an
/// edit to the stage's session layer.  When the \a ctx object expires, it
/// restores the stage's EditTarget to whatever it was previously.
///
/// \code
/// void SetVisState(const UsdPrim &prim, bool vis) {
///     UsdEditContext ctx(prim.GetStage(),
///                        prim.GetStage()->GetSessionLayer());
///     prim.GetAttribute("visible").Set(vis);
/// }
/// \endcode
///
/// <b>Threading Note</b>
///
/// When one thread is mutating a \a UsdStage, it is unsafe for any other thread
/// to either query or mutate it.  Using this class with a stage in such a way
/// that it modifies the stage's EditTarget constitutes a mutation.
///
class UsdEditContext : boost::noncopyable
{
public:
    /// Construct without modifying \a stage's current EditTarget.  Save
    /// \a stage's current EditTarget to restore on destruction.
    USD_API explicit UsdEditContext(const UsdStagePtr &stage);

    /// Construct and save \a stage's current EditTarget to restore on
    /// destruction, then invoke stage->SetEditTarget(editTarget).
    /// 
    /// If \a editTarget is invalid, a coding error will be issued by the
    /// \a stage, and its EditTarget will not be modified.
	USD_API UsdEditContext(const UsdStagePtr &stage, const UsdEditTarget &editTarget);

    /// \overload
    /// This ctor is handy to construct an edit context from the return
    /// value of another function (Cannot return a UsdEditContext since it
    /// needs to be noncopyable).
    /// 
    /// If \a editTarget is invalid, a coding error will be issued by the
    /// \a stage, and its EditTarget will not be modified.
	USD_API UsdEditContext(const std::pair<UsdStagePtr, UsdEditTarget > &stageTarget);

    /// Restore the stage's original EditTarget if this context's stage is
    /// valid.  Otherwise do nothing.
	USD_API ~UsdEditContext();

private:
    // The stage this context is bound to.
    UsdStagePtr _stage;

    // The stage's original EditTarget.
    UsdEditTarget _originalEditTarget;
};

// Utility class for returning UsdEditContexts to python.  For use in wrapping
// code.
struct UsdPyEditContext
{
    USD_API
    explicit UsdPyEditContext(
        const std::pair<UsdStagePtr, UsdEditTarget> &stageTarget);
    USD_API
    explicit UsdPyEditContext(const UsdStagePtr &stage,
                              const UsdEditTarget &editTarget=UsdEditTarget());
private:
    friend struct Usd_PyEditContextAccess;

    UsdStagePtr _stage;
    UsdEditTarget _editTarget;
    boost::shared_ptr<UsdEditContext> _editContext;
};

#endif // USD_EDITCONTEXT_H
