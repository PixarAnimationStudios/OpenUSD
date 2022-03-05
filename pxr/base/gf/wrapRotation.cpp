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



PXR_NAMESPACE_USING_DIRECTIVE

namespace {

void SetAxisHelper( GfRotation &rotation, const GfVec3d &axis )
{
    rotation.SetAxisAngle( axis, rotation.GetAngle() );
}

void SetAngleHelper( GfRotation &rotation, double angle )
{
    rotation.SetAxisAngle( rotation.GetAxis(), angle );
}

static boost::python::tuple
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
                                  nullptr /* thetaSwHint */, 
                                  useHint,
                                  nullptr /* swShift */);

    return boost::python::make_tuple(angle[0], angle[1], angle[2]);
}

static boost::python::tuple
_DecomposeRotation(const GfMatrix4d &rot,
                    const GfVec3d &TwAxis,
                    const GfVec3d &FBAxis,
                    const GfVec3d &LRAxis,
                    double handedness,
                    const boost::python::object &thetaTwHint,
                    const boost::python::object &thetaFBHint,
                    const boost::python::object &thetaLRHint,
                    const boost::python::object &thetaSwHint,
                    bool useHint,
                    const boost::python::object & swShiftIn)
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
        thetaTwHint.ptr() != Py_None ? &(angle[0]) : nullptr,
        thetaFBHint.ptr() != Py_None ? &(angle[1]) : nullptr,
        thetaLRHint.ptr() != Py_None ? &(angle[2]) : nullptr,
        thetaSwHint.ptr() != Py_None ? &(angle[3]) : nullptr,
        useHint,
        swShiftIn.ptr() != Py_None ? &swShift : nullptr);

    return boost::python::make_tuple(angle[0], angle[1], angle[2], angle[3]);
}

static boost::python::tuple
_MatchClosestEulerRotation(
    const double targetTw,
    const double targetFB,
    const double targetLR,
    const double targetSw,
    const boost::python::object &thetaTw,
    const boost::python::object &thetaFB,
    const boost::python::object &thetaLR,
    const boost::python::object &thetaSw)
{
    double angle[4] = {
        thetaTw.ptr() != Py_None ?
            boost::python::extract<double>(thetaTw) : 0.0,
        thetaFB.ptr() != Py_None ?
            boost::python::extract<double>(thetaFB) : 0.0,
        thetaLR.ptr() != Py_None ?
            boost::python::extract<double>(thetaLR) : 0.0,
        thetaSw.ptr() != Py_None ?
            boost::python::extract<double>(thetaSw) : 0.0
        };

    GfRotation::MatchClosestEulerRotation(
        targetTw, targetFB, targetLR, targetSw,
        thetaTw.ptr() != Py_None ? &(angle[0]) : nullptr,
        thetaFB.ptr() != Py_None ? &(angle[1]) : nullptr,
        thetaLR.ptr() != Py_None ? &(angle[2]) : nullptr,
        thetaSw.ptr() != Py_None ? &(angle[3]) : nullptr);

    return boost::python::make_tuple(angle[0], angle[1], angle[2], angle[3]);
}

static std::string _Repr(GfRotation const &self) {
    return TF_PY_REPR_PREFIX + "Rotation(" + TfPyRepr(self.GetAxis()) + ", " +
        TfPyRepr(self.GetAngle()) + ")";
}

#if PY_MAJOR_VERSION == 2
static GfRotation __truediv__(const GfRotation &self, double value)
{
    return self / value;
}

static GfRotation __itruediv__(GfRotation &self, double value)
{
    return self /= value;
}
#endif

} // anonymous namespace 

