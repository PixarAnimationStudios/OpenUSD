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
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/transform.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python/args.hpp>
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/init.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>

#include <string>


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrBaseGfWrapTransform {

static GfVec3d _NoTranslation() { return GfVec3d(0,0,0); }
static GfVec3d _IdentityScale() { return GfVec3d(1,1,1); }
static GfRotation _NoRotation() { return GfRotation( GfVec3d::XAxis(), 0.0 ); }

static std::string _Repr(GfTransform const &self)
{
    std::string prefix = TF_PY_REPR_PREFIX + "Transform(";
    std::string indent(prefix.size(), ' ');

    // Use keyword args for clarity.
    // Only use args that do not match the defaults.
    std::vector<std::string> kwargs;
    if (self.GetTranslation() != _NoTranslation())
        kwargs.push_back( "translation = " +
                          TfPyRepr(self.GetTranslation()) );
    if (self.GetRotation() != _NoRotation())
        kwargs.push_back( "rotation = " +
                          TfPyRepr(self.GetRotation()) );
    if (self.GetScale() != _IdentityScale())
        kwargs.push_back( "scale = " +
                          TfPyRepr(self.GetScale()) );
    if (self.GetPivotPosition() != _NoTranslation())
        kwargs.push_back( "pivotPosition = " +
                          TfPyRepr(self.GetPivotPosition()) );
    if (self.GetPivotOrientation() != _NoRotation())
        kwargs.push_back( "pivotOrientation = " +
                          TfPyRepr(self.GetPivotOrientation()) );

    return prefix + TfStringJoin(kwargs, std::string(", \n" + indent).c_str()) + ")";
}

} // anonymous namespace 

void wrapTransform()
{    
    typedef GfTransform This;

    boost::python::class_<This>( "Transform", boost::python::init<>() )

        .def(boost::python::init<const GfVec3d&, const GfRotation&, const GfVec3d&,
                  const GfVec3d&, const GfRotation&>
             ((boost::python::args("translation") = pxrBaseGfWrapTransform::_NoTranslation(),
               boost::python::args("rotation") = pxrBaseGfWrapTransform::_NoRotation(),
               boost::python::args("scale") = pxrBaseGfWrapTransform::_IdentityScale(),
               boost::python::args("pivotPosition") = pxrBaseGfWrapTransform::_NoTranslation(),
               boost::python::args("pivotOrientation") = pxrBaseGfWrapTransform::_NoRotation()),
              "Initializer used by 3x code."))

        // This is the constructor used by 2x code.  Leave the initial
        // arguments as non-default to force the user to provide enough
        // values to indicate her intentions.
        .def(boost::python::init<const GfVec3d &, const GfRotation &, const GfRotation&,
                  const GfVec3d &, const GfVec3d &>
             ((boost::python::args("scale"),
               boost::python::args("pivotOrientation"),
               boost::python::args("rotation"),
               boost::python::args("pivotPosition"),
               boost::python::args("translation")),
              "Initializer used by old 2x code. (Deprecated)"))

        .def(boost::python::init<const GfMatrix4d &>())

        .def( TfTypePythonClass() )

        .def( "Set",
              (This & (This::*)( const GfVec3d &, const GfRotation &,
                                 const GfVec3d &, const GfVec3d &,
                                 const GfRotation & ))( &This::Set ),
              boost::python::return_self<>(),
             (boost::python::args("translation") = pxrBaseGfWrapTransform::_NoTranslation(),
              boost::python::args("rotation") = pxrBaseGfWrapTransform::_NoRotation(),
              boost::python::args("scale") = pxrBaseGfWrapTransform::_IdentityScale(),
              boost::python::args("pivotPosition") = pxrBaseGfWrapTransform::_NoTranslation(),
              boost::python::args("pivotOrientation") = pxrBaseGfWrapTransform::_NoRotation()))

        .def( "Set",
              (This & (This::*)( const GfVec3d &, const GfRotation &,
                                 const GfRotation &, const GfVec3d &,
                                 const GfVec3d &))&This::Set,
              boost::python::return_self<>(),
              (boost::python::args("scale"),
               boost::python::args("pivotOrientation"),
               boost::python::args("rotation"),
               boost::python::args("pivotPosition"),
               boost::python::args("translation")),
              "Set method used by old 2x code. (Deprecated)")

        .def( "SetMatrix", &This::SetMatrix, boost::python::return_self<>() )
        .def( "GetMatrix", &This::GetMatrix )

        .def( "SetIdentity", &This::SetIdentity, boost::python::return_self<>() )

        .add_property( "translation", boost::python::make_function
                       (&This::GetTranslation,
                        boost::python::return_value_policy<boost::python::return_by_value>()),
                       &This::SetTranslation )

        .add_property( "rotation", boost::python::make_function
                       (&This::GetRotation,
                        boost::python::return_value_policy<boost::python::return_by_value>()),
                       &This::SetRotation )

        .add_property( "scale", boost::python::make_function
                       (&This::GetScale,
                        boost::python::return_value_policy<boost::python::return_by_value>()),
                       &This::SetScale )

        .add_property( "pivotPosition", boost::python::make_function
                       (&This::GetPivotPosition,
                        boost::python::return_value_policy<boost::python::return_by_value>()),
                       &This::SetPivotPosition )

        .add_property( "pivotOrientation", boost::python::make_function
                       (&This::GetPivotOrientation,
                        boost::python::return_value_policy<boost::python::return_by_value>()),
                       &This::SetPivotOrientation )

        .def("GetTranslation", &This::GetTranslation,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("SetTranslation", &This::SetTranslation )

        .def("GetRotation", &This::GetRotation,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("SetRotation", &This::SetRotation)

        .def("GetScale", &This::GetScale,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("SetScale", &This::SetScale )

        .def("GetPivotPosition", &This::GetPivotPosition,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("SetPivotPosition", &This::SetPivotPosition )

        .def("GetPivotOrientation", &This::GetPivotOrientation,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("SetPivotOrientation", &This::SetPivotOrientation )


        .def( boost::python::self_ns::str(boost::python::self) )
        .def( boost::python::self == boost::python::self )
        .def( boost::python::self != boost::python::self )
        .def( boost::python::self *= boost::python::self )
        .def( boost::python::self * boost::python::self )
        
        .def("__repr__", pxrBaseGfWrapTransform::_Repr)

        ;
    
}
