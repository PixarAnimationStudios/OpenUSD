//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/primFlags.h"
#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

const Usd_PrimFlagsConjunction UsdPrimDefaultPredicate = 
    UsdPrimIsActive && UsdPrimIsDefined && 
    UsdPrimIsLoaded && !UsdPrimIsAbstract;

const Usd_PrimFlagsPredicate UsdPrimAllPrimsPredicate = 
    Usd_PrimFlagsPredicate::Tautology();

bool
Usd_PrimFlagsPredicate::operator()(const UsdPrim &prim) const
{
    if (!prim) {
        TF_CODING_ERROR("Applying predicate to invalid prim.");
        return false;
    }
    return _Eval(prim._Prim(), prim.IsInstanceProxy());
}

Usd_PrimFlagsConjunction
Usd_PrimFlagsDisjunction::operator!() const {
    return Usd_PrimFlagsConjunction(_GetNegated());
}

Usd_PrimFlagsDisjunction
Usd_PrimFlagsConjunction::operator!() const {
    return Usd_PrimFlagsDisjunction(_GetNegated());
}

PXR_NAMESPACE_CLOSE_SCOPE

