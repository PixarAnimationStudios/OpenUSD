//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_EDIT_CONTEXT_H
#define PXR_USD_USD_EDIT_CONTEXT_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/editTarget.h"
#include "pxr/base/tf/declarePtrs.h"

#include <memory>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE


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
class UsdEditContext
{
    UsdEditContext(UsdEditContext const &) = delete;
    UsdEditContext &operator=(UsdEditContext const &) = delete;
public:
    /// Construct without modifying \a stage's current EditTarget.  Save
    /// \a stage's current EditTarget to restore on destruction.
    ///
    /// If \a stage is invalid, a coding error will be issued by the
    /// constructor, and this class takes no action.
    USD_API
    explicit UsdEditContext(const UsdStagePtr &stage);

    /// Construct and save \a stage's current EditTarget to restore on
    /// destruction, then invoke stage->SetEditTarget(editTarget).
    /// 
    /// If \a stage is invalid, a coding error will be issued by the
    /// constructor, and this class takes no action.
    ///
    /// If \a editTarget is invalid, a coding error will be issued by the
    /// \a stage, and its EditTarget will not be modified.
    USD_API
    UsdEditContext(const UsdStagePtr &stage, const UsdEditTarget &editTarget);

    /// \overload
    /// This ctor is handy to construct an edit context from the return
    /// value of another function (Cannot return a UsdEditContext since it
    /// needs to be noncopyable).
    /// 
    /// If \a stage is invalid, a coding error will be issued by the
    /// constructor, and this class takes no action.
    /// 
    /// If \a editTarget is invalid, a coding error will be issued by the
    /// \a stage, and its EditTarget will not be modified.
    USD_API
    UsdEditContext(const std::pair<UsdStagePtr, UsdEditTarget > &stageTarget);

    /// Restore the stage's original EditTarget if this context's stage is
    /// valid.  Otherwise do nothing.
    USD_API
    ~UsdEditContext();

private:
    // The stage this context is bound to.
    UsdStagePtr _stage;

    // The stage's original EditTarget.
    UsdEditTarget _originalEditTarget;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_EDIT_CONTEXT_H
