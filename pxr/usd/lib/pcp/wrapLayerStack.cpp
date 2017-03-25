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

using namespace boost::python;
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

static std::vector<std::string>
_GetResolvedAssetPaths(const PcpLayerStack &layerStack)
{

    const std::set<string>& paths = layerStack.GetResolvedAssetPaths();
    return std::vector<string>(paths.begin(), paths.end());
}

} // anonymous namespace 

void wrapLayerStack()
{
    class_<PcpLayerStack, PcpLayerStackPtr, boost::noncopyable>
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
        .add_property("resolvedAssetPaths",
                      make_function(&_GetResolvedAssetPaths,
                                    return_value_policy<TfPySequenceToList>()))
        .add_property("relocatesSourceToTarget",
                      make_function(&PcpLayerStack::GetRelocatesSourceToTarget,
                                    return_value_policy<return_by_value>()))
        .add_property("relocatesTargetToSource",
                      make_function(&PcpLayerStack::GetRelocatesTargetToSource,
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

TF_REFPTR_CONST_VOLATILE_GET(PcpLayerStack)
