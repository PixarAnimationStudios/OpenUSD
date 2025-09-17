//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_INHERITS_H
#define PXR_USD_USD_INHERITS_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfPrimSpec);

/// \class UsdInherits
///
/// A proxy class for applying listOp edits to the inherit paths list for a
/// prim.
///
/// All paths passed to the UsdInherits API are expected to be in the 
/// namespace of the owning prim's stage. Subroot prim inherit paths
/// will be translated from this namespace to the  namespace of the current
/// edit target, if necessary. If a path cannot be translated, a coding error 
/// will be issued and no changes will be made. Root prim inherit paths will 
/// not be translated.
///
class UsdInherits {
    friend class UsdPrim;

    explicit UsdInherits(const UsdPrim& prim) : _prim(prim) {}

public:
    /// Adds a path to the inheritPaths listOp at the current EditTarget,
    /// in the position specified by \p position.
    USD_API
    bool AddInherit(const SdfPath &primPath,
                    UsdListPosition position=UsdListPositionBackOfPrependList);

    /// Removes the specified path from the inheritPaths listOp at the
    /// current EditTarget.
    USD_API
    bool RemoveInherit(const SdfPath &primPath);

    /// Removes the authored inheritPaths listOp edits at the current edit
    /// target.
    USD_API
    bool ClearInherits();

    /// Explicitly set the inherited paths, potentially blocking weaker opinions
    /// that add or remove items, returning true on success, false if the edit
    /// could not be performed.
    USD_API
    bool SetInherits(const SdfPathVector& items);

    /// Return all the paths in this prim's stage's local layer stack that would
    /// compose into this prim via direct inherits (excluding prim specs that
    /// would be composed into this prim due to inherits authored on ancestral
    /// prims) in strong-to-weak order.
    ///
    /// Note that there currently may not be any scene description at these
    /// paths on the stage.  This returns all the potential places that such
    /// opinions could appear.
    USD_API
    SdfPathVector GetAllDirectInherits() const;
    
    /// Return the prim this object is bound to.
    const UsdPrim &GetPrim() const { return _prim; }
    UsdPrim GetPrim() { return _prim; }

    explicit operator bool() { return bool(_prim); }

    // ---------------------------------------------------------------------- //
    // Private Methods and Members
    // ---------------------------------------------------------------------- //
private:

    UsdPrim _prim;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_INHERITS_H
