//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SCHEDULE_H
#define PXR_EXEC_VDF_SCHEDULE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/countingIterator.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/request.h"
#include "pxr/exec/vdf/scheduleNode.h"
#include "pxr/exec/vdf/scheduleTasks.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/bits.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfConnection;
class VdfNetwork;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfSchedule
///
/// \brief Contains a specification of how to execute a particular VdfNetwork.
///
/// Contains ordering and dependency information about the nodes in a network.
///

class VdfSchedule
{
public:
    /// Minimal iterator range that the schedule returns instances of, in order
    /// to facilitate iterating over sub-sections of the internal containers.
    ///
    template <typename Iterator>
    class IteratorRange {
    public:
        template <typename IteratorConvertible>
        IteratorRange(IteratorConvertible begin, IteratorConvertible end) :
            _begin(begin), _end(end) {}

        Iterator begin() const { return _begin; }
        Iterator end() const { return _end; }
        bool empty() const { return _begin == _end; }
        size_t size() const { return std::distance(_begin, _end); }

    private:
        Iterator _begin;
        Iterator _end;
    };

    /// Noncopyable.
    ///
    VdfSchedule(const VdfSchedule &) = delete;
    VdfSchedule &operator=(const VdfSchedule &) = delete;

    /// The type for the vector of schedule nodes in the schedule.
    ///
    using ScheduleNodeVector = std::vector<VdfScheduleNode>;

    /// An iterable range of task ids.
    ///
    using TaskIdRange = IteratorRange<Vdf_CountingIterator<VdfScheduleTaskId>>;

    /// An iterable range of input dependencies.
    ///
    using InputDependencyRange =
        IteratorRange<std::vector<VdfScheduleInputDependency>::const_iterator>;

    /// An iterable range of scheduled inputs.
    ///
    using InputsRange =
        IteratorRange<std::vector<VdfScheduleInput>::const_iterator>;

    /// An OutputId is a small key object that, once obtained for a particular
    /// VdfOutput, can be used to query the schedule about that VdfOutput.
    /// Querying the schedule using OutputId allows efficient queries to be made
    /// without specific knowledge of how the schedule stores its data.
    ///
    class OutputId {
    public:
        /// Returns whether this OutputId can be used to make queries
        /// about an output's scheduling. Output which are not scheduled will
        /// have invalid ids.
        ///
        bool IsValid() const {
            return _scheduleNodeIndex >= 0 && _secondaryIndex >= 0;
        }

        /// Increment this OutputId to refer to the next scheduled output
        /// on the current output's node.
        /// Callers should not expect an OutputId that is incremented past
        /// the end of the scheduled outputs to automatically go invalid.
        /// Rather than using this operator directly, consider using
        /// VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(...) instead.
        ///
        OutputId &operator++() {
            _secondaryIndex++;
            return *this;
        }

        /// Equality operator.
        ///
        bool operator==(const OutputId &rhs) const {
            return _scheduleNodeIndex == rhs._scheduleNodeIndex &&
                   _secondaryIndex == rhs._secondaryIndex;
        }

        bool operator!=(const OutputId &rhs) const {
            return !(*this == rhs);
        }

    private:
        // Construct an OutputId with a specific schedule node index
        // and secondary index.
        //
        // The scheduleNodeIndex is expected to be one of the possible values
        // stored in VdfSchedule::_nodesToIndexMap (i.e. [0, _nodes.size()-1])
        // or a negative value to indicate an invalid id.
        //
        // And invalid id signifies that an output is not scheduled.
        //
        // The secondaryIndex is an index into the associated
        // VdfScheduleNode's VdfScheduleOutputs vector, which stores data
        // about the scheduled node explicitly.
        //
        OutputId(int scheduleNodeIndex, int secondaryIndex) :
            _scheduleNodeIndex(scheduleNodeIndex),
            _secondaryIndex(secondaryIndex)
        {}

        // Only VdfSchedule is allowed to construct instances of VdfOuputId.
        friend class VdfSchedule;

        // Data members
        int _scheduleNodeIndex;
        int _secondaryIndex;
    };

    /// Constructs an empty schedule.
    ///
    VDF_API
    VdfSchedule();

    /// Destructor
    ///
    VDF_API
    ~VdfSchedule();

