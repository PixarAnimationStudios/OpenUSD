//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_PAYLOADS_H
#define PXR_USD_USD_PAYLOADS_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/payload.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdPayloads
///
/// UsdPayloads provides an interface to authoring and introspecting payloads.
/// Payloads behave the same as Usd references except that payloads can be 
/// optionally loaded. 
class UsdPayloads {
    friend class UsdPrim;

    explicit UsdPayloads(const UsdPrim& prim) : _prim(prim) {}

public:
    /// Adds a payload to the payload listOp at the current EditTarget, in the
    /// position specified by \p position. 
    /// \sa \ref Usd_Failing_References "Why adding references may fail" for
    /// explanation of expectations on \p payload and what return values and 
    /// errors to expect, and \ref Usd_OM_ListOps for details on list editing 
    /// and composition of listOps. 
    USD_API
    bool AddPayload(const SdfPayload& payload,
                    UsdListPosition position=UsdListPositionBackOfPrependList);

    /// \overload 
    USD_API
    bool AddPayload(const std::string &identifier,
                    const SdfPath &primPath,
                    const SdfLayerOffset &layerOffset = SdfLayerOffset(),
                    UsdListPosition position=UsdListPositionBackOfPrependList);

    /// \overload
    /// \sa \ref Usd_DefaultPrim_References "Payloads Without Prim Paths"
    USD_API
    bool AddPayload(const std::string &identifier,
                    const SdfLayerOffset &layerOffset = SdfLayerOffset(),
                    UsdListPosition position=UsdListPositionBackOfPrependList);

    /// Add an internal payload to the specified prim.
    /// \sa \ref Usd_Internal_References "Internal Payloads"
    USD_API
    bool AddInternalPayload(const SdfPath &primPath,
                    const SdfLayerOffset &layerOffset = SdfLayerOffset(),
                    UsdListPosition position=UsdListPositionBackOfPrependList);

    /// Removes the specified payload from the payloads listOp at the
    /// current EditTarget.  This does not necessarily eliminate the payload 
    /// completely, as it may be added or set in another layer in the same 
    /// LayerStack as the current EditTarget. 
    /// \sa \ref Usd_OM_ListOps 
    USD_API
    bool RemovePayload(const SdfPayload& ref);

    /// Removes the authored payload listOp edits at the current EditTarget.
    /// The same caveats for Remove() apply to Clear().  In fact, Clear() may
    /// actually increase the number of composed payloads, if the listOp being 
    /// cleared contained the "remove" operator. 
    /// \sa \ref Usd_OM_ListOps 
    USD_API
    bool ClearPayloads();

    /// Explicitly set the payloads, potentially blocking weaker opinions that 
    /// add or remove items. 
    /// \sa \ref Usd_Failing_References "Why adding payloads may fail" for
    /// explanation of expectations on \p items and what return values and 
    /// errors to expect, and \ref Usd_OM_ListOps for details on list editing 
    /// and composition of listOps. 
    USD_API
    bool SetPayloads(const SdfPayloadVector& items);

    /// Return the prim this object is bound to.
    const UsdPrim &GetPrim() const { return _prim; }

    /// \overload
    UsdPrim GetPrim() { return _prim; }

    explicit operator bool() { return bool(_prim); }

private:
    UsdPrim _prim;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_PAYLOADS_H
