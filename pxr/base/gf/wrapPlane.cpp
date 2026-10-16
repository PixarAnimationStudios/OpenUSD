//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/plane.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range3d.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string _Repr(GfPlane const &self) {
    return TF_PY_REPR_PREFIX + "Plane(" + TfPyRepr(self.GetNormal()) + ", " +
        TfPyRepr(self.GetDistanceFromOrigin()) + ")";
}

} // anonymous namespace 

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/vector.h>

void wrapPlane(nanobind::module_ &m)
{    
    typedef GfPlane This;

    m.def("FitPlaneToPoints", [](const std::vector<GfVec3d>& points) -> nanobind::object {
        GfPlane plane;
        return GfFitPlaneToPoints(points, &plane) ? nanobind::cast(plane) : nanobind::none();
    });

    nanobind::class_<This>(m, "Plane")
        .def(nanobind::init<>())
        .def(nanobind::init<const GfVec3d &, double>())
        .def(nanobind::init<const GfVec3d &, const GfVec3d &>())
        .def(nanobind::init<const GfVec3d &, const GfVec3d &, const GfVec3d &>())
        .def(nanobind::init<const GfVec4d &>())

        .def(TfTypePythonClass())

        .def("Set", [](This &self, const GfVec3d &normal, double distanceToOrigin) -> This& {
            self.Set(normal, distanceToOrigin);
            return self;
        }, nanobind::rv_policy::reference)
        .def("Set", [](This &self, const GfVec3d &normal, const GfVec3d &point) -> This& {
            self.Set(normal, point);
            return self;
        }, nanobind::rv_policy::reference)
        .def("Set", [](This &self, const GfVec3d &p0, const GfVec3d &p1, const GfVec3d &p2) -> This& {
            self.Set(p0, p1, p2);
            return self;
        }, nanobind::rv_policy::reference)
        .def("Set", [](This &self, const GfVec4d &eqn) -> This& {
            self.Set(eqn);
            return self;
        }, nanobind::rv_policy::reference)

        .def_prop_ro("normal", &This::GetNormal, nanobind::rv_policy::copy)
        .def_prop_ro("distanceFromOrigin", &This::GetDistanceFromOrigin)

        .def("GetDistance", &This::GetDistance)
        .def("GetDistanceFromOrigin", &This::GetDistanceFromOrigin)
        .def("GetNormal", &This::GetNormal, nanobind::rv_policy::copy)
        .def("GetEquation", &This::GetEquation)
        .def("Project", &This::Project)

        .def("Transform", &This::Transform, nanobind::rv_policy::reference)

        .def("Reorient", [](This &self, const GfVec3d &p) -> This& {
            self.Reorient(p);
            return self;
        }, nanobind::rv_policy::reference)

        .def("IntersectsPositiveHalfSpace",
             (bool (This::*)(const GfRange3d &) const)&This::IntersectsPositiveHalfSpace)

        .def("IntersectsPositiveHalfSpace",
             (bool (This::*)(const GfVec3d &) const)&This::IntersectsPositiveHalfSpace)

        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self)

        .def("__repr__", _Repr)
        ;
}
