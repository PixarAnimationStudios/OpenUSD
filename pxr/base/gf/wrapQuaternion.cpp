//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/quaternion.h"
#include "pxr/base/tf/hash.h"
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

static string _Repr(GfQuaternion const &self) {
    return TF_PY_REPR_PREFIX + "Quaternion(" + TfPyRepr(self.GetReal()) + ", " +
        TfPyRepr(self.GetImaginary()) + ")";
}

static size_t __hash__(GfQuaternion const &self) { return TfHash{}(self); }

} // anonymous namespace 

void wrapQuaternion(nanobind::module_ &m)
{    
    typedef GfQuaternion This;

    m.def( "Dot", (double (*)(const GfQuaternion &, const GfQuaternion &))GfDot);
    m.def( "Slerp", (GfQuaternion (*)(double, const GfQuaternion&, const GfQuaternion&))GfSlerp);
    
    nanobind::class_<This> cls(m, "Quaternion", "Quaternion class");
    cls
        .def(nanobind::init<>())

        .def(nanobind::init<int>())

        .def(nanobind::init<double, const GfVec3d &>())

        .def(nanobind::init<const This &>())

        .def(TfTypePythonClass())

        .def_static("GetZero", &This::GetZero)

        .def_static("GetIdentity", &This::GetIdentity)

        .def_prop_rw("real", &This::GetReal, &This::SetReal)
        .def_prop_rw("imaginary", &This::GetImaginary, &This::SetImaginary,
                     nanobind::rv_policy::copy)

        .def("GetImaginary", &This::GetImaginary, nanobind::rv_policy::copy)
        .def("GetInverse", &This::GetInverse)
        .def("GetLength", &This::GetLength)
        .def("GetReal", &This::GetReal)

        .def("GetNormalized", &This::GetNormalized,
             nanobind::arg("eps") = GF_MIN_VECTOR_LENGTH)
        .def("Normalize", [](This &self, double eps) -> This & {
            self.Normalize(eps);
            return self;
        }, nanobind::arg("eps") = GF_MIN_VECTOR_LENGTH, nanobind::rv_policy::reference)

        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def( nanobind::self == nanobind::self )
        .def( nanobind::self != nanobind::self )
        .def( nanobind::self *= nanobind::self )
        .def( nanobind::self *= double() )
        .def( nanobind::self /= double() )
        .def( nanobind::self += nanobind::self )
        .def( nanobind::self -= nanobind::self )
        .def( nanobind::self + nanobind::self )
        .def( nanobind::self - nanobind::self )
        .def( nanobind::self * nanobind::self )
        .def( nanobind::self * double() )
        .def( double() * nanobind::self )
        .def( nanobind::self / double() )
        .def("__repr__", _Repr)
        .def("__hash__", __hash__)

        ;
}
