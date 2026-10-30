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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/ar/pyResolverContext.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

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

void wrapLayerStackIdentifier(nanobind::module_ &m)
{
    namespace nb = nanobind;
    typedef PcpLayerStackIdentifier This;

    nb::class_<This>(m, "LayerStackIdentifier")
        .def(nb::init<>())
        .def(nb::init<
             const SdfLayerHandle &,
             const SdfLayerHandle &,
             const ArResolverContext &,
             const PcpExpressionVariablesSource &>(),
             nb::arg("rootLayer"),
             nb::arg("sessionLayer") = SdfLayerHandle(),
             nb::arg("pathResolverContext") = ArResolverContext(),
             nb::arg("expressionVariablesOverrideSource") = 
                 PcpExpressionVariablesSource())

        .def_ro("sessionLayer", &This::sessionLayer, nb::rv_policy::copy)
        .def_ro("rootLayer", &This::rootLayer, nb::rv_policy::copy)
        .def_ro("pathResolverContext", &This::pathResolverContext, nb::rv_policy::copy)
        .def_ro("expressionVariablesOverrideSource", 
                &This::expressionVariablesOverrideSource, nb::rv_policy::copy)

        .def("__repr__", &_Repr)
        .def("__hash__", &This::GetHash)
        .def(!nb::self)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(nb::self <  nb::self)
        .def(nb::self <= nb::self)
        .def(nb::self >  nb::self)
        .def(nb::self >= nb::self)
        ;

}