    /// Clears the schedule.
    ///
    /// This marks the schedule as invalid and is no longer suitable for
    /// execution.
    ///
    VDF_API
    void Clear();

    /// Returns whether or not this schedule is valid and can be used for
    /// execution.
    ///
    bool IsValid() const {
        return _isValid;
    }

    /// Returns the network for this schedule.
    ///
    const VdfNetwork *GetNetwork() const { return _network; }

    /// @{ \name Queries

    /// Returns whether this schedule includes \p node in any way.
    ///
    VDF_API
    bool IsScheduled(const VdfNode &node) const;

    /// Returns a small, cheap OutputId, which can be passed to other Get*
    /// methods in this class to efficiently get scheduling information
    /// about a particular VdfOutput.  If the schedule does not include \p
    /// output, the returned OutputId's IsValid() method will return false.
    ///
    VDF_API
    OutputId GetOutputId(const VdfOutput &output) const;

    /// Similar to GetOutputId, but creates an OutptuId if none exists,
    /// effectively adding the output to the schedule.  So you want to be
    /// very careful how you use this method.
    ///
    VDF_API
    OutputId GetOrCreateOutputId(const VdfOutput &output);

    /// Adds the input targeted by the given \p connection to the schedule. The
    /// specified \p mask indicates which data elements the input depends on.
    ///
    /// \sa DeduplicateInputs
    ///
    VDF_API
    void AddInput(const VdfConnection &connection, const VdfMask &mask);

    /// Consolidates scheduled input entries added by AddInput.
    ///
    /// Ensures that each pair of scheduled input and source output has a
    /// unique entry that accumulates the masks passed to AddInput.  The
    /// scheduler is responsible for calling this method after all inputs have
    /// been added and before any call to GetInputs.
    ///
    VDF_API
    void DeduplicateInputs();

    /// Returns the VdfNode that owns the VdfOutput associated with the given
    /// \p outputId.
    /// 
    VDF_API
    const VdfNode *GetNode(const OutputId &outputId) const;

    /// Gets an OutputId identifying the first scheduled output for the given
    /// \p node, if any.  The returned OutputId may be invalid if there are no
    /// scheduled outputs for \p node.
    ///
    /// Note that \p node must be scheduled for this API to work,
    /// cf. IsScheduled().
    ///
    /// Rather than calling this method directly, consider using
    /// VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(...) instead.
    ///
    VDF_API
    OutputId GetOutputIdsBegin(const VdfNode &node) const;

    /// Gets an OutputId identifying the "end" of the scheduled outputs for
    /// a node.  This OutputId should never be used to query the schedule, as
    /// it never represents a particular scheduled output.
    /// See GetOutputIdsBegin().
    ///
    /// Note that \p node must be scheduled for this API to work,
    /// cf. IsScheduled().
    ///
    /// Rather than calling this method directly, consider using
    /// VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(...) instead.
    ///
    VDF_API
    OutputId GetOutputIdsEnd(const VdfNode &node) const;

    /// Returns a range of inputs scheduled for the given \p node. Note that
    /// not all inputs in the network are also scheduled for the \p node.
    ///
    VDF_API
    InputsRange GetInputs(const VdfNode &node) const;

    /// Returns \c true if the output is expected to have an effect on its 
    /// corresponding input, and \c false otherwise.
    ///
    /// Outputs that don't have an 'affects' mask or a corresponding input
    /// are always considered to affect their data.
    ///
    VDF_API
    bool IsAffective(const OutputId &outputId) const;

    /// @}

    /// @{ \name Queries By OutputId
    ///    Any time the schedule is queried by OutputId, the caller must
    ///    ensure the OutputId's IsValid() method returns true beforehand.
    ///    As an optimization, the schedule does not verify this for the calls
    ///    below.

    /// Returns the scheduled VdfOutput associated with the given OutputId.
    ///
    VDF_API
    const VdfOutput *GetOutput(const OutputId &outputId) const;

    /// Returns the output whose temporary buffer can be immediately deallocated
    /// after \p node has finished executing.
    ///
    VDF_API
    const VdfOutput *GetOutputToClear(const VdfNode &node) const;

    /// Returns the request mask associated with the given OutputId.
    ///
    VDF_API
    const VdfMask &GetRequestMask(const OutputId &outputId) const;

