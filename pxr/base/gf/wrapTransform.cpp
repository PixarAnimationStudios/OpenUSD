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

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

#include <string>
#include <vector>

using std::string;
using std::vector;
PXR_NAMESPACE_USING_DIRECTIVE

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

void wrapTransform(nanobind::module_ &m)
{    
    typedef GfTransform This;

    nanobind::class_<This>(m, "Transform")
        .def(nanobind::init<>())

        .def(nanobind::init<const GfVec3d&, const GfRotation&, const GfVec3d&,
                  const GfVec3d&, const GfRotation&>(),
             nanobind::arg("translation") = _NoTranslation(),
             nanobind::arg("rotation") = _NoRotation(),
             nanobind::arg("scale") = _IdentityScale(),
             nanobind::arg("pivotPosition") = _NoTranslation(),
             nanobind::arg("pivotOrientation") = _NoRotation(),
             "Initializer used by 3x code.")

        // This is the constructor used by 2x code.  Leave the initial
        // arguments as non-default to force the user to provide enough
        // values to indicate her intentions.
        .def(nanobind::init<const GfVec3d &, const GfRotation &, const GfRotation&,
                  const GfVec3d &, const GfVec3d &>(),
             nanobind::arg("scale"),
             nanobind::arg("pivotOrientation"),
             nanobind::arg("rotation"),
             nanobind::arg("pivotPosition"),
             nanobind::arg("translation"),
             "Initializer used by old 2x code. (Deprecated)")

        .def(nanobind::init<const GfMatrix4d &>())

        .def(TfTypePythonClass())

        .def( "Set",
              (This & (This::*)( const GfVec3d &, const GfRotation &,
                                 const GfVec3d &, const GfVec3d &,
                                 const GfRotation & ))( &This::Set ),
              nanobind::rv_policy::reference,
              nanobind::arg("translation") = _NoTranslation(),
              nanobind::arg("rotation") = _NoRotation(),
              nanobind::arg("scale") = _IdentityScale(),
              nanobind::arg("pivotPosition") = _NoTranslation(),
              nanobind::arg("pivotOrientation") = _NoRotation())

        .def( "Set",
              (This & (This::*)( const GfVec3d &, const GfRotation &,
                                 const GfRotation &, const GfVec3d &,
                                 const GfVec3d &))&This::Set,
              nanobind::rv_policy::reference,
              nanobind::arg("scale"),
              nanobind::arg("pivotOrientation"),
              nanobind::arg("rotation"),
              nanobind::arg("pivotPosition"),
              nanobind::arg("translation"),
              "Set method used by old 2x code. (Deprecated)")

        .def( "SetMatrix", &This::SetMatrix, nanobind::rv_policy::reference )
        .def( "GetMatrix", &This::GetMatrix )

        .def( "SetIdentity", &This::SetIdentity, nanobind::rv_policy::reference )

        .def_prop_rw( "translation", &This::GetTranslation,
                      &This::SetTranslation, nanobind::rv_policy::copy )

        .def_prop_rw( "rotation", &This::GetRotation,
                      &This::SetRotation, nanobind::rv_policy::copy )

        .def_prop_rw( "scale", &This::GetScale,
                      &This::SetScale, nanobind::rv_policy::copy )

        .def_prop_rw( "pivotPosition", &This::GetPivotPosition,
                      &This::SetPivotPosition, nanobind::rv_policy::copy )

        .def_prop_rw( "pivotOrientation", &This::GetPivotOrientation,
                      &This::SetPivotOrientation, nanobind::rv_policy::copy )

        .def("GetTranslation", &This::GetTranslation,
             nanobind::rv_policy::copy)
        .def("SetTranslation", &This::SetTranslation )

        .def("GetRotation", &This::GetRotation,
             nanobind::rv_policy::copy)
        .def("SetRotation", &This::SetRotation)

        .def("GetScale", &This::GetScale,
             nanobind::rv_policy::copy)
        .def("SetScale", &This::SetScale )

        .def("GetPivotPosition", &This::GetPivotPosition,
             nanobind::rv_policy::copy)
        .def("SetPivotPosition", &This::SetPivotPosition )

        .def("GetPivotOrientation", &This::GetPivotOrientation,
             nanobind::rv_policy::copy)
        .def("SetPivotOrientation", &This::SetPivotOrientation )


        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def( nanobind::self == nanobind::self )
        .def( nanobind::self != nanobind::self )
        .def( nanobind::self *= nanobind::self )
        .def( nanobind::self * nanobind::self )
        
        .def("__repr__", _Repr)

        ;
    
}
