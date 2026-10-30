//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/line.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string _Repr(GfLine const &self) {
    return TF_PY_REPR_PREFIX + "Line(" + TfPyRepr(self.GetPoint(0.0)) + ", " +
        TfPyRepr(self.GetDirection()) + ")";
}

static void
SetDirectionHelper( GfLine &self, const GfVec3d &dir )
{
    self.Set( self.GetPoint(0.0), dir );
}

} // anonymous namespace 

void wrapLine(nanobind::module_ &m)
{    
    typedef GfLine This;

    m.def("FindClosestPoints", [](const GfLine &l1, const GfLine &l2) {
        GfVec3d p1(0), p2(0);
        double t1 = 0.0, t2 = 0.0;
        bool result = GfFindClosestPoints(l1, l2, &p1, &p2, &t1, &t2);
        return nanobind::make_tuple(result, p1, p2, t1, t2);
    },
        "FindClosestPoints( l1, l2 ) -> tuple<intersects = bool, p1 = GfVec3d, p2 = GfVec3d,"
        " t1 = double, t2 = double>\n"
        "\n"
        "l1 : GfLine\n"
        "l2 : GfLine\n"
        "\n"
        "Computes the closest points between two lines, returning a "
        "tuple.  The first item in the tuple is true if the lines"
        "intersect.  The two points are returned in p1 and p2.  The "
        "parametric distance of each point on the lines is returned "
        "in t1 and t2.\n"
        "----------------------------------------------------------------------" );
    
    nanobind::class_<This>(m, "Line", "Line class")
        .def(nanobind::init<>())
        .def(nanobind::init<const GfVec3d &, const GfVec3d &>())

        .def(TfTypePythonClass())

        .def("Set", &This::Set, nanobind::rv_policy::reference)

        .def("GetPoint", &This::GetPoint)

        .def("GetDirection", &This::GetDirection,
             nanobind::rv_policy::copy)

        .def_prop_rw("direction", &This::GetDirection, SetDirectionHelper,
                     nanobind::rv_policy::copy)

        .def("FindClosestPoint", [](const GfLine &self, const GfVec3d &point) {
            double t;
            GfVec3d result = self.FindClosestPoint(point, &t);
            return nanobind::make_tuple(result, t);
        })

        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self)

        .def("__repr__", _Repr)
        ;
}
