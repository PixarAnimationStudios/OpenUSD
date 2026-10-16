//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/rect2i.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string _Repr(GfRect2i const &self) {
    return TF_PY_REPR_PREFIX + "Rect2i(" + TfPyRepr(self.GetMin()) + ", " +
        TfPyRepr(self.GetMax()) + ")";
}

static size_t __hash__(GfRect2i const &self) {
    return TfHash()(self);
}

} // anonymous namespace 

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

void wrapRect2i(nanobind::module_ &m)
{    
    using This = GfRect2i;

    nanobind::class_<This>(m, "Rect2i")
        .def(nanobind::init<>())
        .def(nanobind::init<const This &>())
        .def(nanobind::init<const GfVec2i &, const GfVec2i &>())
        .def(nanobind::init<const GfVec2i &, int, int>())

        .def(TfTypePythonClass())

        .def("IsNull", &This::IsNull)
        .def("IsEmpty", &This::IsEmpty)
        .def("IsValid", &This::IsValid)

        .def_prop_rw("min", &This::GetMin, &This::SetMin, nanobind::rv_policy::copy)
        .def_prop_rw("max", &This::GetMax, &This::SetMax, nanobind::rv_policy::copy)

        .def_prop_rw("minX", &This::GetMinX, &This::SetMinX) 
        .def_prop_rw("maxX", &This::GetMaxX, &This::SetMaxX) 
        .def_prop_rw("minY", &This::GetMinY, &This::SetMinY) 
        .def_prop_rw("maxY", &This::GetMaxY, &This::SetMaxY) 

        .def("GetMin", &This::GetMin, nanobind::rv_policy::copy)
        .def("GetMax", &This::GetMax, nanobind::rv_policy::copy)

        .def("GetMinX", &This::GetMinX)
        .def("GetMaxX", &This::GetMaxX)
        .def("GetMinY", &This::GetMinY)
        .def("GetMaxY", &This::GetMaxY)

        .def("SetMin", &This::SetMin)
        .def("SetMax", &This::SetMax)

        .def("SetMinX", &This::SetMinX)
        .def("SetMaxX", &This::SetMaxX)
        .def("SetMinY", &This::SetMinY)
        .def("SetMaxY", &This::SetMaxY)

        .def("GetArea", &This::GetArea)
        .def("GetCenter", &This::GetCenter)
        .def("GetHeight", &This::GetHeight)
        .def("GetSize", &This::GetSize)
        .def("GetWidth", &This::GetWidth)

        .def("Translate", &This::Translate, nanobind::rv_policy::reference)
        .def("GetNormalized", &This::GetNormalized)
        .def("GetIntersection", &This::GetIntersection)
        .def("GetUnion", &This::GetUnion)

        .def("Contains", &This::Contains)

        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self)
        .def(nanobind::self += nanobind::self)
        .def(nanobind::self + nanobind::self)
        
        .def("__repr__", _Repr)
        .def("__hash__", __hash__)
        ;
}
