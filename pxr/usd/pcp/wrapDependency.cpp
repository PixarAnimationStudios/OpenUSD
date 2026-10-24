//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/dependency.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string
_DependencyRepr(const PcpDependency &dep)
{
     return TF_PY_REPR_PREFIX + "Cache.Dependency("
        + TfPyRepr(dep.indexPath) + ", "
        + TfPyRepr(dep.sitePath) + ", "
        + TfPyRepr(dep.mapFunc) + ")";
}

} // anonymous namespace 

void 
wrapDependency(nanobind::module_ &m)
{
    namespace nb = nanobind;

    nb::class_<PcpDependency>(m, "Dependency")
        .def_rw("indexPath", &PcpDependency::indexPath, nb::rv_policy::copy)
        .def_rw("sitePath", &PcpDependency::sitePath, nb::rv_policy::copy)
        .def_rw("mapFunc", &PcpDependency::mapFunc, nb::rv_policy::copy)
        .def("__repr__", &_DependencyRepr)
        .def(nb::init<const SdfPath &, const SdfPath &, const PcpMapFunction &>())
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        ;

    TfNbWrapEnum<PcpDependencyType>(m);
}
