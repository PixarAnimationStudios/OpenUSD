//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/instanceKey.h"
#include "pxr/usd/pcp/primIndex.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"
 
PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static size_t
__hash__(const PcpInstanceKey& key)
{
    return TfHash{}(key);
}

} // anonymous namespace 

void
wrapInstanceKey()
{
    class_<PcpInstanceKey>("InstanceKey")
        .def(init<const PcpPrimIndex&>(args("primIndex")))

        .def(self == self)
        .def(self != self)

        .def("__str__", &PcpInstanceKey::GetString)
        .def("__hash__", __hash__)
        ;
}
