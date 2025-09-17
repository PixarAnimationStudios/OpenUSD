//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_OUTPUT_H
#define PXR_EXEC_VDF_OUTPUT_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/types.h"

#include <tbb/spin_mutex.h>

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;
class VdfInput;
class VdfMask;
class VdfOutputSpec;

////////////////////////////////////////////////////////////////////////////////
///
/// A VdfOutput represents an output on a node.  It has a spec and a list of
/// nodes currently connected to it.
///
class VdfOutput
{
public:

    /// Constructor
    ///
    VDF_API
    VdfOutput(VdfNode &owner, int specIndex);

    /// Destructor
    ///
    VDF_API
    ~VdfOutput();

    /// Returns a list of connections connected to this output.
    ///
    const VdfConnectionVector &GetConnections() const { return _connections; }

    /// Returns the number of connections for this output.
    ///
    size_t GetNumConnections() const { 
        return _connections.size(); 
    }

    /// Returns the owning node for this output.
    ///
    const VdfNode &GetNode() const { return _owner; }

    /// Returns the non-const owning node for this output.
    ///
    VdfNode &GetNode() { return _owner; }

    /// Returns the name of this output.
    ///
    VDF_API
    const TfToken &GetName() const;

    /// Sets the input associated with this output.  If \p input is NULL it 
    /// clears the associated input.
    ///
    VDF_API
    void SetAssociatedInput(const VdfInput *input);

    /// Returns the in/out connector associated with this output.
    ///
    const VdfInput *GetAssociatedInput() const { 
        return _associatedInput; 
    }

    /// Returns the mask of elements that this output is expected to modify
    /// from its corresponding input.
    ///
    /// Outputs with no corresponding input return a NULL mask.
    ///
    const VdfMask *GetAffectsMask() const {
        return _affectsMask.IsEmpty() ? nullptr : &_affectsMask;
    }

    /// Sets the affects mask for this output.  
    ///
    /// It is a coding error to set this mask on an output that has no 
    /// corresponding input, or to set a mask that is of size 0.
    ///
    VDF_API
    void SetAffectsMask(const VdfMask &mask);

    /// The unique id of this output. No current, or previously constructed
    /// output in the same network will share this id.
    ///
    VdfId GetId() const {
        return _id;
    }

    /// Get the output index from the output id. The index uniquely identifies
    /// the output in the current state of the network, but may alias an index
    /// which existed in a previous state of the network and has since been
    /// destructed.
    ///
    static VdfIndex GetIndexFromId(const VdfId id) {
        return static_cast<VdfIndex>(id);
    }

    /// Get the output version from the output id. Output ids with the same
    /// index are disambiguated by a version number. Given multiple output ids
    /// with the same index, only one version will match the current state
    /// of the network.
    ///
    static VdfVersion GetVersionFromId(const VdfId id) {
        return static_cast<VdfVersion>(id >> 32);
    }

    /// Returns the connector specification object for this output.
    ///
    VDF_API
    const VdfOutputSpec &GetSpec() const;

    /// Returns the debug name for this output.
    ///
    /// Creates a debug name that combines the output name with the debug
    /// name of its owner.
    ///
    VDF_API
    std::string GetDebugName() const;

    /// Returns the expected number of entries in the data computed at 
    /// this output.
    ///
    /// This number is a guess based on what's currently connected to the 
    /// output.
    ///
    VDF_API
    int GetNumDataEntries() const;

private:

    // Only the VdfNetwork and VdfIsolatedSubnetwork should have access to the
    // connect API.
    friend class VdfNetwork;
    friend class VdfIsolatedSubnetwork;

    // Connects \p node's input named \p inputName to this output with a 
    // given \p mask.  If \p atIndex is >= 0 the connection will be placed at
    // index \p atIndex on the target input.  Otherwise the connection will
    // be appended at the end.
    //
    VdfConnection *_Connect(VdfNode *node,
                            const TfToken &inputName,
                            const VdfMask &mask,
                            int atIndex);

    /// Removes \p connection from this output.
    void _RemoveConnection(VdfConnection *connection);

private:

    // The node that owns this output.
    VdfNode &_owner;

    // The output id.
    const VdfId _id;

    // The input (if any) associated with this output.
    const VdfInput *_associatedInput;

    // The list of connections connected to this output.
    VdfConnectionVector _connections;

    // The mask indicating the elements that this output is expected to modify
    // from its corresponding input.  For outputs with no corresponding input,
    // the mask will be of size 0.
    VdfMask _affectsMask;

    // The index of the connector spec for this output on the owning node.
    int _specIndex;

    // Sychronize concurrently connecting to this output.
    tbb::spin_mutex _connectionsMutex;
};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