void wrapRotation()
{    
    typedef GfRotation This;

    boost::python::class_<This> ( "Rotation", "3-space rotation", boost::python::init<>())

        .def(boost::python::init<const GfVec3d &, double>())
        .def(boost::python::init<const GfQuaternion &>())
        .def(boost::python::init<const GfQuatd &>())
        .def(boost::python::init<const GfVec3d &, const GfVec3d &>())

        .def( TfTypePythonClass() )

        .def("SetAxisAngle", &This::SetAxisAngle, boost::python::return_self<>(),
             (boost::python::args("axis"), boost::python::args("angle")))
        .def("SetQuat", &This::SetQuat, boost::python::return_self<>(), (boost::python::args("quat")))
        .def("SetQuaternion", &This::SetQuaternion, boost::python::return_self<>(),
             (boost::python::args("quaternion")))
        .def("SetRotateInto", &This::SetRotateInto, boost::python::return_self<>(),
             (boost::python::args("rotateFrom"), boost::python::args("rotateTo")))
        .def("SetIdentity", &This::SetIdentity, boost::python::return_self<>())

        .add_property("axis", boost::python::make_function(&This::GetAxis, boost::python::return_value_policy<boost::python::copy_const_reference>()),
                      SetAxisHelper )
        .add_property("angle", &This::GetAngle, SetAngleHelper)

        .def("GetAxis", &This::GetAxis, boost::python::return_value_policy<boost::python::copy_const_reference>())
        .def("GetAngle", &This::GetAngle)

        .def("GetQuaternion", &This::GetQuaternion)
        .def("GetQuat", &This::GetQuat)

        .def("GetInverse", &This::GetInverse)

        .def("Decompose", &This::Decompose)

        .def("DecomposeRotation3", _DecomposeRotation3,
             (boost::python::arg("rot"),
              boost::python::arg("twAxis"),
              boost::python::arg("fbAxis"),
              boost::python::arg("lrAxis"),
              boost::python::arg("handedness"),
              boost::python::arg("thetaTwHint") = 0.0,
              boost::python::arg("thetaFBHint") = 0.0,
              boost::python::arg("thetaLRHint") = 0.0,
              boost::python::arg("useHint") = false)
             )
        .staticmethod("DecomposeRotation3")

        .def("DecomposeRotation", _DecomposeRotation,
             (boost::python::arg("rot"),
              boost::python::arg("twAxis"),
              boost::python::arg("fbAxis"),
              boost::python::arg("lrAxis"),
              boost::python::arg("handedness"),
              boost::python::arg("thetaTwHint"),
              boost::python::arg("thetaFBHint"),
              boost::python::arg("thetaLRHint"),
              boost::python::arg("thetaSwHint") = boost::python::object(),
              boost::python::arg("useHint") = false,
              boost::python::arg("swShift") = boost::python::object())
             )
        .staticmethod("DecomposeRotation")

        .def("MatchClosestEulerRotation", _MatchClosestEulerRotation)
        .staticmethod("MatchClosestEulerRotation")

        .def("RotateOntoProjected", &This::RotateOntoProjected)
        .staticmethod("RotateOntoProjected")

        .def("TransformDir", (GfVec3f (This::*)( const GfVec3f & ) const)&This::TransformDir )
        .def("TransformDir", (GfVec3d (This::*)( const GfVec3d & ) const)&This::TransformDir )

        .def( boost::python::self_ns::str(boost::python::self) )
        .def( boost::python::self == boost::python::self )
        .def( boost::python::self != boost::python::self )
        .def( boost::python::self *= boost::python::self )
        .def( boost::python::self *= double() )
        .def( boost::python::self /= double() )
        .def( boost::python::self * boost::python::self )
        .def( boost::python::self * double() )
        .def( double() * boost::python::self )
        .def( boost::python::self / double() )

 #if PY_MAJOR_VERSION == 2
        // Needed only to support "from __future__ import division" in
        // python 2. In python 3 builds boost::python adds this for us.
        .def("__truediv__", __truediv__ )
        .def("__itruediv__", __itruediv__ )
#endif

       .def("__repr__", _Repr)
        
        ;
    boost::python::to_python_converter<std::vector<This>,
        TfPySequenceToPython<std::vector<This> > >();
}
