//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/payloads.h"
#include "pxr/usd/usd/listEditImpl.h"

PXR_NAMESPACE_OPEN_SCOPE

// ------------------------------------------------------------------------- //
// UsdPayloads
// ------------------------------------------------------------------------- //

using _ListEditImpl = 
    Usd_ListEditImpl<UsdPayloads, SdfPayloadsProxy>;

// The implementation doesn't define this function as it needs to be specialized
// so we implement it here.
template <>
SdfPayloadsProxy 
_ListEditImpl::_GetListEditorForSpec(const SdfPrimSpecHandle &spec)
{
    return spec->GetPayloadList();
}

bool
UsdPayloads::AddPayload(const SdfPayload& refIn, UsdListPosition position)
{
    return _ListEditImpl::Add(*this, refIn, position);
}

bool
UsdPayloads::AddPayload(const std::string &assetPath,
                        const SdfPath &primPath,
                        const SdfLayerOffset &layerOffset,
                        UsdListPosition position)
{
    SdfPayload payload(assetPath, primPath, layerOffset);
    return AddPayload(payload, position);
}

bool
UsdPayloads::AddPayload(const std::string &assetPath,
                        const SdfLayerOffset &layerOffset,
                        UsdListPosition position)
{
    SdfPayload payload(assetPath, SdfPath(), layerOffset);
    return AddPayload(payload, position);
}

bool 
UsdPayloads::AddInternalPayload(const SdfPath &primPath,
                                const SdfLayerOffset &layerOffset,
                                UsdListPosition position)
{
    SdfPayload payload(std::string(), primPath, layerOffset);
    return AddPayload(payload, position);
}

bool
UsdPayloads::RemovePayload(const SdfPayload& refIn)
{
    return _ListEditImpl::Remove(*this, refIn);
}

bool
UsdPayloads::ClearPayloads()
{
    return _ListEditImpl::Clear(*this);
}

bool 
UsdPayloads::SetPayloads(const SdfPayloadVector& itemsIn)
{
    return _ListEditImpl::Set(*this, itemsIn);
}

PXR_NAMESPACE_CLOSE_SCOPE

