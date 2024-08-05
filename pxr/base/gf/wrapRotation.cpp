//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
                                  nullptr /* thetaSwHint */, 
                                  useHint,
                                  nullptr /* swShift */);

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
        thetaTwHint.ptr() != Py_None ? &(angle[0]) : nullptr,
        thetaFBHint.ptr() != Py_None ? &(angle[1]) : nullptr,
        thetaLRHint.ptr() != Py_None ? &(angle[2]) : nullptr,
        thetaSwHint.ptr() != Py_None ? &(angle[3]) : nullptr,
        useHint,
        swShiftIn.ptr() != Py_None ? &swShift : nullptr);

    return make_tuple(angle[0], angle[1], angle[2], angle[3]);
}

static tuple
_MatchClosestEulerRotation(
    const double targetTw,
    const double targetFB,
    const double targetLR,
    const double targetSw,
    const object &thetaTw,
    const object &thetaFB,
    const object &thetaLR,
    const object &thetaSw)
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

    return make_tuple(angle[0], angle[1], angle[2], angle[3]);
}

static string _Repr(GfRotation const &self) {
    return TF_PY_REPR_PREFIX + "Rotation(" + TfPyRepr(self.GetAxis()) + ", " +
        TfPyRepr(self.GetAngle()) + ")";
}

} // anonymous namespace 

static size_t __hash__(GfRotation const &self) {
    return TfHash()(self);
}

void wrapRotation()
{    
    typedef GfRotation This;

    class_<This> ( "Rotation", "3-space rotation", init<>())

        .def(init<const GfVec3d &, double>())
        .def(init<const GfQuaternion &>())
        .def(init<const GfQuatd &>())
        .def(init<const GfVec3d &, const GfVec3d &>())
        .def(init<const GfRotation &>())

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

        .def("MatchClosestEulerRotation", _MatchClosestEulerRotation)
        .staticmethod("MatchClosestEulerRotation")

        .def("RotateOntoProjected", &This::RotateOntoProjected)
        .staticmethod("RotateOntoProjected")

        .def("TransformDir", &This::TransformDir)

        // Provide wrapping that makes up for the fact that, in Python, we
        // don't allow implicit conversion from GfVec3f to GfVec3d (which we
        // do in C++).
        .def("TransformDir",
	     +[](const This &self, const GfVec3f &p) -> GfVec3d {
                 return self.TransformDir(p);
             })

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
        .def("__hash__", __hash__)

        ;
    to_python_converter<std::vector<This>,
        TfPySequenceToPython<std::vector<This> > >();
}
