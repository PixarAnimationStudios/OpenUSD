//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_CONTEXT_H
#define PXR_EXEC_VDF_CONTEXT_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/error.h"
#include "pxr/exec/vdf/evaluationState.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/scheduleTasks.h"
#include "pxr/exec/vdf/traits.h"
#include "pxr/exec/vdf/types.h"
#include "pxr/exec/vdf/vector.h"

#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/functionLite.h"
#include "pxr/base/tf/mallocTag.h"

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;

////////////////////////////////////////////////////////////////////////////////
///
/// A context is the parameter bundle passed to callbacks of computations.
/// It is the only API through which functions have access to their inputs.
///
class VdfContext
{
public:

    // VdfContext should not be copied or assigned to.
    VdfContext(const VdfContext &rhs) = delete;
    VdfContext &operator=(const VdfContext &rhs) = delete;

    /// Constructs a VdfContext for the given \p node with the current
    /// evaluation \p state.
    ///
    VdfContext(
        const VdfEvaluationState &state,
        const VdfNode &node) :
        VdfContext(state, node, VdfScheduleTaskInvalid)
    {}

    /// Constructs a VdfContext for the given \p node and node \p invocation
    /// with the current evaluation \p state.
    ///
    VdfContext(
        const VdfEvaluationState &state,
        const VdfNode &node,
        const VdfScheduleTaskIndex invocation) :
        _state(state),
        _node(node),
        _invocation(invocation)
    {}

    /// Returns a value from the input named \c name of type \c T.  
    /// This method assumes that the caller expects only a single value to
    /// exist on its input.
    ///
    template<typename T>
    inline VdfByValueOrConstRef<T>
    GetInputValue(const TfToken &name) const;

    /// Returns a pointer to the value from the input named \p name if the  
    /// input has a valid value, otherwise returns nullptr.
    ///
    template<typename T>
    inline const T *
    GetInputValuePtr(const TfToken &name) const;

    /// Returns a pointer to the value from the input named \p name if the  
    /// input has a valid value, otherwise returns \p defPtr.
    ///
    template<typename T>
    inline const T *
    GetInputValuePtr(const TfToken &name, const T *defPtr) const;

    /// Returns true, if there are input values from the input named \c name
    /// of type \c T.  
    ///
    template<typename T>
    inline bool HasInputValue(const TfToken &name) const;

    /// Returns true if the output named \p outputName is requested by at least
    /// one downstream node, or false if there are no consumers for the output
    /// or if \p outputName isn't a valid output on this node.
    ///
    /// This can be used by the node callback to avoid computing expensive
    /// outputs that are not needed.
    ///
    /// NOTE: It's far preferred for computations to be designed with
    /// appropriate granularity, but this may be used in cases where the
    /// computation of multiple outputs isn't feasibly separable.
    ///
    VDF_API
    bool IsOutputRequested(const TfToken &outputName) const;

    /// Sets the value of the output named \p outputName to \p value.  
    ///
    /// This can be used when the node already has all the answers and 
    /// doesn't want to bother with iterators.
    ///
    /// Note that this method currently always performs a full copy, because
    /// it can't tell if we are allowed to take ownership of the given output.
    ///
    template<typename T>
    inline void SetOutput(const TfToken &outputName,
                          const T &value) const;

    /// Sets the value of the output to \p value.  
    ///
    /// This can be used when the node already has all the answers and 
    /// doesn't want to bother with iterators.
    ///
    /// Note that this method currently always performs a full copy, because
    /// it can't tell if we are allowed to take ownership of the given output.
    ///
    /// It is invalid to call this method on any node that does not have
    /// exactly one output.
    ///
    template<typename T>
    inline void SetOutput(const T &value) const;

    /// Sets the value of the output to \p value.  
    ///
    /// This can be used when the node already has all the answers and 
    /// doesn't want to bother with iterators.
    ///
    /// This method moves the data into the output, and thus avoids a copy.
    /// However, note that \p value will no longer contain any meaningful data
    /// after this method returned.
    ///
    /// It is invalid to call this method on any node that does not have
    /// exactly one output.
    ///
    template<typename T>
    inline void SetOutput(T &&value) const;

    /// Sets the value of the output named \p outputName to \p value.  
    ///
    /// This can be used when the node already has all the answers and 
    /// doesn't want to bother with iterators.
    ///
    /// This method moves the data into the output, and thus avoids a copy.
    /// However, note that \p value will no longer contain any meaningful data
    /// after this method returned.
    ///
    template<typename T>
    inline void SetOutput(const TfToken &outputName,
                          T &&value) const;

    /// Sets an empty value on the output.
    ///
    /// It is invalid to call this method on any node that does not have
    /// exactly one output.
    ///
    template<typename T>
    inline void SetEmptyOutput() const;

    /// Sets an empty value on the output named \p outputName.
    ///
    template<typename T>
    inline void SetEmptyOutput(const TfToken &outputName) const;

