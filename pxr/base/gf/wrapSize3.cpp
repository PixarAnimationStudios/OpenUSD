//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/size3.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/implicit.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/return_arg.hpp"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static int
normalizeIndex(int index) {
    return TfPyNormalizeIndex(index, 3, true /*throw error*/);
}

static int __len__(const GfSize3 &self) {
    return 3;
}

static size_t __getitem__(const GfSize3 &self, int index) {
    index = normalizeIndex(index);
    return self[index];
}

static void __setitem__(GfSize3 &self, int index, size_t value) {
    index = normalizeIndex(index);
    self[index] = value;
}
    
static bool __contains__(const GfSize3 &self, size_t value) {
    if (self[0] == value || self[1] == value || self[2] == value)
        return true;
    return false;
}

static string _Repr(GfSize3 const &self) {
    return TF_PY_REPR_PREFIX + "Size3(" + TfPyRepr(self[0]) + ", " +
        TfPyRepr(self[1]) + ", " + TfPyRepr(self[2]) + ")";
}

} // anonymous namespace 

void wrapSize3()
{    
    typedef GfSize3 This;

    static const int dimension = 3;

    class_<This> cls( "Size3", "A 3D size class", init<>() );
    cls
        .def(init<const This &>())
        .def(init<const GfVec3i &>())
        .def(init<size_t, size_t, size_t>())

        .def( TfTypePythonClass() )

        .def("Set", (GfSize3 &(This::*)(size_t, size_t, size_t))&This::Set, return_self<>())

        .def_readonly("dimension", dimension)

        .def("__len__", __len__)
        .def("__getitem__", __getitem__)
        .def("__setitem__", __setitem__)
        .def("__contains__", __contains__)

        .def( str(self) )
        .def( self == self )
        .def( self != self )

        .def( self += self )
        .def( self -= self )
        .def( self *= size_t() )
        .def( self /= size_t() )
        .def( self + self )
        .def( self - self )
        .def( self * self )
        .def( size_t() * self )
        .def( self * size_t() )
        .def( self / size_t() )
        .def("__repr__", _Repr)

        ;
    to_python_converter<std::vector<This>,
        TfPySequenceToPython<std::vector<This> > >();


    // conversion operator
    implicitly_convertible<This, GfVec3i>();

}
