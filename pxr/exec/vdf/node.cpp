//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/node.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/inputAndOutputSpecsRegistry.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/requiredInputsPredicate.h"
#include "pxr/exec/vdf/scheduleInvalidator.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/mallocTag.h"

PXR_NAMESPACE_OPEN_SCOPE

VdfNode::VdfNode(VdfNetwork *network,
                 const VdfInputSpecs &inputSpecs,
                 const VdfOutputSpecs &outputSpecs)
    : _network(*network)
    , _specs(NULL)
    , _inputs(inputSpecs.GetSize())
    , _outputs(outputSpecs.GetSize())
{
    TfAutoMallocTag2 tag("Vdf", "VdfNode::VdfNode");
    
    // Add us to the network.
    TF_AXIOM(network);
    network->_AddNode(this);

    // Initialize specs that we acquire directly from the registry.
    _InitializeInputAndOutputSpecs(
        network->_GetInputOutputSpecsRegistry().AcquireSharedSpecs(
            inputSpecs, outputSpecs));
}

VdfNode::VdfNode(VdfNetwork *network)
    : _network(*network)
    , _specs(NULL)
{
    TfAutoMallocTag2 tag("Vdf", "VdfNode::VdfNode");
    
    // Add us to the network.
    TF_AXIOM(network);
    network->_AddNode(this);
    
    // Note that users of this constructor are on the hook for calling
    // _InitializeInputAndOutputSpecs themselves.
}

VdfNode::~VdfNode()
{
    _network._UnregisterNodeDebugName(*this);

    // Delete all inputs and outputs.
    TF_FOR_ALL(iter, _inputs)
        delete iter->second;

    TF_FOR_ALL(iter, _outputs)
        delete iter->second;

    _network._GetInputOutputSpecsRegistry().ReleaseSharedSpecs(_specs);
}

VdfInput *
VdfNode::GetInput(const TfToken &inputName)
{
    _TokenInputMap::iterator i = _inputs.find(inputName);

    if (i == _inputs.end()) {
        return NULL;
    }

    return i->second;
}

VdfOutput *
VdfNode::GetOutput(const TfToken &name)
{
    VdfOutput *output = GetOptionalOutput(name);

    if (!output) {
        TF_CODING_ERROR("Output connector '" + name.GetString() +
                        "' does not exist.");
    }

    return output;
}

VdfOutput *
VdfNode::GetOptionalOutput(const TfToken &name)
{
    _TokenOutputMap::iterator i = _outputs.find(name);

    if (i == _outputs.end()) {
        return NULL;
    }

    return i->second;
}

VdfOutput *
VdfNode::GetOutput()
{
    if (_outputs.empty()) {
        TF_CODING_ERROR("GetOutput() called on node with no outputs.");
        return NULL;
    }

    // This optimization (of not having to do a hash lookup when we only
    // have one output) relies on the fact that size() is constant time.
    if (_outputs.size() != 1) {
        TF_CODING_ERROR("GetOutput() can only be called on nodes with a single output.");
    }

    return _outputs.begin()->second;
}

void 
VdfNode::SetDebugName(const std::string &name)
{
    TfAutoMallocTag2 tag("Vdf", __ARCH_PRETTY_FUNCTION__);
    SetDebugNameCallback([name] { return name; });
}

void
VdfNode::SetDebugNameCallback(VdfNodeDebugNameCallback &&callback)
{
    if (!callback) {
        TF_CODING_ERROR("Null callback for node: %s",
                        ArchGetDemangled(typeid(*this)).c_str());
    } else {
        _network._RegisterNodeDebugName(*this, std::move(callback));
    }  
}

const std::string
VdfNode::GetDebugName() const
{
    return _network.GetNodeDebugName(this);
}

size_t
VdfNode::GetMemoryUsage() const
{
    return sizeof(*this);
}

bool
VdfNode::IsSpeculationNode() const
{
    return false;
}

/* virtual */
VdfRequiredInputsPredicate
VdfNode::GetRequiredInputsPredicate(const VdfContext &context) const
{
    return VdfRequiredInputsPredicate::AllReads(*this);
}

