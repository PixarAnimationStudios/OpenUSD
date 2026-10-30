//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/skinningQuery.h"

#include "pxr/base/gf/interval.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/usd/usdGeom/boundable.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

nb::object
_ComputeJointInfluences(const UsdSkelSkinningQuery& self, UsdTimeCode time)
{
    VtIntArray indices;
    VtFloatArray weights;
    if (self.ComputeJointInfluences(&indices, &weights, time)) {
        return nb::make_tuple(indices, weights);
    }
    return nb::none();
}

nb::object
_ComputeVaryingJointInfluences(const UsdSkelSkinningQuery& self,
                               size_t numPoints,
                               UsdTimeCode time)
{
    VtIntArray indices;
    VtFloatArray weights;
    if (self.ComputeVaryingJointInfluences(numPoints, &indices,
                                          &weights, time)) {
        return nb::make_tuple(indices, weights);
    }
    return nb::none();
}

std::vector<double>
_GetTimeSamples(const UsdSkelSkinningQuery& self)
{
    std::vector<double> times;
    self.GetTimeSamples(&times);
    return times;
}

std::vector<double>
_GetTimeSamplesInInterval(const UsdSkelSkinningQuery& self,
                          const GfInterval& interval)
{
    std::vector<double> times;
    self.GetTimeSamplesInInterval(interval, &times);
    return times;
}

nb::object
_GetJointOrder(const UsdSkelSkinningQuery& self)
{
    VtTokenArray jointOrder;
    if (self.GetJointOrder(&jointOrder))
        return nb::cast(jointOrder);
    return nb::none();
}

nb::object
_GetBlendShapeOrder(const UsdSkelSkinningQuery& self)
{
    VtTokenArray blendShapeOrder;
    if (self.GetBlendShapeOrder(&blendShapeOrder))
        return nb::cast(blendShapeOrder);
    return nb::none();
}

template <typename Matrix4>
bool
_ComputeSkinnedPoints(const UsdSkelSkinningQuery& self,
                      const VtArray<Matrix4>& xforms,
                      VtVec3fArray& points,
                      UsdTimeCode time)
{
    return self.ComputeSkinnedPoints(xforms, &points, time);
}

template <typename Matrix4>
Matrix4
_ComputeSkinnedTransform(const UsdSkelSkinningQuery& self,
                         const VtArray<Matrix4>& xforms,
                         UsdTimeCode time)
{
    Matrix4 xform;
    self.ComputeSkinnedTransform(xforms, &xform, time);
    return xform;
}

} // namespace

void wrapUsdSkelSkinningQuery(nb::module_ &m)
{
    using This = UsdSkelSkinningQuery;

    nb::class_<This>(m, "SkinningQuery")

        .def(nb::init<>())

        .def("__bool__", [](const This& self) { return bool(self); })

        .def("__str__", &This::GetDescription)

        .def("GetPrim", &This::GetPrim, nb::rv_policy::copy)

        .def("HasJointInfluences", &This::HasJointInfluences)

        .def("HasBlendShapes", &This::HasBlendShapes)
        
        .def("GetNumInfluencesPerComponent",
             &This::GetNumInfluencesPerComponent)

        .def("GetInterpolation", &This::GetInterpolation, nb::rv_policy::copy)

        .def("IsRigidlyDeformed", &This::IsRigidlyDeformed)

        .def("GetSkinningMethodAttr", &This::GetSkinningMethodAttr,
             nb::rv_policy::copy)

        .def("GetGeomBindTransformAttr", &This::GetGeomBindTransformAttr,
             nb::rv_policy::copy)

        .def("GetJointIndicesPrimvar", &This::GetJointIndicesPrimvar,
             nb::rv_policy::copy)

        .def("GetJointWeightsPrimvar", &This::GetJointWeightsPrimvar,
             nb::rv_policy::copy)

        .def("GetBlendShapesAttr", &This::GetBlendShapesAttr,
             nb::rv_policy::copy)

        .def("GetBlendShapeTargetsRel", &This::GetBlendShapeTargetsRel,
             nb::rv_policy::copy)

        // deprecated
        .def("GetMapper", &This::GetMapper, nb::rv_policy::copy)

        .def("GetJointMapper", &This::GetJointMapper, nb::rv_policy::copy)

        .def("GetBlendShapeMapper", &This::GetBlendShapeMapper,
             nb::rv_policy::copy)

        .def("GetJointOrder", &_GetJointOrder)

        .def("GetBlendShapeOrder", &_GetBlendShapeOrder)

        .def("GetTimeSamples", &_GetTimeSamples)

        .def("GetTimeSamplesInInterval", &_GetTimeSamplesInInterval)

        .def("ComputeJointInfluences", &_ComputeJointInfluences,
             nb::arg("time")=UsdTimeCode::Default())

        .def("ComputeVaryingJointInfluences", &_ComputeVaryingJointInfluences,
             nb::arg("numPoints"), nb::arg("time")=UsdTimeCode::Default())

        .def("ComputeSkinnedPoints", &_ComputeSkinnedPoints<GfMatrix4d>,
             nb::arg("xforms"), nb::arg("points"),
             nb::arg("time")=UsdTimeCode::Default())

        .def("ComputeSkinnedPoints", &_ComputeSkinnedPoints<GfMatrix4f>,
             nb::arg("xforms"), nb::arg("points"),
             nb::arg("time")=UsdTimeCode::Default())

        .def("ComputeSkinnedTransform", &_ComputeSkinnedTransform<GfMatrix4d>,
             nb::arg("xforms"),
             nb::arg("time")=UsdTimeCode::Default())

        .def("ComputeSkinnedTransform", &_ComputeSkinnedTransform<GfMatrix4f>,
             nb::arg("xforms"),
             nb::arg("time")=UsdTimeCode::Default())

        .def("ComputeExtentsPadding",
             static_cast<float (UsdSkelSkinningQuery::*)(
                 const VtMatrix4dArray&,
                 const UsdGeomBoundable&) const>(
                     &This::ComputeExtentsPadding),
             nb::arg("skelRestXforms"),
             nb::arg("boundable"))

        .def("ComputeExtentsPadding",
             static_cast<float (UsdSkelSkinningQuery::*)(
                 const VtMatrix4fArray&,
                 const UsdGeomBoundable&) const>(
                     &This::ComputeExtentsPadding),
             nb::arg("skelRestXforms"),
             nb::arg("boundable"))

        .def("GetSkinningMethod", &This::GetSkinningMethod)

        .def("GetGeomBindTransform", &This::GetGeomBindTransform,
             nb::arg("time")=UsdTimeCode::Default())
        ;

}
