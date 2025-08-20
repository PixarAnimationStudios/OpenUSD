//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/input.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"

#include "pxr/base/tf/bits.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

VdfInput::~VdfInput()
{
    for(size_t i=0; i<_connections.size(); i++)
        delete _connections[i];
}

const VdfInputSpec &
VdfInput::GetSpec() const
{
    return *_owner.GetInputSpecs().GetInputSpec(_specIndex);
}

VdfConnection *
VdfInput::_AddConnection(
    VdfOutput     &output,
    const VdfMask &mask,
    int            atIndex)
{
    TfAutoMallocTag2 tag("Vdf", "VdfInput::_AddConnection");
    VdfConnection *const c = new VdfConnection(output, mask, *this);

    if (atIndex == VdfNetwork::AppendConnection) {
        _connections.push_back(c);
    } else {

        // If the index is out-of-range we fallback to append.
        if (!TF_VERIFY(atIndex >= 0 &&
                static_cast<unsigned int>(atIndex) <= _connections.size())) {
            atIndex = _connections.size();
        }

        _connections.insert(_connections.begin() + atIndex, c);
    }

    return c;
}

void
VdfInput::_RemoveConnection(VdfConnection *connection)
{
    // Remove connection from the input connector.
    const VdfConnectionVector::iterator iter =
        std::find(_connections.begin(), _connections.end(), connection);

    if (TF_VERIFY(iter != _connections.end())) {
        _connections.erase(iter);
    }
}

void
VdfInput::_ReorderInputConnections(
    const TfSpan<const VdfConnectionVector::size_type> &newToOldIndices)
{
    const VdfConnectionVector::size_type numConnections = _connections.size();

    if (newToOldIndices.size() != numConnections) {
        TF_CODING_ERROR(
            "Mismatch between the number of input connections (%zu) "
            "and the number of indices given to reorder them (%zu).",
            numConnections, newToOldIndices.size());
        return;
    }

    // Used to validate that duplicate old indices aren't specified.
    TfBits oldIndices(numConnections);

    VdfConnectionVector newConnections;
    newConnections.reserve(numConnections);
    for (const VdfConnectionVector::size_type oldIndex : newToOldIndices) {
        if (oldIndex >= numConnections) {
            TF_CODING_ERROR(
                "The indices given for reordering include out-of-range old "
                "indices (including, at least, %zu).", oldIndex);
            return;
        }

        if (oldIndices.IsSet(oldIndex)) {
            TF_CODING_ERROR(
                "The indices given for reordering contains duplicate old "
                "indices (including, at least, %zu).", oldIndex);
            return;
        }
        oldIndices.Set(oldIndex);

        newConnections.push_back(_connections[oldIndex]);
    }

    _connections = std::move(newConnections);
}

std::string
VdfInput::GetDebugName() const
{
    return "[" + GetName().GetString() + "]" + GetNode().GetDebugName();
}

PXR_NAMESPACE_CLOSE_SCOPE