VdfMask
VdfNode::ComputeOutputDependencyMask(
    const VdfConnection &inputConnection,
    const VdfMask &inputDependencyMask,
    const VdfOutput &output ) const
{
    const VdfInput &input = inputConnection.GetTargetInput();
    TF_AXIOM(&input.GetNode() == this);
    TF_AXIOM(&output.GetNode() == this);

    const VdfOutput *associatedOutput = input.GetAssociatedOutput();
    if (!associatedOutput) {
        // Call the virtual method to handle dependencies for non-associated
        // inputs.
        return _ComputeOutputDependencyMask(inputConnection,
                                            inputDependencyMask, output);
    }

    // Otherwise, we're handling dependencies for an associated input.

    if (associatedOutput == &output) {
        return inputDependencyMask;
    }

    // Dependency doesn't propagate from associated inputs to
    // non-associated outputs.  This is an important aspect of the
    // semantics of associated outputs, and should be formalized somehow.
    return VdfMask();
}

void 
VdfNode::ComputeOutputDependencyMasks(
    const VdfConnection &inputConnection,
    const VdfMask &inputDependencyMask,
    VdfMaskedOutputVector *outputDependencies) const
{
    if (!TF_VERIFY(outputDependencies))
        return;

    // If a subclass has an implementation, let it win, otherwise fall
    // back to the default implementation.
    if (_ComputeOutputDependencyMasks(inputConnection, inputDependencyMask,
            outputDependencies))
        return;

    TF_FOR_ALL(i, GetOutputsIterator()) {
        const VdfOutput &output = *i->second;

        // Ask the node what mask to use.
        VdfMask dependencyMask = ComputeOutputDependencyMask(
            inputConnection, inputDependencyMask, output);

        // If there are no bits set in the mask, there's nothing to do.
        //
        // It's important that we do this check because
        // ComputeOutputDependencyMask can return an empty mask to indicate
        // all zeros, and if we don't do this check, we can hit axioms when
        // doing mask operations in the caller.
        //
        if (dependencyMask.IsAllZeros())
            continue;

        // XXX:constCast unclean!
        outputDependencies->push_back(
            VdfMaskedOutput(const_cast<VdfOutput *>(&output), dependencyMask));
    }
}

VdfMask
VdfNode::_ComputeOutputDependencyMask(
    const VdfConnection &inputConnection,
    const VdfMask &inputDependencyMask,
    const VdfOutput &output ) const
{
    const VdfMask *affectsMask = output.GetAffectsMask();

    // If the output has an affects mask, return it, indicating that all
    // affected elements depend on the input.
    if (affectsMask) {
        return *affectsMask;
    }

    // Otherwise, return an all-ones mask that's the size of the output.
    return VdfMask::AllOnes(output.GetNumDataEntries());
}

bool
VdfNode::_ComputeOutputDependencyMasks(
    const VdfConnection &inputConnection,
    const VdfMask &inputDependencyMask,
    VdfMaskedOutputVector *outputDependencies) const
{
    // Unimplemented at the base class.
    return false;
}

VdfConnectionAndMaskVector
VdfNode::_ComputeInputDependencyRequest(
    const VdfMaskedOutputVector &request) const
{
    VdfConnectionAndMaskVector inputDependencies;
    for (const VdfMaskedOutput &maskedOutput : request) {
        VdfConnectionAndMaskVector dependencies =
            ComputeInputDependencyMasks(
                maskedOutput, true /* skipAssociatedInputs */);
        inputDependencies.insert(
            inputDependencies.end(),
            dependencies.begin(), dependencies.end());
    }

    return inputDependencies;
}

VdfMask::Bits
VdfNode::ComputeInputDependencyMask(
    const VdfMaskedOutput &maskedOutput,
    const VdfConnection &inputConnection ) const
{
    const VdfInput &input = inputConnection.GetTargetInput();
    TF_DEV_AXIOM(&input.GetNode() == this);

    const VdfOutput *output = maskedOutput.GetOutput();
    TF_DEV_AXIOM(output);
    TF_DEV_AXIOM(&output->GetNode() == this);

    // See if we're handling depdencencies between an output and its
    // associated input.
    const VdfOutput *associatedOutput = input.GetAssociatedOutput();
    if (associatedOutput && associatedOutput == output) {
        return maskedOutput.GetMask().GetBits();
    }

    // Call the virtual method to handle dependencies for non-associated
    // inputs.
    return _ComputeInputDependencyMask(maskedOutput, inputConnection);
}

VdfConnectionAndMaskVector 
VdfNode::ComputeInputDependencyMasks(
    const VdfMaskedOutput &maskedOutput,
    bool skipAssociatedInputs) const
{
    return _ComputeInputDependencyMasks(
        maskedOutput, skipAssociatedInputs);
}

