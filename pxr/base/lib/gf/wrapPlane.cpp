//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>

#include <string>

using namespace boost::python;

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

static string _Repr(GfPlane const &self) {
    return TF_PY_REPR_PREFIX + "Plane(" + TfPyRepr(self.GetNormal()) + ", " +
        TfPyRepr(self.GetDistanceFromOrigin()) + ")";
}

void wrapPlane()
{    
    typedef GfPlane This;

    object getNormal = make_function(&This::GetNormal,
                                     return_value_policy<return_by_value>());

    class_<This>( "Plane", init<>() )
        .def(init< const GfVec3d &, double >())
        .def(init< const GfVec3d &, const GfVec3d & >())
        .def(init< const GfVec3d &, const GfVec3d &, const GfVec3d & >())

        .def( TfTypePythonClass() )

        .def("Set", (void (This::*)(const GfVec3d &, double))
             &This::Set, return_self<>())
        .def("Set", (void (This::*)(const GfVec3d &, const GfVec3d &))
             &This::Set, return_self<>())
        .def("Set", (void (This::*)( const GfVec3d &, const GfVec3d &,
                                     const GfVec3d & ))
             &This::Set, return_self<>())

        .add_property( "normal", getNormal)
        .add_property( "distanceFromOrigin", &This::GetDistanceFromOrigin )

        .def( "GetDistance", &This::GetDistance )
        .def( "GetDistanceFromOrigin", &This::GetDistanceFromOrigin )
        .def( "GetNormal", getNormal)
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

PXR_NAMESPACE_CLOSE_SCOPE
