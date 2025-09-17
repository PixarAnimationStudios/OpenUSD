//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/output.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/inputSpec.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/outputSpec.h"

#include "pxr/base/tf/diagnostic.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

VdfOutput::VdfOutput(VdfNode &owner, int specIndex) :
    _owner(owner),
    _id(owner.GetNetwork()._AcquireOutputId()),
    _associatedInput(NULL),
    _specIndex(specIndex)
{
}

VdfOutput::~VdfOutput()
{
    _owner.GetNetwork()._ReleaseOutputId(_id);
}

const TfToken &
VdfOutput::GetName() const
{
    return GetSpec().GetName();
}

void
VdfOutput::SetAssociatedInput(const VdfInput *input)
{
    if (input && _associatedInput) {
        TF_CODING_ERROR("Cannot associate more than one input to "
                        "a single output.");
        return;
    }
    _associatedInput = input;
}

void 
VdfOutput::SetAffectsMask(const VdfMask &mask)
{
    if (_associatedInput) {

        if (_affectsMask != mask) {

            _affectsMask = mask;
            GetNode().GetNetwork()._DidChangeAffectsMask(*this);
        }

    } else {

        TF_CODING_ERROR("Can't set the affects mask on output '"
                + GetDebugName()
                + "', it doesn't have a corresponding input.");
    }
}

const VdfOutputSpec &
VdfOutput::GetSpec() const
{
    return *_owner.GetOutputSpecs().GetOutputSpec(_specIndex);
}

VdfConnection * 
VdfOutput::_Connect(VdfNode *node, const TfToken &inputName, 
                    const VdfMask &mask, int atIndex)
{
    TF_AXIOM(node);
    TfAutoMallocTag2 tag("Vdf", "VdfOutput::_Connect");

    VdfInput *input = node->GetInput(inputName);
    if (!input) {
        TF_CODING_ERROR("Couldn't find input '"
                + inputName.GetString()
                + "' on node '"
                + node->GetDebugName()
                + "' to connect to.");
        return NULL;
    }

    // Now we want to do some validation to make sure that we are 
    // connecting type-identical input and outputs.

    const VdfInputSpec *inputSpec = &input->GetSpec();

    // We better have a valid input spec or our network is not coherent.
    if (!inputSpec) {
        // CODE_COVERAGE_OFF - should never be possible to hit this
        TF_CODING_ERROR("We couldn't find a spec for the given input.");
        return NULL;
        // CODE_COVERAGE_ON
    }

    const VdfOutputSpec &spec = GetSpec();

    // Check for type compatibility and prevent users from making bogus
    // connections.

    if (!TF_VERIFY(
        inputSpec->TypeMatches(spec),
        "Input and output types don't match.  Trying to connect %s (%s) "
        "to [%s]%s (%s)",
        GetDebugName().c_str(),
        spec.GetTypeName().c_str(),
        inputName.GetText(),
        node->GetDebugName().c_str(),
        inputSpec->GetTypeName().c_str())) {

        return NULL;
    }

    // If this is a writable connector, we currently don't support
    // more than one connection into it.
    if (inputSpec->GetAccess() != VdfInputSpec::READ &&
        input->GetNumConnections() > 0) {

        TF_CODING_ERROR("The current execution system does not support more "
                        "than one connection to a ReadWrite connector.");

        return NULL;
    }

    // Connect to the input.
    VdfConnection *connection =
        input->_AddConnection(*this, mask, atIndex);

    // Add this node to the list of nodes that we're connected to. 
    //
    // When we're connected to the same node multiple times, there will be
    // multiple copies of the connection in _connections. _Disconnect() relies
    // on that behavior.
    {
        std::lock_guard<tbb::spin_mutex> lock(_connectionsMutex);
        _connections.push_back(connection);
    }

    return connection;
}

void
VdfOutput::_RemoveConnection(VdfConnection *connection)
{
    std::lock_guard<tbb::spin_mutex> lock(_connectionsMutex);

    // Remove connection from _connections[].
    VdfConnectionVector::iterator iter =
        std::find(_connections.begin(), _connections.end(), connection);

    if (!TF_VERIFY(iter != _connections.end()))
        return;
    
    // Note that connection order doesn't matter on an output.  It would
    // be great if we could avoid the find() above completely because it
    // is a major slowdown when deleting models.
    
    std::swap(*iter, _connections.back());
    _connections.pop_back();
}

std::string
VdfOutput::GetDebugName() const
{
    return GetNode().GetDebugName() + "[" + GetName().GetString() + "]";
}

int 
VdfOutput::GetNumDataEntries() const
{
    // If we have an affects mask, that will give us the right size.
    if (const VdfMask *const affectsMask = GetAffectsMask()) {
        return affectsMask->GetSize();
    }

    // If we have any outgoing connections, the size on one of their masks
    // will give us the right answer.
    if (!_connections.empty()) {
        return _connections.front()->GetMask().GetSize();
    }

    // If we have an associated input and it is connected, that mask gives us
    // the right answer.
    if (_associatedInput &&
        _associatedInput->GetNumConnections() > 0) {
        return (*_associatedInput)[0].GetMask().GetSize();

    }

    // Finally we have no option, we don't know what size of data we'll end up
    // with if we compute. We simply return 1 here.
    return 1;
}

PXR_NAMESPACE_CLOSE_SCOPE
