//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerTree.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

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

void wrapLayerTree(nanobind::module_ &m)
{    
    namespace nb = nanobind;

    // Register conversion for python list <-> SdfLayerTreeHandleVector
    TfPyContainerConversions::from_python_sequence<
        SdfLayerTreeHandleVector,
        TfPyContainerConversions::
            variable_capacity_all_items_convertible_policy >();

    nb::class_<SdfLayerTree> cls(
        m, "LayerTree", "", nb::is_weak_referenceable(), nb::dynamic_attr());
    cls
        .def(TfNbRefAndWeakPtr())
        .def(TfNbMakePyConstructor(&_NewEmpty))
        .def(TfNbMakePyConstructor(&_NewNoOffset))
        .def(TfNbMakePyConstructor(&_New))
        .def_prop_ro("layer", &SdfLayerTree::GetLayer, nb::rv_policy::copy)
        .def_prop_ro("offset", &SdfLayerTree::GetOffset, nb::rv_policy::copy)
        .def_prop_ro("childTrees", &SdfLayerTree::GetChildTrees, nb::rv_policy::copy)
        ;
}