    /// Returns the request mask for the given node invocation.
    ///
    const VdfMask &GetRequestMask(
        const VdfScheduleTaskIndex invocationIndex) const {
        TF_DEV_AXIOM(!VdfScheduleTaskIsInvalid(invocationIndex));
        return _nodeInvocations[invocationIndex].requestMask;
    }

    /// Returns pointers to the request and affects masks simultaneously,
    /// saving on the overhead of making two queries when client code just
    /// needs both masks.
    ///
    VDF_API
    void GetRequestAndAffectsMask(
        const OutputId &outputId,
        const VdfMask **requestMask,
        const VdfMask **affectsMask) const;

    /// Returns pointers to the request and affects masks for the given
    /// node invocation index.
    ///
    void GetRequestAndAffectsMask(
        const VdfScheduleTaskIndex invocationIndex,
        const VdfMask **requestMask,
        const VdfMask **affectsMask) const {
        TF_DEV_AXIOM(!VdfScheduleTaskIsInvalid(invocationIndex));
        *requestMask = &_nodeInvocations[invocationIndex].requestMask;
        *affectsMask = &_nodeInvocations[invocationIndex].affectsMask;
    }

    /// Returns the affects mask associated with the given OutputId.
    ///
    VDF_API
    const VdfMask &GetAffectsMask(const OutputId &outputId) const;

    /// Returns the keep mask associated with the given OutputId.
    ///
    VDF_API
    const VdfMask &GetKeepMask(const OutputId &outputId) const;

    /// Returns the keep mask for the given node invocation index.
    ///
    const VdfMask &GetKeepMask(
        const VdfScheduleTaskIndex invocationIndex) const {
        TF_DEV_AXIOM(!VdfScheduleTaskIsInvalid(invocationIndex));
        return _nodeInvocations[invocationIndex].keepMask;
    }

    /// Returns the "pass to" output associated with the given OutputId.
    ///
    VDF_API
    const VdfOutput *GetPassToOutput(const OutputId &outputId) const;

    /// Returns the "from buffer's" output associated with the given OutputId.
    ///
    VDF_API
    const VdfOutput *GetFromBufferOutput(const OutputId &outputId) const;

    /// Returns \c true if this schedule participates in sparse mung buffer
    /// locking.
    ///
    bool HasSMBL() const {
        return _hasSMBL;
    }

    /// @}

    /// Loops over each scheduled output of \p node and calls \p callback 
    /// with the output and request mask in an efficient manner.
    /// 
    VDF_API
    void ForEachScheduledOutput(
        const VdfNode &node,
        const VdfScheduledOutputCallback &callback) const;

    /// Returns the number of unique input dependencies created for the
    /// scheduled task graph. Each unique input dependency refers to the same
    /// output and mask combination.
    ///
    size_t GetNumUniqueInputDependencies() const {
        return _numUniqueInputDeps;
    }

    /// Returns the total number of compute tasks in the schedule.
    ///
    size_t GetNumComputeTasks() const {
        return _computeTasks.size();
    }

    /// Returns the total number of inputs tasks in the schedule.
    ///
    size_t GetNumInputsTasks() const {
        return _inputsTasks.size();
    }

    /// Returns the total number of prep tasks in the schedule.
    ///
    size_t GetNumPrepTasks() const {
        return _numPrepTasks;
    }

    /// Returns the total number of keep tasks in the schedule.
    ///
    size_t GetNumKeepTasks() const {
        return _numKeepTasks;
    }

    /// Returns a range of ids describing compute tasks associated with
    /// the given node.
    ///
    const TaskIdRange GetComputeTaskIds(const VdfNode &node) const {
        int scheduleNodeIndex = _GetScheduleNodeIndex(node);
        TF_DEV_AXIOM(scheduleNodeIndex >= 0);
        return TaskIdRange(
            _nodesToComputeTasks[scheduleNodeIndex].taskId,
            _nodesToComputeTasks[scheduleNodeIndex].taskId +
            _nodesToComputeTasks[scheduleNodeIndex].taskNum);
    }

    /// Returns an iterable range of task indices given an input dependency.
    ///
    TaskIdRange GetComputeTaskIds(
        const VdfScheduleInputDependency &input) const {
        return TaskIdRange(
            input.computeOrKeepTaskId,
            input.computeOrKeepTaskId + input.computeTaskNum);
    }

