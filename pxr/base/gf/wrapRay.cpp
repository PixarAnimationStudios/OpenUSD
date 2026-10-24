//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/ray.h"
#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/line.h"
#include "pxr/base/gf/lineSeg.h"
#include "pxr/base/gf/plane.h"
#include "pxr/base/gf/range3d.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static void
SetStartPointHelper( GfRay &self, const GfVec3d &startPoint ) {
    self.SetPointAndDirection( startPoint, self.GetDirection() );
}

static void
SetDirectionHelper( GfRay &self, const GfVec3d &direction ) {
    self.SetPointAndDirection( self.GetStartPoint(), direction );
}

static string _Repr(GfRay const &self) {
    return TF_PY_REPR_PREFIX + "Ray(" + TfPyRepr(self.GetStartPoint()) + ", " +
        TfPyRepr(self.GetDirection()) + ")";
}

} // anonymous namespace 

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

void wrapRay(nanobind::module_ &m)
{    
    typedef GfRay This;

    m.def("FindClosestPoints", [](const GfRay &l1, const GfLine &l2) {
        GfVec3d p1(0), p2(0);
        double t1 = 0.0, t2 = 0.0;
        bool result = GfFindClosestPoints(l1, l2, &p1, &p2, &t1, &t2);
        return nanobind::make_tuple(result, p1, p2, t1, t2);
    },
        "FindClosestPoints( r1, l2 ) -> tuple<intersects=bool, "
        "p1 = GfVec3d, p2 = GfVec3d, t1 = double, t2 = double>\n"
        "\n"
        "r1 : GfRay\n"
        "l2 : GfLine\n"
        "\n"
        "Computes the closest points between a ray and a line,\n"
        "returning a tuple. The first item in the tuple is true if "
        "they intersect. The two points are returned in p1 and p2.\n"
        "The parametric distance of each point on the ray and line is\n"
        "returned in t1 and t2.\n"
        "----------------------------------------------------------------------"
        );
    m.def("FindClosestPoints", [](const GfRay &l1, const GfLineSeg &l2) {
        GfVec3d p1(0), p2(0);
        double t1 = 0.0, t2 = 0.0;
        bool result = GfFindClosestPoints(l1, l2, &p1, &p2, &t1, &t2);
        return nanobind::make_tuple(result, p1, p2, t1, t2);
    },
        "FindClosestPoints( r1, s2 ) -> tuple<intersects = bool, "
        "p1 = GfVec3d, p2 = GfVec3d, t1 = double, t2 = double>\n"
        "\n"
        "r1 : GfRay\n"
        "s2 : GfLineSeg\n"
        "\n"
        "Computes the closest points between a ray and a line segment,\n"
        "returning a tuple. The first item in the tuple is true if "
        "they intersect. The two points are returned in p1 and p2.\n"
        "The parametric distance of each point on the ray and line\n"
        "segment is returned in t1 and t2.\n"
        "----------------------------------------------------------------------"
        );

    nanobind::class_<This>(m, "Ray", "")
        .def(nanobind::init<>())
        .def(nanobind::init<const GfVec3d &, const GfVec3d &>())

        .def(TfTypePythonClass())

        .def("SetPointAndDirection", [](This &self, const GfVec3d &startPoint, const GfVec3d &direction) -> This& {
            self.SetPointAndDirection(startPoint, direction);
            return self;
        }, nanobind::rv_policy::reference)
        .def("SetEnds", [](This &self, const GfVec3d &startPoint, const GfVec3d &endPoint) -> This& {
            self.SetEnds(startPoint, endPoint);
            return self;
        }, nanobind::rv_policy::reference)

        .def_prop_rw("startPoint", &This::GetStartPoint, SetStartPointHelper,
                     nanobind::rv_policy::copy)
        .def_prop_rw("direction", &This::GetDirection, SetDirectionHelper,
                     nanobind::rv_policy::copy)

        .def("GetPoint", &This::GetPoint)

        .def("FindClosestPoint", [](const GfRay &self, const GfVec3d &point) {
            double rayDist;
            GfVec3d result = self.FindClosestPoint(point, &rayDist);
            return nanobind::make_tuple(result, rayDist);
        })

        .def("Transform", &This::Transform, nanobind::rv_policy::reference)
        
        .def("Intersect", [](const GfRay &self, const GfVec3d &p0, const GfVec3d &p1, const GfVec3d &p2) {
            double dist = 0;
            GfVec3d barycentricCoords(0);
            bool frontFacing = false;
            bool result = self.Intersect(p0, p1, p2, &dist, &barycentricCoords, &frontFacing);
            return nanobind::make_tuple(result, dist, barycentricCoords, frontFacing);
        },
             "Intersect( p0, p1, p2 ) -> tuple<intersects = bool, dist =\n"
             "float, barycentric = GfVec3d, frontFacing = bool>")
        .def("Intersect", [](const GfRay &self, const GfPlane &plane) {
            double dist = 0;
            bool frontFacing = false;
            bool result = self.Intersect(plane, &dist, &frontFacing);
            return nanobind::make_tuple(result, dist, frontFacing);
        },
              "Intersect( plane ) -> tuple<intersects = bool, dist = float,\n"
              "frontFacing = bool>")
        .def("Intersect", [](const GfRay &self, const GfRange3d &box) {
            double enterDist = 0, exitDist = 0;
            bool result = self.Intersect(box, &enterDist, &exitDist);
            return nanobind::make_tuple(result, enterDist, exitDist);
        },
              "Intersect( range3d ) -> tuple<intersects = bool, enterDist\n"
              "= float, exitDist = float>")
        .def("Intersect", [](const GfRay &self, const GfBBox3d &box) {
            double enterDist = 0, exitDist = 0;
            bool result = self.Intersect(box, &enterDist, &exitDist);
            return nanobind::make_tuple(result, enterDist, exitDist);
        },
              "Intersect( bbox3d ) -> tuple<intersects = bool, enterDist\n"
              "= float, exitDist = float>")
        .def("Intersect", [](const GfRay &self, const GfVec3d& center, double radius) {
            double enterDist = 0, exitDist = 0;
            bool result = self.Intersect(center, radius, &enterDist, &exitDist);
            return nanobind::make_tuple(result, enterDist, exitDist);
        },
              "Intersect( center, radius ) -> tuple<intersects = bool,\n"
              "enterDist = float, exitDist = float>")
        .def("Intersect", [](const GfRay &self, const GfVec3d &origin, const GfVec3d &axis, double radius) {
            double enter = 0, exit = 0;
            bool result = self.Intersect(origin, axis, radius, &enter, &exit);
            return nanobind::make_tuple(result, enter, exit);
        },
              "Intersect( origin, axis, radius ) -> tuple<intersects = bool,\n"
              "enterDist = float, exitDist = float>")
        .def("Intersect", [](const GfRay &self, const GfVec3d &origin, const GfVec3d &axis, double radius, double height) {
            double enter = 0, exit = 0;
            bool result = self.Intersect(origin, axis, radius, height, &enter, &exit);
            return nanobind::make_tuple(result, enter, exit);
        },
              "Intersect( origin, axis, radius, height ) -> \n"
              "tuple<intersects = bool, enterDist = float, exitDist = float>")

        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self)

        .def("__repr__", _Repr)
        ;
}
