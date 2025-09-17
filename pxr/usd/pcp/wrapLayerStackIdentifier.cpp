//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapLayerStackIdentifier.cpp

#include "pxr/pxr.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/pcp/expressionVariables.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static
std::string
_Repr(const PcpLayerStackIdentifier& x)
{
    return TfStringPrintf(
        "%sLayerStackIdentifier(%s, %s, %s, %s)",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(x.rootLayer).c_str(),
        TfPyRepr(x.sessionLayer).c_str(),
        TfPyRepr(x.pathResolverContext).c_str(),
        TfPyRepr(x.expressionVariablesOverrideSource).c_str());
}

} // anonymous namespace 

void wrapLayerStackIdentifier()
{
    typedef PcpLayerStackIdentifier This;

    class_<This>("LayerStackIdentifier")
        .def(init<>())
        .def(init<
             const SdfLayerHandle &,
             const SdfLayerHandle &,
             const ArResolverContext &,
             const PcpExpressionVariablesSource &>
             ((args("rootLayer"),
               args("sessionLayer") = SdfLayerHandle(),
               args("pathResolverContext") = ArResolverContext(),
               args("expressionVariablesOverrideSource") = 
                   PcpExpressionVariablesSource())))

        .add_property("sessionLayer", 
                      make_getter(&This::sessionLayer, 
                                  return_value_policy<return_by_value>()))
        .add_property("rootLayer", 
                      make_getter(&This::rootLayer, 
                                  return_value_policy<return_by_value>()))
        .add_property("pathResolverContext", 
                      make_getter(&This::pathResolverContext, 
                                  return_value_policy<return_by_value>()))
        .add_property("expressionVariablesOverrideSource", 
                      make_getter(&This::expressionVariablesOverrideSource, 
                                  return_value_policy<return_by_value>()))

        .def("__repr__", &_Repr)
        .def("__hash__", &This::GetHash)
        .def(!self)
        .def(self == self)
        .def(self != self)
        .def(self <  self)
        .def(self <= self)
        .def(self >  self)
        .def(self >= self)
        ;
}
