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
#include "pxr/base/gf/rotation.h"
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

void SetAxisHelper( GfRotation &rotation, const GfVec3d &axis )
{
    rotation.SetAxisAngle( axis, rotation.GetAngle() );
}

void SetAngleHelper( GfRotation &rotation, double angle )
{
    rotation.SetAxisAngle( rotation.GetAxis(), angle );
}

static tuple
_DecomposeRotation3(const GfMatrix4d &rot,
                    const GfVec3d &TwAxis,
                    const GfVec3d &FBAxis,
                    const GfVec3d &LRAxis,
                    double handedness,
                    double thetaTwHint,
                    double thetaFBHint,
                    double thetaLRHint,
                    bool useHint)
{
    double angle[3] = {thetaTwHint, thetaFBHint, thetaLRHint};

    GfRotation::DecomposeRotation(rot, TwAxis, FBAxis, LRAxis, handedness,
                                  &(angle[0]),
                                  &(angle[1]),
                                  &(angle[2]),
                                  NULL /* thetaSwHint */, 
                                  useHint,
                                  NULL /* swShift */);

    return make_tuple(angle[0], angle[1], angle[2]);
}

static tuple
_DecomposeRotation(const GfMatrix4d &rot,
                    const GfVec3d &TwAxis,
                    const GfVec3d &FBAxis,
                    const GfVec3d &LRAxis,
                    double handedness,
                    const object &thetaTwHint,
                    const object &thetaFBHint,
                    const object &thetaLRHint,
                    const object &thetaSwHint,
                    bool useHint,
                    const object & swShiftIn)
{
    double angle[4] = {
        thetaTwHint.ptr() != Py_None ? 
            boost::python::extract<double>(thetaTwHint) : 0.0,
        thetaFBHint.ptr() != Py_None ? 
            boost::python::extract<double>(thetaFBHint) : 0.0,
        thetaLRHint.ptr() != Py_None ? 
            boost::python::extract<double>(thetaLRHint) : 0.0,
        thetaSwHint.ptr() != Py_None ? 
            boost::python::extract<double>(thetaSwHint) : 0.0
        };
    double swShift = swShiftIn.ptr() != Py_None ? 
        boost::python::extract<double>(swShiftIn) : 0.0;

    GfRotation::DecomposeRotation(
        rot, TwAxis, FBAxis, LRAxis, handedness,
        thetaTwHint.ptr() != Py_None ? &(angle[0]) : NULL,
        thetaFBHint.ptr() != Py_None ? &(angle[1]) : NULL,
        thetaLRHint.ptr() != Py_None ? &(angle[2]) : NULL,
        thetaSwHint.ptr() != Py_None ? &(angle[3]) : NULL,
        useHint,
        swShiftIn.ptr() != Py_None ? &swShift : NULL);

    return make_tuple(angle[0], angle[1], angle[2], angle[3]);
}

static string _Repr(GfRotation const &self) {
    return TF_PY_REPR_PREFIX + "Rotation(" + TfPyRepr(self.GetAxis()) + ", " +
        TfPyRepr(self.GetAngle()) + ")";
}

void wrapRotation()
{    
    typedef GfRotation This;

    class_<This> ( "Rotation", "3-space rotation", init<>())

        .def(init<const GfVec3d &, double>())
        .def(init<const GfQuaternion &>())
        .def(init<const GfQuatd &>())
        .def(init<const GfVec3d &, const GfVec3d &>())

        .def( TfTypePythonClass() )

        .def("SetAxisAngle", &This::SetAxisAngle, return_self<>(),
             (args("axis"), args("angle")))
        .def("SetQuat", &This::SetQuat, return_self<>(), (args("quat")))
        .def("SetQuaternion", &This::SetQuaternion, return_self<>(),
             (args("quaternion")))
        .def("SetRotateInto", &This::SetRotateInto, return_self<>(),
             (args("rotateFrom"), args("rotateTo")))
        .def("SetIdentity", &This::SetIdentity, return_self<>())

        .add_property("axis", make_function(&This::GetAxis, return_value_policy<copy_const_reference>()),
                      SetAxisHelper )
        .add_property("angle", &This::GetAngle, SetAngleHelper)

        .def("GetAxis", &This::GetAxis, return_value_policy<copy_const_reference>())
        .def("GetAngle", &This::GetAngle)

        .def("GetQuaternion", &This::GetQuaternion)
        .def("GetQuat", &This::GetQuat)

        .def("GetInverse", &This::GetInverse)

        .def("Decompose", &This::Decompose)

        .def("DecomposeRotation3", _DecomposeRotation3,
             (arg("rot"),
              arg("twAxis"),
              arg("fbAxis"),
              arg("lrAxis"),
              arg("handedness"),
              arg("thetaTwHint") = 0.0,
              arg("thetaFBHint") = 0.0,
              arg("thetaLRHint") = 0.0,
              arg("useHint") = false)
             )
        .staticmethod("DecomposeRotation3")

        .def("DecomposeRotation", _DecomposeRotation,
             (arg("rot"),
              arg("twAxis"),
              arg("fbAxis"),
              arg("lrAxis"),
              arg("handedness"),
              arg("thetaTwHint"),
              arg("thetaFBHint"),
              arg("thetaLRHint"),
              arg("thetaSwHint") = object(),
              arg("useHint") = false,
              arg("swShift") = object())
             )
        .staticmethod("DecomposeRotation")

        .def("RotateOntoProjected", &This::RotateOntoProjected)
        .staticmethod("RotateOntoProjected")

        .def("TransformDir", (GfVec3f (This::*)( const GfVec3f & ) const)&This::TransformDir )
        .def("TransformDir", (GfVec3d (This::*)( const GfVec3d & ) const)&This::TransformDir )

        .def( str(self) )
        .def( self == self )
        .def( self != self )
        .def( self *= self )
        .def( self *= double() )
        .def( self /= double() )
        .def( self * self )
        .def( self * double() )
        .def( double() * self )
        .def( self / double() )

        .def("__repr__", _Repr)
        
        ;
    to_python_converter<std::vector<This>,
        TfPySequenceToPython<std::vector<This> > >();
    
}

PXR_NAMESPACE_CLOSE_SCOPE
