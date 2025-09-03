//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_INPUT_H
#define PXR_EXEC_VDF_INPUT_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/inputSpec.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/span.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfConnection;
class VdfNode;
class VdfOutput;

////////////////////////////////////////////////////////////////////////////////
///
/// A VdfInput is used to connect a VdfNode to one or more VdfNodes'
/// outputs.  Each of the connections is represented by a VdfConnection 
/// object that is owned by the VdfInput.
///
class VdfInput 
{
public:

    /// Destructor
    ///
    VDF_API
    ~VdfInput();

    /// Constructor.
    ///
    /// Creates an empty connector.
    ///
    VdfInput(VdfNode &owner, int specIndex, const VdfOutput *output = NULL) : 
        _owner(owner), _associatedOutput(output), _specIndex(specIndex) {
    }

    /// Returns a list of connections connected to this input.
    ///
    const VdfConnectionVector &GetConnections() const { return _connections; }

    /// Returns the number of connections for this input.
    ///
    size_t GetNumConnections() const { 
        return _connections.size(); 
    }

    /// Returns the connection at \p index.
    ///
    const VdfConnection &operator[](size_t index) const {
        return *_connections[index];
    }

    /// Returns the connection at \p index, writable.
    ///
    VdfConnection &GetNonConstConnection(size_t index) const {
        return *_connections[index];
    }

    /// Returns the spec for this input connector.
    ///
    VDF_API
    const VdfInputSpec &GetSpec() const;

    /// Returns the output corresponding to this input.  This is only
    /// non-NULL for writeable input connectors.
    ///
    const VdfOutput *GetAssociatedOutput() const { return _associatedOutput; }

    /// Returns the owning node for this input connector.
    ///
    const VdfNode &GetNode() const { return _owner; }

    /// Returns the non-const owning node for this input connector.
    ///
    VdfNode &GetNode() { return _owner; }

    /// Returns the name of this input.
    ///
    const TfToken &GetName() const {
        return GetSpec().GetName();
    }

    /// Returns a descriptive name for this input connector.
    ///
    VDF_API
    std::string GetDebugName() const;

private:

    // Only the VdfNetwork and VdfIsolatedSubnetwork should have access to the
    // connect API.
    friend class VdfNetwork;
    friend class VdfIsolatedSubnetwork;
    
    // VdfOutput needs to be able to call _AddConnection().
    friend class VdfOutput;

    /// Returns the index of the connector spec of this input on the owning
    /// node.
    ///
    int _GetSpecIndex() const {
        return _specIndex;
    }

    /// Adds a connection to \p output with the given \p mask at index 
    /// \p atIndex.
    ///
    /// If \p atIndex is VdfNetwork::AppendConnection the connection will be
    /// added at the end.
    ///
    VdfConnection *_AddConnection(
        VdfOutput &output,
        const VdfMask &mask,
        int atIndex);

    /// Removes \p connection from this input. 
    void _RemoveConnection(VdfConnection *connection);

    /// Rorders all connections according to the mapping defined by \p
    /// newToOldIndices.
    ///
    /// For each index `i`, `newToOldIndices[i]` is the old connection index and
    /// `i` is the desired new connection index. The number of indices given
    /// must be the same as the number of input connections, each index must be
    /// a valid connection index, and the indices must be unique.
    ///
    void _ReorderInputConnections(
        const TfSpan<const VdfConnectionVector::size_type> &newToOldIndices);

private:

    // The owner of this input connector.
    VdfNode &_owner;

    // A pointer to a corresponding output.  This is only non-null for in/out
    // connectors.
    const VdfOutput *_associatedOutput;

    // The list of connections on this connector.
    VdfConnectionVector _connections;

    // The index of the connector spec for this input on the owning node.
    int _specIndex;
};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