    /// Returns an index to the keep task associated with the given node.
    ///
    const VdfScheduleTaskIndex GetKeepTaskIndex(const VdfNode &node) const {
        int scheduleNodeIndex = _GetScheduleNodeIndex(node);
        return scheduleNodeIndex >= 0
            ? _nodesToKeepTasks[scheduleNodeIndex]
            : VdfScheduleTaskInvalid;
    }

    /// Returns the compute task associated with the given task index.
    ///
    const VdfScheduleComputeTask &GetComputeTask(
        const VdfScheduleTaskIndex index) const {
        TF_DEV_AXIOM(index < _computeTasks.size());
        return _computeTasks[index];
    }

    /// Returns the inputs task associated with the given task index.
    ///
    const VdfScheduleInputsTask &GetInputsTask(
        const VdfScheduleTaskIndex index) const {
        TF_DEV_AXIOM(index < _inputsTasks.size());
        return _inputsTasks[index];
    }

    /// Returns an iterable range of prereq input dependencies for the given 
    /// inputs task.
    ///
    InputDependencyRange GetPrereqInputDependencies(
        const VdfScheduleInputsTask &task) const {
        std::vector<VdfScheduleInputDependency>::const_iterator begin =
            _inputDeps.begin() + task.inputDepIndex;
        return InputDependencyRange(begin, begin + task.prereqsNum);
    }

    /// Returns an iterable range of optional (i.e. dependent on prereq results)
    /// input dependencies for the given inputs task.
    ///
    InputDependencyRange GetOptionalInputDependencies(
        const VdfScheduleInputsTask &task) const {
        std::vector<VdfScheduleInputDependency>::const_iterator begin =
            _inputDeps.begin() + task.inputDepIndex + task.prereqsNum;
        return InputDependencyRange(begin, begin + task.optionalsNum);
    }

    /// Returns an iterable range of required (i.e. read/writes and reads not
    /// dependent on prereqs) input dependencies for the given compute task.
    ///
    InputDependencyRange GetRequiredInputDependencies(
        const VdfScheduleComputeTask &task) const {
        std::vector<VdfScheduleInputDependency>::const_iterator begin =
            _inputDeps.begin() + task.requiredsIndex;
        return InputDependencyRange(begin, begin + task.requiredsNum);
    }

    /// Returns the unique index assigned to the output.
    ///
    VDF_API
    VdfScheduleInputDependencyUniqueIndex GetUniqueIndex(
        const OutputId outputId) const;

    /// @{ \name Scheduler Data Access

    /// Returns whether this schedule is small enough to avoid overhead incurred
    /// by the _nodesToIndexMap mapping, which is otherwise of great benefit to
    /// schedule node lookup time.
    ///
    bool IsSmallSchedule() const { return _isSmallSchedule; }

    /// Sets the request that was used to make up this schedule.
    ///
    VDF_API
    void SetRequest(const VdfRequest &request);

    /// Returns the request for this schedule.
    ///
    const VdfRequest &GetRequest() const { return _request; }

    /// Returns the vector of schedule nodes in this schedule.
    ///
    /// It is never appropriate to access the vector of schedule nodes
    /// directly except during scheduling.
    ///
    ScheduleNodeVector &GetScheduleNodeVector() { 
        return _nodes;
    }

    const ScheduleNodeVector &GetScheduleNodeVector() const { 
        return _nodes;
    }

    /// Returns the node index of the schedule node associated with the given
    /// \p outputId.
    ///
    int GetScheduleNodeIndex(const OutputId &outputId) const {
        return outputId._scheduleNodeIndex;
    }

    /// Returns a set of bits where each set bit's index corresponds to
    /// the node index of a node in this schedule.
    /// 
    const TfBits &GetScheduledNodeBits() const { return _scheduledNodes; }

    /// Registers a request mask for the output indicated by \p outputId.
    ///
    VDF_API
    void SetRequestMask(const OutputId &outputId, const VdfMask &mask);

    /// Registers an affects mask for the output indicated by \p outputId.
    ///
    VDF_API
    void SetAffectsMask(const OutputId &outputId, const VdfMask &mask);

    /// Registers a keep mask for the output indicated by \p outputId.
    ///
    VDF_API
    void SetKeepMask(const OutputId &outputId, const VdfMask &mask);    

