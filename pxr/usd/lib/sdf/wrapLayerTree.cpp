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
#include <boost/python.hpp>

#include "pxr/usd/sdf/layerTree.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

using namespace boost::python;

static SdfLayerTreeHandle
_NewEmpty()
{
   SdfLayerTreeHandleVector childTrees;
   return SdfLayerTree::New(SdfLayerHandle(), childTrees);
}

static SdfLayerTreeHandle
_NewNoOffset(const SdfLayerHandle & layer,
             const SdfLayerTreeHandleVector & childTrees)
{
   return SdfLayerTree::New(layer, childTrees);
}

static SdfLayerTreeHandle
_New(const SdfLayerHandle & layer,
     const SdfLayerTreeHandleVector & childTrees,
     const SdfLayerOffset & cumulativeOffset)
{
   return SdfLayerTree::New(layer, childTrees, cumulativeOffset);
}

void wrapLayerTree()
{    
    // Register conversion for python list <-> SdfLayerTreeHandleVector
    to_python_converter<SdfLayerTreeHandleVector,
                        TfPySequenceToPython<SdfLayerTreeHandleVector> >();
    TfPyContainerConversions::from_python_sequence<
        SdfLayerTreeHandleVector,
        TfPyContainerConversions::
            variable_capacity_all_items_convertible_policy >();

    class_<SdfLayerTree, TfWeakPtr<SdfLayerTree>, boost::noncopyable>
        ("LayerTree", "", no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(&_NewEmpty))
        .def(TfMakePyConstructor(&_NewNoOffset))
        .def(TfMakePyConstructor(&_New))
        .add_property("layer",
                      make_function(&SdfLayerTree::GetLayer,
                                    return_value_policy<return_by_value>()))
        .add_property("offset",
                      make_function(&SdfLayerTree::GetOffset,
                                    return_value_policy<return_by_value>()))
        .add_property("childTrees",
            make_function(&SdfLayerTree::GetChildTrees,
                          return_value_policy<TfPySequenceToList>()))
        ;
}

TF_REFPTR_CONST_VOLATILE_GET(SdfLayerTree)
