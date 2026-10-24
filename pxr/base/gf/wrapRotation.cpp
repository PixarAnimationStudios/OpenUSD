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

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/vector.h>

#include <string>

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

static string _Repr(GfRotation const &self) {
    return TF_PY_REPR_PREFIX + "Rotation(" + TfPyRepr(self.GetAxis()) + ", " +
        TfPyRepr(self.GetAngle()) + ")";
}

} // anonymous namespace 

static size_t __hash__(GfRotation const &self) {
    return TfHash()(self);
}

void wrapRotation(nanobind::module_ &m)
{    
    typedef GfRotation This;

    nanobind::class_<This>(m, "Rotation", "3-space rotation")
        .def(nanobind::init<>())
        .def(nanobind::init<const GfVec3d &, double>())
        .def(nanobind::init<const GfQuaternion &>())
        .def(nanobind::init<const GfQuatd &>())
        .def(nanobind::init<const GfVec3d &, const GfVec3d &>())
        .def(nanobind::init<const GfRotation &>())

        .def(TfTypePythonClass())

        .def("SetAxisAngle", &This::SetAxisAngle, nanobind::rv_policy::reference,
             nanobind::arg("axis"), nanobind::arg("angle"))
        .def("SetQuat", &This::SetQuat, nanobind::rv_policy::reference,
             nanobind::arg("quat"))
        .def("SetQuaternion", &This::SetQuaternion, nanobind::rv_policy::reference,
             nanobind::arg("quaternion"))
        .def("SetRotateInto", &This::SetRotateInto, nanobind::rv_policy::reference,
             nanobind::arg("rotateFrom"), nanobind::arg("rotateTo"))
        .def("SetIdentity", &This::SetIdentity, nanobind::rv_policy::reference)

        .def_prop_rw("axis", &This::GetAxis, SetAxisHelper, nanobind::rv_policy::copy)
        .def_prop_rw("angle", &This::GetAngle, SetAngleHelper)

        .def("GetAxis", &This::GetAxis, nanobind::rv_policy::copy)
        .def("GetAngle", &This::GetAngle)

        .def("GetQuaternion", &This::GetQuaternion)
        .def("GetQuat", &This::GetQuat)

        .def("GetInverse", &This::GetInverse)

        .def("Decompose", &This::Decompose)

        .def_static("DecomposeRotation3", [](const GfMatrix4d &rot,
                    const GfVec3d &TwAxis, const GfVec3d &FBAxis, const GfVec3d &LRAxis,
                    double handedness, double thetaTwHint, double thetaFBHint,
                    double thetaLRHint, bool useHint) {
            double angle[3] = {thetaTwHint, thetaFBHint, thetaLRHint};
            GfRotation::DecomposeRotation(rot, TwAxis, FBAxis, LRAxis, handedness,
                                          &(angle[0]), &(angle[1]), &(angle[2]),
                                          nullptr, useHint, nullptr);
            return nanobind::make_tuple(angle[0], angle[1], angle[2]);
        },
             nanobind::arg("rot"),
             nanobind::arg("twAxis"),
             nanobind::arg("fbAxis"),
             nanobind::arg("lrAxis"),
             nanobind::arg("handedness"),
             nanobind::arg("thetaTwHint") = 0.0,
             nanobind::arg("thetaFBHint") = 0.0,
             nanobind::arg("thetaLRHint") = 0.0,
             nanobind::arg("useHint") = false)

        .def_static("DecomposeRotation", [](const GfMatrix4d &rot,
                    const GfVec3d &TwAxis, const GfVec3d &FBAxis, const GfVec3d &LRAxis,
                    double handedness, nanobind::object thetaTwHint, nanobind::object thetaFBHint,
                    nanobind::object thetaLRHint, nanobind::object thetaSwHint,
                    bool useHint, nanobind::object swShiftIn) {
            double angle[4] = {
                !thetaTwHint.is_none() ? nanobind::cast<double>(thetaTwHint) : 0.0,
                !thetaFBHint.is_none() ? nanobind::cast<double>(thetaFBHint) : 0.0,
                !thetaLRHint.is_none() ? nanobind::cast<double>(thetaLRHint) : 0.0,
                !thetaSwHint.is_none() ? nanobind::cast<double>(thetaSwHint) : 0.0
            };
            double swShift = !swShiftIn.is_none() ? nanobind::cast<double>(swShiftIn) : 0.0;
            GfRotation::DecomposeRotation(
                rot, TwAxis, FBAxis, LRAxis, handedness,
                !thetaTwHint.is_none() ? &(angle[0]) : nullptr,
                !thetaFBHint.is_none() ? &(angle[1]) : nullptr,
                !thetaLRHint.is_none() ? &(angle[2]) : nullptr,
                !thetaSwHint.is_none() ? &(angle[3]) : nullptr,
                useHint,
                !swShiftIn.is_none() ? &swShift : nullptr);
            return nanobind::make_tuple(angle[0], angle[1], angle[2], angle[3]);
        },
             nanobind::arg("rot"),
             nanobind::arg("twAxis"),
             nanobind::arg("fbAxis"),
             nanobind::arg("lrAxis"),
             nanobind::arg("handedness"),
             nanobind::arg("thetaTwHint").none(),
             nanobind::arg("thetaFBHint").none(),
             nanobind::arg("thetaLRHint").none(),
             nanobind::arg("thetaSwHint") = nanobind::none(),
             nanobind::arg("useHint") = false,
             nanobind::arg("swShift") = nanobind::none())

        .def_static("MatchClosestEulerRotation", [](double targetTw, double targetFB,
                    double targetLR, double targetSw, nanobind::handle thetaTw,
                    nanobind::handle thetaFB, nanobind::handle thetaLR, nanobind::handle thetaSw) {
            double angle[4] = {
                !thetaTw.is_none() ? nanobind::cast<double>(thetaTw) : 0.0,
                !thetaFB.is_none() ? nanobind::cast<double>(thetaFB) : 0.0,
                !thetaLR.is_none() ? nanobind::cast<double>(thetaLR) : 0.0,
                !thetaSw.is_none() ? nanobind::cast<double>(thetaSw) : 0.0
            };
            GfRotation::MatchClosestEulerRotation(
                targetTw, targetFB, targetLR, targetSw,
                !thetaTw.is_none() ? &(angle[0]) : nullptr,
                !thetaFB.is_none() ? &(angle[1]) : nullptr,
                !thetaLR.is_none() ? &(angle[2]) : nullptr,
                !thetaSw.is_none() ? &(angle[3]) : nullptr);
            return nanobind::make_tuple(angle[0], angle[1], angle[2], angle[3]);
        },
             nanobind::arg("targetTw"),
             nanobind::arg("targetFB"),
             nanobind::arg("targetLR"),
             nanobind::arg("targetSw"),
             nanobind::arg("thetaTw").none(),
             nanobind::arg("thetaFB").none(),
             nanobind::arg("thetaLR").none(),
             nanobind::arg("thetaSw").none())

        .def_static("RotateOntoProjected", &This::RotateOntoProjected)

        .def("TransformDir", &This::TransformDir)

        // Provide wrapping that makes up for the fact that, in Python, we
        // don't allow implicit conversion from GfVec3f to GfVec3d (which we
        // do in C++).
        .def("TransformDir",
             +[](const This &self, const GfVec3f &p) -> GfVec3d {
                 return self.TransformDir(p);
             })

        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self)
        .def(nanobind::self *= nanobind::self)
        .def(nanobind::self *= double())
        .def(nanobind::self /= double())
        .def(nanobind::self * nanobind::self)
        .def(nanobind::self * double())
        .def(double() * nanobind::self)
        .def(nanobind::self / double())

        .def("__repr__", _Repr)
        .def("__hash__", __hash__)
        ;
}
