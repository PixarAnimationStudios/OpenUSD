//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/expressionVariables.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python.hpp"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static SdfLayerHandleVector
_GetLayerStackLayers(const PcpLayerStack &layerStack)
{
    const SdfLayerRefPtrVector& layers = layerStack.GetLayers();
    return SdfLayerHandleVector(layers.begin(), layers.end());
}

static SdfLayerOffsetVector
_GetLayerOffsets(const PcpLayerStack &layerStack)
{
    const size_t numLayers = layerStack.GetLayers().size();

    SdfLayerOffsetVector rval(numLayers);
    for (size_t i = 0; i != numLayers; ++i) {
        if (const SdfLayerOffset* offset = layerStack.GetLayerOffsetForLayer(i))
            rval[i] = *offset;
    }

    return rval;
}

} // anonymous namespace 

void wrapLayerStack()
{
    class_<PcpLayerStack, PcpLayerStackPtr, noncopyable>
        ("LayerStack", no_init)
        .def(TfPyRefAndWeakPtr())
        .add_property("identifier", 
                      make_function(&PcpLayerStack::GetIdentifier,
                                    return_value_policy<return_by_value>()))
        .add_property("layers", 
                      make_function(&_GetLayerStackLayers,
                                    return_value_policy<TfPySequenceToList>()))
        .add_property("layerOffsets",
                      make_function(&_GetLayerOffsets,
                                    return_value_policy<TfPySequenceToList>()))
        .add_property("layerTree", 
                      make_function(&PcpLayerStack::GetLayerTree,
                                    return_value_policy<return_by_value>()))
        .add_property("sessionLayerTree", 
                      make_function(&PcpLayerStack::GetSessionLayerTree,
                                    return_value_policy<return_by_value>()))
        .add_property("mutedLayers",
                      make_function(&PcpLayerStack::GetMutedLayers,
                                    return_value_policy<TfPySequenceToList>()))
        .add_property("expressionVariables",
                      make_function(&PcpLayerStack::GetExpressionVariables,
                                    return_value_policy<return_by_value>()))
        .add_property("expressionVariableDependencies",
                      make_function(
                          &PcpLayerStack::GetExpressionVariableDependencies,
                          return_value_policy<TfPySequenceToList>()))
        .add_property("relocatesSourceToTarget",
                      make_function(&PcpLayerStack::GetRelocatesSourceToTarget,
                                    return_value_policy<return_by_value>()))
        .add_property("relocatesTargetToSource",
                      make_function(&PcpLayerStack::GetRelocatesTargetToSource,
                                    return_value_policy<return_by_value>()))
        .add_property("incrementalRelocatesSourceToTarget",
                      make_function(
                          &PcpLayerStack::GetIncrementalRelocatesSourceToTarget,
                          return_value_policy<return_by_value>()))
        .add_property("incrementalRelocatesTargetToSource",
                      make_function(
                          &PcpLayerStack::GetIncrementalRelocatesTargetToSource,
                          return_value_policy<return_by_value>()))
        .add_property("localErrors", 
                      make_function(&PcpLayerStack::GetLocalErrors,
                                    return_value_policy<TfPySequenceToList>()))
        .add_property("pathsToPrimsWithRelocates", 
              make_function(&PcpLayerStack::GetPathsToPrimsWithRelocates,
                            return_value_policy<TfPySequenceToList>()))
        // TODO: repr, eq, etc.
        ;
}
