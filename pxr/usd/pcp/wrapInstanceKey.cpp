//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/instanceKey.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/base/tf/pyNanobindBridge.h"
 
PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static size_t
__hash__(const PcpInstanceKey& key)
{
    return TfHash{}(key);
}

} // anonymous namespace 

void
wrapInstanceKey(nanobind::module_ &m)
{
    namespace nb = nanobind;

    nb::class_<PcpInstanceKey>(m, "InstanceKey")
        .def(nb::init<>())
        .def(nb::init<const PcpPrimIndex&>(), nb::arg("primIndex"))

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def("__str__", &PcpInstanceKey::GetString)
        .def("__hash__", __hash__)
        ;

}
