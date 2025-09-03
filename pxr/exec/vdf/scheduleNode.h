//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SCHEDULE_NODE_H
#define PXR_EXEC_VDF_SCHEDULE_NODE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/scheduleTasks.h"
#include "pxr/exec/vdf/types.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNode;
class VdfOutput;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfScheduleOuput
/// 
/// This class contains scheduling information for an output.
/// A VdfScheduleNode contains a list of these.
///
class VdfScheduleOutput
{
public:
    VdfScheduleOutput(const VdfOutput *o, const VdfMask &m)
        : output(o), requestMask(m),
          passToOutput(NULL), fromBufferOutput(NULL),
          uniqueIndex(VdfScheduleTaskInvalid) {}

    /// The output
    const VdfOutput *output;

    /// The request mask for this output
    VdfMask requestMask;

    /// The request mask ANDed with the affects mask (if any)
    VdfMask affectsMask;

    /// The output to pass a buffer to.
    const VdfOutput *passToOutput;

    /// The output to get our buffer from.
    const VdfOutput *fromBufferOutput;

    /// The mask of the data this output is supposed to keep after it passes
    /// its buffer to passToOutput.
    VdfMask keepMask;

    /// The unique index assigned to this output, if it passes its buffer.
    VdfScheduleInputDependencyUniqueIndex uniqueIndex;

};

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfScheduleInput
/// 
/// This class contains scheduling information for an input. A VdfSheduleNode
/// contains a list of these.
/// 
class VdfScheduleInput
{
public:
    /// The output from which the scheduled input sources its values.
    const VdfOutput *source;

    /// The dependency mask, indicating which elements of the source output
    /// value this input depends on.
    VdfMask mask;

    /// The input corresponding to this scheduled input.
    const VdfInput *input;
};

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfScheduleNode
/// 
/// This class contains scheduling information necessary to run a single 
/// VdfNode.  A VdfScheduleGroup is a container of these classes.
///
class VdfScheduleNode
{
public:
    VdfScheduleNode(const VdfNode *n) :
        node(n), outputToClear(nullptr), affective(false) {}
    
    /// Returns the index of \p output in the \p outputs array, or -1 if
    /// it does not exist.
    ///
    int GetOutputIndex(const VdfOutput *output) const {
        size_t size = outputs.size();
        for (size_t i = 0; i < size; ++i) {
            if (ARCH_LIKELY(outputs[i].output == output)) {
                return i;
            }
        }
        return -1;
    }

    /// The node being scheduled.
    const VdfNode *node;

    /// An output whose temporary buffer can be deallocated as soon as
    /// this schedule node has finished executing.
    const VdfOutput *outputToClear;

    /// Whether this node, as scheduled, is affective, meaning it cannot
    /// be ignored as an optimization while a buffer is passed from an
    /// input to its associated output.
    bool affective;

    /// The list of outputs that are being scheduled for this node.
    std::vector<VdfScheduleOutput> outputs;

    /// The list of inputs scheduled for this node.
    std::vector<VdfScheduleInput> inputs;

};

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
