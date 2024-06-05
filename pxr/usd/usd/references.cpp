//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/references.h"
#include "pxr/usd/usd/listEditImpl.h"

PXR_NAMESPACE_OPEN_SCOPE

// ------------------------------------------------------------------------- //
// UsdReferences
// ------------------------------------------------------------------------- //

using _ListEditImpl = 
    Usd_ListEditImpl<UsdReferences, SdfReferencesProxy>;

// The implementation doesn't define this function as it needs to be specialized
// so we implement it here.
template <>
SdfReferencesProxy 
_ListEditImpl::_GetListEditorForSpec(const SdfPrimSpecHandle &spec)
{
    return spec->GetReferenceList();
}

bool
UsdReferences::AddReference(const SdfReference& refIn, UsdListPosition position)
{
    return _ListEditImpl::Add(*this, refIn, position);
}

bool
UsdReferences::AddReference(const std::string &assetPath,
                            const SdfPath &primPath,
                            const SdfLayerOffset &layerOffset,
                            UsdListPosition position)
{
    SdfReference reference(assetPath, primPath, layerOffset);
    return AddReference(reference, position);
}

bool
UsdReferences::AddReference(const std::string &assetPath,
                            const SdfLayerOffset &layerOffset,
                            UsdListPosition position)
{
    SdfReference reference(assetPath, SdfPath(), layerOffset);
    return AddReference(reference, position);
}

bool 
UsdReferences::AddInternalReference(const SdfPath &primPath,
                                    const SdfLayerOffset &layerOffset,
                                    UsdListPosition position)
{
    SdfReference reference(std::string(), primPath, layerOffset);
    return AddReference(reference, position);
}

bool
UsdReferences::RemoveReference(const SdfReference& refIn)
{
    return _ListEditImpl::Remove(*this, refIn);
}

bool
UsdReferences::ClearReferences()
{
    return _ListEditImpl::Clear(*this);
}

bool 
UsdReferences::SetReferences(const SdfReferenceVector& itemsIn)
{
    return _ListEditImpl::Set(*this, itemsIn);
}

PXR_NAMESPACE_CLOSE_SCOPE

