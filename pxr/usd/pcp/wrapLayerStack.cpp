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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyPtrHelpers.h"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

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

void wrapLayerStack(nanobind::module_ &m)
{
    namespace nb = nanobind;

    nb::class_<PcpLayerStack> cls(
        m, "LayerStack", nb::is_weak_referenceable(), nb::dynamic_attr());
    TfNbRefAndWeakPtr(cls);
    cls
        .def(TfTypePythonClass())
        .def_prop_ro("identifier", &PcpLayerStack::GetIdentifier,
                     nb::rv_policy::copy)
        .def_prop_ro("layers", &_GetLayerStackLayers)
        .def_prop_ro("layerOffsets", &_GetLayerOffsets)
        .def_prop_ro("layerTree", &PcpLayerStack::GetLayerTree,
                     nb::rv_policy::copy)
        .def_prop_ro("sessionLayerTree", &PcpLayerStack::GetSessionLayerTree,
                     nb::rv_policy::copy)
        .def_prop_ro("mutedLayers", [](const PcpLayerStack &self) {
            return TfNbSequenceToList(self.GetMutedLayers());
        })
        .def_prop_ro("expressionVariables",
                     &PcpLayerStack::GetExpressionVariables,
                     nb::rv_policy::copy)
        .def_prop_ro("expressionVariableDependencies",
                     [](const PcpLayerStack &self) {
                         return TfNbSequenceToList(
                             self.GetExpressionVariableDependencies());
                     })
        .def_prop_ro("relocatesSourceToTarget",
                     &PcpLayerStack::GetRelocatesSourceToTarget,
                     nb::rv_policy::copy)
        .def_prop_ro("relocatesTargetToSource",
                     &PcpLayerStack::GetRelocatesTargetToSource,
                     nb::rv_policy::copy)
        .def_prop_ro("incrementalRelocatesSourceToTarget",
                     &PcpLayerStack::GetIncrementalRelocatesSourceToTarget,
                     nb::rv_policy::copy)
        .def_prop_ro("incrementalRelocatesTargetToSource",
                     &PcpLayerStack::GetIncrementalRelocatesTargetToSource,
                     nb::rv_policy::copy)
        .def_prop_ro("localErrors", &PcpLayerStack::GetLocalErrors)
        .def_prop_ro("pathsToPrimsWithRelocates",
                     &PcpLayerStack::GetPathsToPrimsWithRelocates,
                     nb::rv_policy::copy)
        // TODO: repr, eq, etc.
        ;
}
