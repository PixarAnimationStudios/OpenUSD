//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/specializes.h"
#include "pxr/usd/usd/listEditImpl.h"

PXR_NAMESPACE_OPEN_SCOPE

// ------------------------------------------------------------------------- //
// UsdSpecializes
// ------------------------------------------------------------------------- //

using _ListEditImpl = 
    Usd_ListEditImpl<UsdSpecializes, SdfSpecializesProxy>;

// The implementation doesn't define this function as it needs to be specialized
// so we implement it here.
template <>
SdfSpecializesProxy 
_ListEditImpl::_GetListEditorForSpec(const SdfPrimSpecHandle &spec)
{
    return spec->GetSpecializesList();
}

bool
UsdSpecializes::AddSpecialize(const SdfPath &primPathIn, 
                              UsdListPosition position)
{
    return _ListEditImpl::Add(*this, primPathIn, position);
}

bool
UsdSpecializes::RemoveSpecialize(const SdfPath &primPathIn)
{
    return _ListEditImpl::Remove(*this, primPathIn);
}

bool
UsdSpecializes::ClearSpecializes()
{
    return _ListEditImpl::Clear(*this);
}

bool 
UsdSpecializes::SetSpecializes(const SdfPathVector& itemsIn)
{
    return _ListEditImpl::Set(*this, itemsIn);
}

PXR_NAMESPACE_CLOSE_SCOPE