    /// Registers a "pass to" output for the output indicated by \p outputId.
    ///
    VDF_API
    void SetPassToOutput(const OutputId &outputId, const VdfOutput *output);

    /// Registers a "from buffer" for the output indicated by \p outputId.
    ///
    VDF_API
    void SetFromBufferOutput(const OutputId &outputId, const VdfOutput *output);

    /// Registers an output whose temporary buffer can be eagerly cleared
    /// as soon as \p node has finished executing.
    ///
    VDF_API
    void SetOutputToClear(const VdfNode &node, const VdfOutput *outputToClear);

    /// Initializes structures based on the size of the network.
    ///
    VDF_API
    void InitializeFromNetwork(const VdfNetwork &network);

    /// Enables SMBL for this schedule.
    ///
    void SetHasSMBL(bool enable) {
        _hasSMBL = enable;
    }

    /// @}

private:

    // The VdfScheduler and its derived classes are the only objects allowed
    // to set a scheduler as valid.
    friend class VdfScheduler;

    // The VdfScheduler calls this method to make sure that this schedule
    // is marked as valid and registered with a particular network.
    //
    void _SetIsValidForNetwork(const VdfNetwork *network);

    // Returns the index into _nodes that corresponds to the given VdfNode.
    // If the node is not scheduled and thus has no corresponding _nodes entry,
    // this method returns a value less than 0.
    VDF_API
    int _GetScheduleNodeIndex(const VdfNode &node) const;

    // Ensures that \p node is in the schedule and returns its scheduleNode
    // index.
    //
    int _EnsureNodeInSchedule(const VdfNode &node);

    // Data Members

    // The total list of nodes that we have to execute.  This is where the
    // schedule nodes are owned.
    ScheduleNodeVector _nodes;

    // The request for this schedule
    VdfRequest _request;

    // This is a vector that maps VdfNodes to VdfScheduleNode index in _nodes.
    std::vector<int> _nodesToIndexMap;

    // The network that we are registered with.  All of our scheduled nodes 
    // belong to this network.
    const VdfNetwork *_network;

    // Bits are set for each schedule node's index.
    TfBits _scheduledNodes;

    // Flag as to whether or not the schedule is valid.
    bool _isValid;

    // A flag that determines whether this schedule's query methods will
    // use the small schedule optimization, which is to assume there is no
    // _nodesToIndexMap and instead find schedule nodes by searching the
    // _nodes array directly.
    bool _isSmallSchedule;

    // This flag indicates whether this schedule participates in sparse mung
    // buffer locking.
    bool _hasSMBL;

    // The number of unique input dependencies created for this schedule. Each
    // unique input dependencies refers to the same output and mask combination.
    size_t _numUniqueInputDeps;

    // The scheduled tasks for parallel evaluation.
    Vdf_DefaultInitVector<VdfScheduleComputeTask> _computeTasks;
    Vdf_DefaultInitVector<VdfScheduleInputsTask> _inputsTasks;
    size_t _numKeepTasks;
    size_t _numPrepTasks;

    // Scheduled node invocations for nodes with multiple invocations.
    Vdf_DefaultInitVector<VdfScheduleNodeInvocation> _nodeInvocations;

    // The array of input dependencies used to orchestrate task synchronization.
    std::vector<VdfScheduleInputDependency> _inputDeps;

    // Arrays that map from the scheduled node index to the scheduled tasks
    // corresponding to that node.
    std::vector<VdfScheduleNodeTasks> _nodesToComputeTasks;
    std::vector<VdfScheduleTaskIndex> _nodesToKeepTasks;
};

///////////////////////////////////////////////////////////////////////////////

// Example usage:
// void MyFunction(const VdfSchedule &schedule, const VdfNode &node) {
//     VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(outputId, schedule, node) {
//        DoThingsWithARequestMask(schedule->GetRequestMask(outputId));
//     }
// }
//
#define VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(OUTPUT_ID_NAME,VDF_SCHEDULE,VDF_NODE)  \
    for (VdfSchedule::OutputId __endId =                                        \
            (VDF_SCHEDULE).GetOutputIdsEnd(VDF_NODE),                           \
         OUTPUT_ID_NAME = (VDF_SCHEDULE).GetOutputIdsBegin(VDF_NODE) ;          \
         OUTPUT_ID_NAME != __endId; ++OUTPUT_ID_NAME)


///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
