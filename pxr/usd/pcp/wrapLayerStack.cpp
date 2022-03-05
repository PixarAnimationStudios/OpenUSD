//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>


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

void wrapLayerStack()
{
    boost::python::class_<PcpLayerStack, PcpLayerStackPtr, boost::noncopyable>
        ("LayerStack", boost::python::no_init)
        .def(TfPyRefAndWeakPtr())
        .add_property("identifier", 
                      boost::python::make_function(&PcpLayerStack::GetIdentifier,
                                    boost::python::return_value_policy<boost::python::return_by_value>()))
        .add_property("layers", 
                      boost::python::make_function(&_GetLayerStackLayers,
                                    boost::python::return_value_policy<TfPySequenceToList>()))
        .add_property("layerOffsets",
                      boost::python::make_function(&_GetLayerOffsets,
                                    boost::python::return_value_policy<TfPySequenceToList>()))
        .add_property("layerTree", 
                      boost::python::make_function(&PcpLayerStack::GetLayerTree,
                                    boost::python::return_value_policy<boost::python::return_by_value>()))
        .add_property("relocatesSourceToTarget",
                      boost::python::make_function(&PcpLayerStack::GetRelocatesSourceToTarget,
                                    boost::python::return_value_policy<boost::python::return_by_value>()))
        .add_property("relocatesTargetToSource",
                      boost::python::make_function(&PcpLayerStack::GetRelocatesTargetToSource,
                                    boost::python::return_value_policy<boost::python::return_by_value>()))
        .add_property("incrementalRelocatesSourceToTarget",
                      boost::python::make_function(
                          &PcpLayerStack::GetIncrementalRelocatesSourceToTarget,
                          boost::python::return_value_policy<boost::python::return_by_value>()))
        .add_property("incrementalRelocatesTargetToSource",
                      boost::python::make_function(
                          &PcpLayerStack::GetIncrementalRelocatesTargetToSource,
                          boost::python::return_value_policy<boost::python::return_by_value>()))
        .add_property("localErrors", 
                      boost::python::make_function(&PcpLayerStack::GetLocalErrors,
                                    boost::python::return_value_policy<TfPySequenceToList>()))
        .add_property("pathsToPrimsWithRelocates", 
              boost::python::make_function(&PcpLayerStack::GetPathsToPrimsWithRelocates,
                            boost::python::return_value_policy<TfPySequenceToList>()))
        // TODO: repr, eq, etc.
        ;
}

TF_REFPTR_CONST_VOLATILE_GET(PcpLayerStack)
