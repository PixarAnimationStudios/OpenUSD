//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/size2.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include "pxr/base/tf/pyContainerConversions.h"


#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE


namespace {

static int
normalizeIndex(int index) {
    return TfPyNormalizeIndex(index, 2, true /*throw error*/);
}

static int __len__(const GfSize2 &self) {
    return 2;
}

static size_t __getitem__(const GfSize2 &self, int index) {
    index = normalizeIndex(index);
    return self[index];
}

static void __setitem__(GfSize2 &self, int index, size_t value) {
    index = normalizeIndex(index);
    self[index] = value;
}
    
static bool __contains__(const GfSize2 &self, size_t value) {
    if (self[0] == value || self[1] == value)
        return true;
    return false;
}

static string _Repr(GfSize2 const &self) {
    return TF_PY_REPR_PREFIX + "Size2(" + TfPyRepr(self[0]) + ", " + TfPyRepr(self[1]) + ")";
}

} // anonymous namespace 

void wrapSize2(nanobind::module_ &m)
{    
    typedef GfSize2 This;

    static const int dimension = 2;

    nanobind::class_<This> cls(m, "Size2", "A 2D size class");
    cls
        .def(nanobind::init<>())
        .def(nanobind::init<const This &>())
        .def(nanobind::init<const GfVec2i &>())
        .def(nanobind::init<size_t, size_t>())

        .def(TfTypePythonClass())

        .def("Set", (GfSize2 &(This::*)(size_t, size_t))&This::Set, nanobind::rv_policy::reference)

        .def_ro_static("dimension", &dimension)

        .def("__len__", __len__)
        .def("__getitem__", __getitem__)
        .def("__setitem__", __setitem__)
        .def("__contains__", __contains__)

        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def( nanobind::self == nanobind::self )
        .def( nanobind::self != nanobind::self )
        .def( nanobind::self += nanobind::self )
        .def( nanobind::self -= nanobind::self )
        .def( nanobind::self *= int() )
        .def( nanobind::self /= int() )
        .def( nanobind::self + nanobind::self )
        .def( nanobind::self - nanobind::self )
        .def( nanobind::self * nanobind::self )
        .def( int() * nanobind::self )
        .def( nanobind::self * int() )
        .def( nanobind::self / int() )
        .def("__repr__", _Repr)
        
        ;

    // conversion operator
    nanobind::implicitly_convertible<This, GfVec2i>();
}