VdfConnectionAndMaskVector
VdfNode::_ComputeInputDependencyMasks(
    const VdfMaskedOutput &maskedOutput,
    bool skipAssociatedInputs) const
{
    VdfConnectionAndMaskVector inputDependencies;

    TF_FOR_ALL(i, GetInputsIterator()) {

        VdfInput &input = *(i->second);

        bool isAssociated =
            input.GetAssociatedOutput() == maskedOutput.GetOutput();

        // If we only look at associated inputs, we skip all others.
        if (skipAssociatedInputs && isAssociated)
            continue;

        TF_FOR_ALL(j, input.GetConnections()) {
            VdfConnection *connection = *j;
            TF_DEV_AXIOM(connection);

            // Ask the node what mask to use when traversing this
            // input connection.
            VdfMask::Bits dependencyMask =
                ComputeInputDependencyMask(maskedOutput, *connection);

            // If the dependent mask has no entries set, we don't need
            // to bother to push this input on the stack to be processed.
            if (dependencyMask.IsAnySet()) {
                inputDependencies.push_back(
                    VdfConnectionAndMask(connection, VdfMask(dependencyMask)));
            }
        }
    }

    return inputDependencies;
}

VdfConnectionAndMaskVector
VdfNode::ComputeInputDependencyRequest(
    const VdfMaskedOutputVector &request) const
{
    return _ComputeInputDependencyRequest(request);
}

VdfMask::Bits
VdfNode::_ComputeInputDependencyMask(
    const VdfMaskedOutput &maskedOutput,
    const VdfConnection &inputConnection ) const
{
    // If there's no affects mask, then we assume the requested
    // masked output depends on inputConnection.
    // If the requested output has an affects mask, we can use it to narrow
    // the input dependencies.
    const VdfMask *affectsMask = maskedOutput.GetOutput()->GetAffectsMask();
    if (!affectsMask || affectsMask->Overlaps(maskedOutput.GetMask())) {
        return inputConnection.GetMask().GetBits();
    }

    // No dependency.
    return VdfMask::Bits();
}

const VdfInputAndOutputSpecs *
VdfNode::_AcquireInputAndOutputSpecsPointer(
    const VdfInputSpecs &inputSpecs,
    const VdfOutputSpecs &outputSpecs)
{
    // The default implementation uses the registry.
    return _network._GetInputOutputSpecsRegistry().AcquireSharedSpecs(
        inputSpecs, outputSpecs);
}

void
VdfNode::_ReleaseInputAndOutputSpecsPointer(
    const VdfInputAndOutputSpecs *specs)
{
    // The default implementation uses the registry.
    _network._GetInputOutputSpecsRegistry().ReleaseSharedSpecs(specs);
}

void 
VdfNode::_InitializeInputAndOutputSpecs(
    const VdfInputAndOutputSpecs *specs)
{
    // Make sure this is only called when we have no specs at all.
    if (!TF_VERIFY(!_specs)) {
        return;
    }

    _specs = specs;

    // Build and store inputs and outputs from the supplied specs.
    // Note the specs themselves are already stored in _specs
    _AppendOutputs(specs->GetOutputSpecs());
    _AppendInputs(specs->GetInputSpecs());
}

void 
VdfNode::_ClearInputAndOutputSpecsPointer()
{
    _specs = nullptr;
}

VdfConnectionVector
VdfNode::GetInputConnections() const
{ 
    VdfConnectionVector connections;

    TF_FOR_ALL(i, GetInputsIterator()) {
        TF_FOR_ALL(c, i->second->GetConnections()) {
            connections.push_back(*c);
        }
    }

    return connections;
}

VdfConnectionVector
VdfNode::GetOutputConnections() const
{
    VdfConnectionVector connections;

    TF_FOR_ALL(o, GetOutputsIterator()) {
        TF_FOR_ALL(c, o->second->GetConnections()) {
            connections.push_back(*c);
        }
    }

    return connections;
}

bool
VdfNode::IsEqual(const VdfNode &rhs) const
{
    if (this == &rhs)
        return true;

    // Note that we're comparing pointers here.  If these pointers are acquired
    // through the shared specs registry, then that's exactly the right thing.
    // If a derived class has its own storage for the specs, that means those
    // kinds of nodes will never compare IsEqual, which is intentional.
    if (_specs != rhs._specs)
        return false;

    if (&_network != &rhs._network)
        return false;

    if (!_IsDerivedEqual(rhs))
        return false;

    return true;
}

/* virtual */
bool
VdfNode::_IsDerivedEqual(const VdfNode &rhs) const
{
    // Default implementation never compares two nodes as equal.
    return false;
}

void
VdfNode::_DidAddInputConnection(const VdfConnection *c, int atIndex)
{
}

void
VdfNode::_WillRemoveInputConnection(const VdfConnection *c)
{
}

