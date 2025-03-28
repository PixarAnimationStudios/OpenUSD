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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

void wrapTimestamp()
{
    class_<ArTimestamp>("Timestamp")
        .def(init<double>())
        .def(init<ArTimestamp>())

        .def("IsValid", &ArTimestamp::IsValid)
        .def("GetTime", &ArTimestamp::GetTime)

        .def(self == self)
        .def(self != self)
        .def(self < self)
        .def(self <= self)
        .def(self > self)
        .def(self >= self)  

        .def("__hash__", __hash__)
        .def("__repr__", __repr__)
        ;
}
