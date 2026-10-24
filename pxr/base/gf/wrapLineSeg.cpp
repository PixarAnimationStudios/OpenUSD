//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/lineSeg.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string _Repr(GfLineSeg const &self) {
    return TF_PY_REPR_PREFIX + "LineSeg(" + TfPyRepr(self.GetPoint(0.0)) + ", " +
        TfPyRepr(self.GetPoint(1.0)) + ")";
}

} // anonymous namespace 

void wrapLineSeg(nanobind::module_ &m)
{    
    typedef GfLineSeg This;

    m.def("FindClosestPoints", [](const GfLine &l1, const GfLineSeg &l2) {
        GfVec3d p1(0), p2(0);
        double t1 = 0, t2 = 0;
        bool result = GfFindClosestPoints(l1, l2, &p1, &p2, &t1, &t2);
        return nanobind::make_tuple(result, p1, p2, t1, t2);
    },
        "FindClosestPoints( l1, s2 ) -> tuple< intersects = bool, "
        "p1 = GfVec3d, p2 = GfVec3d, t1 = double, t2 = double>"
        "\n\n"
        "l1 : GfLine\n"
        "s2 : GfLineSeg"
        "\n\n"
        "Computes the closest points between a line and a line "
        "segment, returning a tuple. The first item in the tuple "
        "is true if they intersect. The two points are returned in "
        "p1 and p2.  The parametric distance of each point on the "
        "line and line segment is returned in t1 and t2.\n"
        "----------------------------------------------------------------------");
    m.def("FindClosestPoints", [](const GfLineSeg &l1, const GfLineSeg &l2) {
        GfVec3d p1(0), p2(0);
        double t1 = 0, t2 = 0;
        bool result = GfFindClosestPoints(l1, l2, &p1, &p2, &t1, &t2);
        return nanobind::make_tuple(result, p1, p2, t1, t2);
    },
        "FindClosestPoints( s1, s2 ) -> tuple<result = bool,"
        "p1 = GfVec3d, p2 = GfVec3d, t1 = double, t2 = double>"
        "\n\n"
        "l1 : GfLineSeg\n"
        "l2 : GfLineSeg"
        "\n\n"
        "Computes the closest points between two line segments, "
        "returning a tuple.  The first item in the tuple "
        "is true if they intersect.  The two points are returned in "
        "p1 and p2.  The parametric distance of each point on the "
        "line and line segment is returned in t1 and t2.\n"
        "----------------------------------------------------------------------");
    
    nanobind::class_<This>(m, "LineSeg", "Line segment class")
        .def(nanobind::init<>())
        .def(nanobind::init<const GfVec3d &, const GfVec3d &>())

        .def(TfTypePythonClass())

        .def("GetDirection", &This::GetDirection, nanobind::rv_policy::copy)
        .def("GetLength", &This::GetLength)
        .def("GetPoint", &This::GetPoint)

        .def_prop_ro("direction", &This::GetDirection, nanobind::rv_policy::copy)
        .def_prop_ro("length", &This::GetLength)

        .def("FindClosestPoint", [](const GfLineSeg &self, const GfVec3d &point) {
            double t;
            GfVec3d p1 = self.FindClosestPoint(point, &t);
            return nanobind::make_tuple(p1, t);
        })

        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self)

        .def("__repr__", _Repr)
        ;
}