void
VdfNode::_ReplaceInputSpecs(const VdfInputSpecs &inputSpecs)
{
    // Note that we don't mark any schedules dirty when replacing
    // inputs.  This has been done by disconnecting the inputs already.
    const VdfInputAndOutputSpecs *oldSpecs = _specs;

    _specs = _AcquireInputAndOutputSpecsPointer(inputSpecs, GetOutputSpecs());

    _ReleaseInputAndOutputSpecsPointer(oldSpecs);

    // Deallocate old inputs.
    TF_FOR_ALL(iter, _inputs) {
        TF_VERIFY(iter->second->GetNumConnections() == 0);
        delete iter->second;
    }

    // Clear old associated inputs.
    TF_FOR_ALL(iter, _outputs)
        iter->second->SetAssociatedInput(NULL);

    // Clear out inputs and re-populate.
    _inputs = _TokenInputMap(inputSpecs.GetSize());
    _AppendInputs(inputSpecs);
}

void
VdfNode::_AppendInputs(
    const VdfInputSpecs &newInputSpecs,
    std::vector<VdfInput*> *resultingInputs)
{
    // Maintain indices 0, 1, 2 ... etc in order of
    // creation.
    const size_t numExistingInputs = _inputs.size();
    const size_t numNewInputs = newInputSpecs.GetSize();

    if (resultingInputs) {
        resultingInputs->reserve(numNewInputs);
    }

    for (size_t i = 0; i < numNewInputs; ++i) {
        const VdfInputSpec *spec = newInputSpecs.GetInputSpec(i);
        const TfToken &outputName = spec->GetAssociatedOutputName();
        const size_t newIndex = numExistingInputs + i;
        VdfInput *newInput = NULL;

        // If we have an associated output, resolve it and set it in the 
        // connector.
        if (!outputName.IsEmpty()) {
            // Look up our corresponding output, if this fails, it will
            // issue an error.
            VdfOutput *output = GetOutput(spec->GetAssociatedOutputName());
            // If we don't have an output, a coding error will have been
            // issued and we won't add this as a valid input, so we can't
            // connect anything to it.
            if (output) {
                newInput = new VdfInput(*this, newIndex, output);
                output->SetAssociatedInput(newInput);
            }
        } else {
            if (spec->GetAccess() == VdfInputSpec::READ) {
                newInput = new VdfInput(*this, newIndex);
            } else {
                TF_CODING_ERROR("Writable input connectors must specify "
                                "valid output.");
            }
        }

        // Store the newly created input.
        if (newInput) {
            // XXX
            // We should have a TF_VERIFY that fails if we have already inserted
            // this input, rather than silently allowing inputs to be added
            // redundantly. Unfortunately, it turns out we do this all over the
            // place at Pixar. Doing so appears to be inadvertent but benign,
            // but we should get to the bottom of that and add the missing
            // verify here.
            if (!_inputs.insert(std::make_pair(
                    spec->GetName(), newInput)).second) {
                delete newInput;
            }

            else if (resultingInputs) {
                resultingInputs->push_back(newInput);
            }
        }

    }
}

void
VdfNode::_AppendOutputs(
    const VdfOutputSpecs &newOutputSpecs,
    std::vector<VdfOutput*> *resultingOutputs)
{
    // Maintain indices 0, 1, 2 ... etc in order of
    // creation.
    const size_t numExistingOutputs = _outputs.size();
    const size_t numNewOutputs = newOutputSpecs.GetSize();

    if (resultingOutputs) {
        resultingOutputs->reserve(numNewOutputs);
    }

    for(size_t i = 0; i < numNewOutputs; ++i) {
        const size_t newIndex = numExistingOutputs + i;
        VdfOutput *const newOutput = new VdfOutput(*this, newIndex);

        const VdfOutputSpec *spec = newOutputSpecs.GetOutputSpec(i);
        const bool inserted = _outputs.insert(
            std::make_pair(spec->GetName(), newOutput)).second;
            
        if (!TF_VERIFY(
            inserted,
            "Can't add duplicate output '%s'.\n", spec->GetName().GetText())) {
            delete newOutput;
        }

        else if (resultingOutputs) {
            resultingOutputs->push_back(newOutput);
        }
    }

    // In general, we don't need to invalidate schedules when we add outputs to
    // a node. The exception is when we were previously applying the
    // optimization for nodes that have exactly one output.
    if (resultingOutputs && numExistingOutputs <= 1) {
        _network._GetScheduleInvalidator()->InvalidateContainingNode(this);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
