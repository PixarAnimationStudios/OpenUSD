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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/copy_const_reference.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/return_arg.hpp"
#include "pxr/external/boost/python/tuple.hpp"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static string _Repr(GfLineSeg const &self) {
    return TF_PY_REPR_PREFIX + "LineSeg(" + TfPyRepr(self.GetPoint(0.0)) + ", " +
        TfPyRepr(self.GetPoint(1.0)) + ")";
}


static tuple
FindClosestPointsHelper1( const GfLine &l1, const GfLineSeg &l2 )
{
    GfVec3d p1(0), p2(0);
    double t1 = 0, t2 = 0;
    bool result = GfFindClosestPoints( l1, l2, &p1, &p2, &t1, &t2 );
    return pxr_boost::python::make_tuple( result, p1, p2, t1, t2 );
}

static tuple
FindClosestPointsHelper2( const GfLineSeg &l1, const GfLineSeg &l2 )
{
    GfVec3d p1(0), p2(0);
    double t1 = 0, t2 = 0;
    bool result = GfFindClosestPoints( l1, l2, &p1, &p2, &t1, &t2 );
    return pxr_boost::python::make_tuple( result, p1, p2, t1, t2 );
}

static tuple
FindClosestPointHelper( const GfLineSeg &self, const GfVec3d &point )
{
    double t;
    GfVec3d p1 = self.FindClosestPoint( point, &t );
    return pxr_boost::python::make_tuple( p1, t );
}

} // anonymous namespace 

void wrapLineSeg()
{    
    typedef GfLineSeg This;

    def("FindClosestPoints", FindClosestPointsHelper1, 
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
    def("FindClosestPoints", FindClosestPointsHelper2, 
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
    
    object getDirection = make_function(&This::GetDirection,
                                        return_value_policy<return_by_value>());

    class_<This>( "LineSeg", "Line segment class", init<>() )
        .def(init< const GfVec3d &, const GfVec3d & >())

        .def( TfTypePythonClass() )

        .def("GetDirection", getDirection)
        .def("GetLength", &This::GetLength)
        .def("GetPoint", &This::GetPoint)

        .add_property("direction", getDirection)
        .add_property("length", &This::GetLength)

        .def( "FindClosestPoint", FindClosestPointHelper )

        .def( str(self) )
        .def( self == self )
        .def( self != self )

        .def("__repr__", _Repr)
        ;
    
}
