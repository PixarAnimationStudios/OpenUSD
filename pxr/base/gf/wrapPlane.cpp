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

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/def.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>

#include <string>

using namespace boost::python;

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string _Repr(GfPlane const &self) {
    return TF_PY_REPR_PREFIX + "Plane(" + TfPyRepr(self.GetNormal()) + ", " +
        TfPyRepr(self.GetDistanceFromOrigin()) + ")";
}

static object _FitPlaneToPoints(const std::vector<GfVec3d>& points) {
    GfPlane plane;
    return GfFitPlaneToPoints(points, &plane) ? object(plane) : object();
}

} // anonymous namespace 

void wrapPlane()
{    
    typedef GfPlane This;

    object getNormal = make_function(&This::GetNormal,
                                     return_value_policy<return_by_value>());

    def( "FitPlaneToPoints", _FitPlaneToPoints );

    class_<This>( "Plane", init<>() )
        .def(init< const GfVec3d &, double >())
        .def(init< const GfVec3d &, const GfVec3d & >())
        .def(init< const GfVec3d &, const GfVec3d &, const GfVec3d & >())
        .def(init< const GfVec4d & >())

        .def( TfTypePythonClass() )

        .def("Set", (void (This::*)(const GfVec3d &, double))
             &This::Set, return_self<>())
        .def("Set", (void (This::*)(const GfVec3d &, const GfVec3d &))
             &This::Set, return_self<>())
        .def("Set", (void (This::*)( const GfVec3d &, const GfVec3d &,
                                     const GfVec3d & ))
             &This::Set, return_self<>())
        .def("Set", (void (This::*)(const GfVec4d &))
             &This::Set, return_self<>())

        .add_property( "normal", getNormal)
        .add_property( "distanceFromOrigin", &This::GetDistanceFromOrigin )

        .def( "GetDistance", &This::GetDistance )
        .def( "GetDistanceFromOrigin", &This::GetDistanceFromOrigin )
        .def( "GetNormal", getNormal)
        .def( "GetEquation", &This::GetEquation )
        .def( "Project", &This::Project )

        .def( "Transform", &This::Transform, return_self<>() )

        .def( "Reorient", &This::Reorient, return_self<>() )

        .def( "IntersectsPositiveHalfSpace",
              (bool (This::*)( const GfRange3d & ) const)
              &This::IntersectsPositiveHalfSpace )

        .def( "IntersectsPositiveHalfSpace",
              (bool (This::*)( const GfVec3d & ) const)
              &This::IntersectsPositiveHalfSpace )

        .def( str(self) )
        .def( self == self )
        .def( self != self )

        .def("__repr__", _Repr)
        
        ;
    to_python_converter<std::vector<This>,
        TfPySequenceToPython<std::vector<This> > >();
    
}
