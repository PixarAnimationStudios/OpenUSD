//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/external/boost/python/class.hpp"
#include "pxr/base/tf/pyEnum.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static size_t __hash__(Tf_PyEnumWrapper const &self)
{
    return TfHash()(self.value);
}

static pxr_boost::python::object _GetValueFromFullName(const std::string &fullName) 
{
    bool found = false;
    const TfEnum value = TfEnum::GetValueFromFullName(fullName, &found);
    if (found) {
        return pxr_boost::python::object(value);
    }
    return pxr_boost::python::object();
}

} // anonymous namespace 

void wrapEnum()
{
    class_<Tf_PyEnum>("Enum", no_init)
        .def("GetValueFromFullName", _GetValueFromFullName)
        .staticmethod("GetValueFromFullName")
        ;

    class_<Tf_PyEnumWrapper, bases<Tf_PyEnum> >
        ("Tf_PyEnumWrapper", no_init)
        .add_property("value", &Tf_PyEnumWrapper::GetValue)
        .add_property("name", &Tf_PyEnumWrapper::GetName)
        .add_property("fullName", &Tf_PyEnumWrapper::GetFullName)
        .add_property("displayName", &Tf_PyEnumWrapper::GetDisplayName)
        .def("__repr__", Tf_PyEnumRepr)
        .def("__hash__", __hash__)
        .def(self == long())
        .def(self == self)
        .def(self < self)
        .def(self <= self)
        .def(self > self)
        .def(self >= self)
        .def(long() | self)
        .def(self | long())
        .def(self | self)
        .def(long() & self)
        .def(self & long())
        .def(self & self)
        .def(long() ^ self)
        .def(self ^ long())
        .def(self ^ self)
        .def( ~ self)
        ;
}
