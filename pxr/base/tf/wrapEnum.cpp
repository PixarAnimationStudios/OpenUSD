//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static size_t __hash__(Tf_PyEnumWrapper const &self)
{
    return TfHash()(self.value);
}

static nanobind::object _GetValueFromFullName(const std::string &fullName) 
{
    bool found = false;
    const TfEnum value = TfEnum::GetValueFromFullName(fullName, &found);
    if (found) {
        return nanobind::steal(
            Tf_PyEnumRegistry::GetInstance().GetPyObject(value));
    }
    return nanobind::none();
}

} // anonymous namespace 

void wrapEnum(nanobind::module_ &m)
{
    namespace nb = nanobind;

    nb::class_<Tf_PyEnum>(m, "Enum")
        .def_static("GetValueFromFullName", _GetValueFromFullName)
        ;

    nb::class_<Tf_PyEnumWrapper, Tf_PyEnum> enumWrapper(m, "Tf_PyEnumWrapper");
    enumWrapper.attr("_baseName") = std::string();
    enumWrapper
        .def_prop_ro("value", &Tf_PyEnumWrapper::GetValue)
        .def_prop_ro("name", &Tf_PyEnumWrapper::GetName)
        .def_prop_ro("fullName", &Tf_PyEnumWrapper::GetFullName)
        .def_prop_ro("displayName", &Tf_PyEnumWrapper::GetDisplayName)
        .def("__repr__", [](nb::handle self) { return Tf_PyEnumRepr(self); })
        .def("__hash__", __hash__)
        .def("__int__", &Tf_PyEnumWrapper::GetValue)
        .def("__index__", &Tf_PyEnumWrapper::GetValue)
        .def("__eq__", [](Tf_PyEnumWrapper const &self, nb::handle other) -> nb::object {
            Tf_PyEnumWrapper *rhs = nullptr;
            if (NB_CALL(nb_type_get)(NB_CTX, &typeid(Tf_PyEnumWrapper),
                                     other.ptr(), 0, nullptr,
                                     reinterpret_cast<void **>(&rhs))) {
                return nb::cast(self == *rhs);
            }
            if (PyLong_Check(other.ptr()) && !PyBool_Check(other.ptr())) {
                long val = PyLong_AsLong(other.ptr());
                if (!PyErr_Occurred()) {
                    return nb::cast(self == val);
                }
                PyErr_Clear();
            }
            return nb::borrow(Py_NotImplemented);
        })
        .def("__ne__", [](Tf_PyEnumWrapper const &self, nb::handle other) -> nb::object {
            Tf_PyEnumWrapper *rhs = nullptr;
            if (NB_CALL(nb_type_get)(NB_CTX, &typeid(Tf_PyEnumWrapper),
                                     other.ptr(), 0, nullptr,
                                     reinterpret_cast<void **>(&rhs))) {
                return nb::cast(self != *rhs);
            }
            if (PyLong_Check(other.ptr()) && !PyBool_Check(other.ptr())) {
                long val = PyLong_AsLong(other.ptr());
                if (!PyErr_Occurred()) {
                    return nb::cast(!(self == val));
                }
                PyErr_Clear();
            }
            return nb::borrow(Py_NotImplemented);
        })
        .def(nb::self < nb::self)
        .def(nb::self <= nb::self)
        .def(nb::self > nb::self)
        .def(nb::self >= nb::self)
        .def(nb::self | nb::self)
        .def(long() | nb::self)
        .def(nb::self | long())
        .def(nb::self & nb::self)
        .def(long() & nb::self)
        .def(nb::self & long())
        .def(nb::self ^ nb::self)
        .def(long() ^ nb::self)
        .def(nb::self ^ long())
        .def(~nb::self)
        ;
}