    /// Sets the one and only output to have the same output value
    /// as the value on the output connected to input \p inputName.
    ///
    /// Calling this method when the input doesn't need to be modified
    /// gives the system an opportunity to apply some optimizations when
    /// possible.
    ///
    /// Note that this optimization might not take effect in certain
    /// circumstances.
    ///
    VDF_API
    void SetOutputToReferenceInput(const TfToken &inputName) const;


    /// \name Error Reporting
    /// @{

    /// Reports a warning to the system that was encountered at runtime. 
    ///
    /// Exactly how the warning is presented to the user, if at all,
    /// is up to the host system.
    ///
    /// Multiple calls to Warn() from the same node will cause the messages
    /// to be concatenated.
    /// 
    VDF_API
    void Warn(const char *fmt, ...) const ARCH_PRINTF_FUNCTION(2, 3);
          

    /// @}


    /// \name Debugging
    /// @{

    /// Returns the debug name for the node for this context.
    ///
    VDF_API
    std::string GetNodeDebugName() const;

    /// Invokes a coding error with an error message and a graph around the
    /// node that this context is currently referencing. 
    /// It takes a printf-style format specification.
    ///
    VDF_API
    void CodingError(const char *fmt, ...) const ARCH_PRINTF_FUNCTION(2, 3);

    /// @}


private:

    // This API is for VdfIterators to access internal context data.
    friend class VdfIterator;

    // Speculation nodes need to call _GetExecutor.
    friend class VdfSpeculationNode;

    // Returns the first input value on the given input, or a null pointer, if
    // no such value exists.
    //
    template<typename T>
    inline const T *_GetFirstInputValue(const TfToken &name) const;

    // Retrieves the request and affects mask of the requested output, if
    // available. Returns \c true if the output is scheduled, and \c false
    // otherwise.
    // 
    // Note, output must be an output on the current node!
    //
    VDF_API
    bool _GetOutputMasks(
        const VdfOutput &output,
        const VdfMask **requestMask,
        const VdfMask **affectsMask) const;

    // Returns true when the output is scheduled and required, and
    // false otherwise.  Used by special iterators to avoid computing outputs
    // that aren't necessary.
    //
    VDF_API
    bool _IsRequiredOutput(const VdfOutput &output) const;

    // Returns the request mask of \p output, if the output has been scheduled
    // and \c NULL otherwise.
    // 
    const VdfMask *_GetRequestMask(const VdfOutput &output) const;

    // Returns the current node
    //
    const VdfNode &_GetNode() const { return _node; }

    // Returns the executor for this context.
    //
    const VdfExecutorInterface &_GetExecutor() const { return _state.GetExecutor(); }

    // Returns the schedule for this context.
    //
    const VdfSchedule &_GetSchedule() const { return _state.GetSchedule(); }

    // Returns the error logger for this context.
    //
    VdfExecutorErrorLogger *_GetErrorLogger() const {
        return _state.GetErrorLogger();
    }

private:

    // The evaluation state
    const VdfEvaluationState &_state;

    // The node this context has been built for.
    const VdfNode &_node;

    // The current node invocation index. If this context is not for a node
    // with multiple invocations, this will be set to VdfScheduleTaskInvalid.
    const VdfScheduleTaskIndex _invocation;
};

///////////////////////////////////////////////////////////////////////////////

template<typename T>
VdfByValueOrConstRef<T>
VdfContext::GetInputValue(const TfToken &name) const
{
    // Calling this API means that the client expects there to be one and
    // only one value, so we always return the first one here if there are 
    // any.
    if (const T *value = _GetFirstInputValue<T>(name)) {
        return *value;
    }

    TF_CODING_ERROR(
        "No input value for token '%s' on node '%s'",
        name.GetText(), GetNodeDebugName().c_str());

    // Ask the type registry for the fallback value to use.
    return VdfExecutionTypeRegistry::GetInstance().GetFallback<T>();
}

template<typename T>
inline const T *
VdfContext::GetInputValuePtr(const TfToken &name) const
{
    return _GetFirstInputValue<T>(name);
}

template<typename T>
inline const T *
VdfContext::GetInputValuePtr(const TfToken &name, const T *defPtr) const
{
    const T *value = _GetFirstInputValue<T>(name);
    return value ? value : defPtr;
}

template<typename T>
bool
VdfContext::HasInputValue(const TfToken &name) const
{
    // Note that we generally shouldn't have to check the result of
    // _GetFirstInputValue(name) here, as opposed to simply checking whether
    // there are any connections with non-zero masks on input.
    //
    // The one exception, unfortunately, is the EfSelectNode, which selects
    // amongst its inputs.  In the case where it doesn't have any inputs, it
    // doesn't set an output at all even though it is connected.  We technically
    // shouldn't be compiling a select node at all when there is no input, but
    // we do so to make sure that "first-time" constraints is fast.  We should
    // revisit that.

    return _GetFirstInputValue<T>(name);
}

