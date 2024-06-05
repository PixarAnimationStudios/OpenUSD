//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layerTree.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

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

} // anonymous namespace 

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
