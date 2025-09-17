//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/transform.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python/args.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/copy_const_reference.hpp"
#include "pxr/external/boost/python/init.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/return_arg.hpp"

#include <string>

using std::string;
using std::vector;
PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static GfVec3d _NoTranslation() { return GfVec3d(0,0,0); }
static GfVec3d _IdentityScale() { return GfVec3d(1,1,1); }
static GfRotation _NoRotation() { return GfRotation( GfVec3d::XAxis(), 0.0 ); }

static string _Repr(GfTransform const &self)
{
    string prefix = TF_PY_REPR_PREFIX + "Transform(";
    string indent(prefix.size(), ' ');

    // Use keyword args for clarity.
    // Only use args that do not match the defaults.
    vector<string> kwargs;
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

    return prefix + TfStringJoin(kwargs, string(", \n" + indent).c_str()) + ")";
}

} // anonymous namespace 

void wrapTransform()
{    
    typedef GfTransform This;

    class_<This>( "Transform", init<>() )

        .def(init<const GfVec3d&, const GfRotation&, const GfVec3d&,
                  const GfVec3d&, const GfRotation&>
             ((args("translation") = _NoTranslation(),
               args("rotation") = _NoRotation(),
               args("scale") = _IdentityScale(),
               args("pivotPosition") = _NoTranslation(),
               args("pivotOrientation") = _NoRotation()),
              "Initializer used by 3x code."))

        // This is the constructor used by 2x code.  Leave the initial
        // arguments as non-default to force the user to provide enough
        // values to indicate her intentions.
        .def(init<const GfVec3d &, const GfRotation &, const GfRotation&,
                  const GfVec3d &, const GfVec3d &>
             ((args("scale"),
               args("pivotOrientation"),
               args("rotation"),
               args("pivotPosition"),
               args("translation")),
              "Initializer used by old 2x code. (Deprecated)"))

        .def(init<const GfMatrix4d &>())

        .def( TfTypePythonClass() )

        .def( "Set",
              (This & (This::*)( const GfVec3d &, const GfRotation &,
                                 const GfVec3d &, const GfVec3d &,
                                 const GfRotation & ))( &This::Set ),
              return_self<>(),
             (args("translation") = _NoTranslation(),
              args("rotation") = _NoRotation(),
              args("scale") = _IdentityScale(),
              args("pivotPosition") = _NoTranslation(),
              args("pivotOrientation") = _NoRotation()))

        .def( "Set",
              (This & (This::*)( const GfVec3d &, const GfRotation &,
                                 const GfRotation &, const GfVec3d &,
                                 const GfVec3d &))&This::Set,
              return_self<>(),
              (args("scale"),
               args("pivotOrientation"),
               args("rotation"),
               args("pivotPosition"),
               args("translation")),
              "Set method used by old 2x code. (Deprecated)")

        .def( "SetMatrix", &This::SetMatrix, return_self<>() )
        .def( "GetMatrix", &This::GetMatrix )

        .def( "SetIdentity", &This::SetIdentity, return_self<>() )

        .add_property( "translation", make_function
                       (&This::GetTranslation,
                        return_value_policy<return_by_value>()),
                       &This::SetTranslation )

        .add_property( "rotation", make_function
                       (&This::GetRotation,
                        return_value_policy<return_by_value>()),
                       &This::SetRotation )

        .add_property( "scale", make_function
                       (&This::GetScale,
                        return_value_policy<return_by_value>()),
                       &This::SetScale )

        .add_property( "pivotPosition", make_function
                       (&This::GetPivotPosition,
                        return_value_policy<return_by_value>()),
                       &This::SetPivotPosition )

        .add_property( "pivotOrientation", make_function
                       (&This::GetPivotOrientation,
                        return_value_policy<return_by_value>()),
                       &This::SetPivotOrientation )

        .def("GetTranslation", &This::GetTranslation,
             return_value_policy<return_by_value>())
        .def("SetTranslation", &This::SetTranslation )

        .def("GetRotation", &This::GetRotation,
             return_value_policy<return_by_value>())
        .def("SetRotation", &This::SetRotation)

        .def("GetScale", &This::GetScale,
             return_value_policy<return_by_value>())
        .def("SetScale", &This::SetScale )

        .def("GetPivotPosition", &This::GetPivotPosition,
             return_value_policy<return_by_value>())
        .def("SetPivotPosition", &This::SetPivotPosition )

        .def("GetPivotOrientation", &This::GetPivotOrientation,
             return_value_policy<return_by_value>())
        .def("SetPivotOrientation", &This::SetPivotOrientation )


        .def( str(self) )
        .def( self == self )
        .def( self != self )
        .def( self *= self )
        .def( self * self )
        
        .def("__repr__", _Repr)

        ;
    
}