template<typename T>
const T *
VdfContext::_GetFirstInputValue(const TfToken &name) const
{
    // We need to implement code fairly similar to what the VdfReadIterator has
    // to do.  The up side is that we can implement a more specific semantic
    // (namely that we return nullptr when the input is connected but not
    // executed, whereas the read iterator will error out).  Also we have the
    // opportunity to squeeze some performance out.  The downside is that we
    // have to make sure that whenever we return true, that the read iterator
    // can reasonably provide a value.  So the code must be kept at least
    // somewhat in sync.
    const VdfInput *input = _GetNode().GetInput(name);
    if (!input) {
        return nullptr;
    }

    for (const VdfConnection *connection : input->GetConnections()) {
        const VdfMask &mask = connection->GetMask();
        const size_t firstIndex = mask.GetFirstSet();
        if (firstIndex < mask.GetSize()) {
            // The connection has a mask on it, make sure there's a value
            // present.

            // Get the output to read the value from.
            const VdfVector *value = _GetExecutor()._GetInputValue(
                *connection, mask);

            if (value) {
                VdfVector::ReadAccessor<T> accessor = 
                    value->GetReadAccessor<T>();
                if (firstIndex < accessor.GetNumValues()) {
                    return &accessor[firstIndex];
                }
            }
        }
    }

    // No values on any of the connections.
    return nullptr;
}

template<typename T>
void
VdfContext::SetOutput(const TfToken &outputName,
                      const T &value) const
{
    TfAutoMallocTag2 tag("Vdf", "VdfContext::SetOutput");
    TfAutoMallocTag2 tag2("Vdf", __ARCH_PRETTY_FUNCTION__);

    // GetOutput emits an error if it returns NULL.
    const VdfOutput *output = _node.GetOutput(outputName);
    if (output && _IsRequiredOutput(*output))
        if (VdfVector *v = _GetExecutor()._GetOutputValueForWriting(*output))
            v->Set(value);
}

template<typename T>
void
VdfContext::SetOutput(const T &value) const
{
    TfAutoMallocTag2 tag("Vdf", "VdfContext::SetOutput");
    TfAutoMallocTag2 tag2("Vdf", __ARCH_PRETTY_FUNCTION__);

    // GetOutput emits an error if it returns NULL. Note that there is no need
    // to check _IsRequiredOutput: By virtue of the owning node being scheduled,
    // we can conclude that its only output is therefore scheduled.
    if (const VdfOutput *output = _node.GetOutput())
        if (VdfVector *v = _GetExecutor()._GetOutputValueForWriting(*output))
            v->Set(value);
}

template<typename T>
void
VdfContext::SetOutput(T &&value) const
{
    TfAutoMallocTag2 tag("Vdf", "VdfContext::SetOutput (move)");
    TfAutoMallocTag2 tag2("Vdf", __ARCH_PRETTY_FUNCTION__);

    // GetOutput emits an error if it returns NULL. Note that there is no need
    // to check _IsRequiredOutput: By virtue of the owning node being scheduled,
    // we can conclude that its only output is therefore scheduled.
    if (const VdfOutput *output = _node.GetOutput())
        if (VdfVector *v = 
                _GetExecutor()._GetOutputValueForWriting(*output))
            v->Set(std::forward<T>(value));
}

template<typename T>
void 
VdfContext::SetOutput(const TfToken &outputName,
                      T &&value) const
{
    TfAutoMallocTag2 tag("Vdf", "VdfContext::SetOutput (move)");
    TfAutoMallocTag2 tag2("Vdf", __ARCH_PRETTY_FUNCTION__);

    // GetOutput emits an error if it returns NULL.
    const VdfOutput *output = _node.GetOutput(outputName);
    if (output && _IsRequiredOutput(*output))
        if (VdfVector *v = _GetExecutor()._GetOutputValueForWriting(*output))
            v->Set(std::forward<T>(value));
}

template<typename T>
void
VdfContext::SetEmptyOutput() const
{
    // GetOutput emits an error if it returns null. Note that there is no need
    // to check _IsRequiredOutput: By virtue of the owning node being scheduled,
    // we can conclude that its only output is therefore scheduled.
    const VdfOutput *const output = _node.GetOutput();
    if (!output) {
        return;
    }

    VdfVector *const vector = _GetExecutor()._GetOutputValueForWriting(*output);
    if (!vector) {
        VDF_FATAL_ERROR(_GetNode(), "Couldn't get output vector.");
    }

    *vector = VdfTypedVector<T>();
}

template<typename T>
void
VdfContext::SetEmptyOutput(const TfToken &outputName) const
{
    // GetOutput emits an error if it returns null.
    const VdfOutput *const output = _node.GetOutput(outputName);
    if (!(output && _IsRequiredOutput(*output))) {
        return;
    }

    VdfVector *const vector = _GetExecutor()._GetOutputValueForWriting(*output);
    if (!vector) {
        VDF_FATAL_ERROR(_GetNode(), "Couldn't get output vector.");
    }

    *vector = VdfTypedVector<T>();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
