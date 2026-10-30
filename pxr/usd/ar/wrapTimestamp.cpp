//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/timestamp.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

static size_t
__hash__(const ArTimestamp &self)
{ 
    return TfHash()(self);
}

static std::string
__repr__(const ArTimestamp &self)
{
    return TF_PY_REPR_PREFIX + "Timestamp" + 
        (self.IsValid() ? 
            TfStringPrintf("(%s)", TfPyRepr(self.GetTime()).c_str()) : "()");
}

#include "pxr/base/tf/pyNanobindBridge.h"

void wrapTimestamp(nanobind::module_& m)
{
    namespace nb = nanobind;
    nb::class_<ArTimestamp>(m, "Timestamp")
        .def(nb::init<>())
        .def(nb::init<double>())
        .def(nb::init<ArTimestamp>())

        .def("IsValid", &ArTimestamp::IsValid)
        .def("GetTime", &ArTimestamp::GetTime)

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(nb::self < nb::self)
        .def(nb::self <= nb::self)
        .def(nb::self > nb::self)
        .def(nb::self >= nb::self)

        .def("__hash__", __hash__)
        .def("__repr__", __repr__)
        ;
}
