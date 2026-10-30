//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/ray.h"

#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static std::string _Repr(GfFrustum const &self)
{
    const std::string prefix = TF_PY_REPR_PREFIX + "Frustum(";
    const std::string indent(prefix.size(), ' ');
    const std::string seperator = ",\n" + indent;

    // Use keyword args for clarity.
    // Only use viewDistance if not matching default
    std::vector<std::string> kwargs;
    kwargs.push_back( "position = " + TfPyRepr(self.GetPosition()));
    kwargs.push_back( "rotation = " + TfPyRepr(self.GetRotation()));
    kwargs.push_back( "window = " + TfPyRepr(self.GetWindow()));
    kwargs.push_back( "nearFar = " + TfPyRepr(self.GetNearFar()));
    kwargs.push_back( "projectionType = " + TfPyRepr(self.GetProjectionType()));
    if (self.GetViewDistance() != 5.0) {
        kwargs.push_back( "viewDistance = " + TfPyRepr(self.GetViewDistance()));
    }
    return prefix + TfStringJoin(kwargs, seperator.c_str()) + ")";
}

static size_t __hash__(GfFrustum const &self) {
    return TfHash()(self);
}

static nanobind::object
GetPerspectiveHelper( const GfFrustum &self, bool isFovVertical ) {
    double fov, aspect, nearDist, farDist;
    bool result = self.GetPerspective( isFovVertical, 
                                       &fov, &aspect, &nearDist, &farDist );
    return result ?
        nanobind::object(nanobind::make_tuple( fov, aspect, nearDist, farDist )) : nanobind::none();
}

static nanobind::tuple
GetOrthographicHelper( const GfFrustum &self ) {
    double left, right, bottom, top, near, far;
    bool result =
        self.GetOrthographic( &left, &right, &bottom, &top, &near, &far );
    return result ?
        nanobind::make_tuple( left, right, bottom, top, near, far ) : nanobind::tuple();
}

static nanobind::tuple
ComputeViewFrameHelper( const GfFrustum &self ) {
    GfVec3d side, up, view;
    self.ComputeViewFrame( &side, &up, &view );
    return nanobind::make_tuple( side, up, view );
}

} // anonymous namespace 

void wrapFrustum(nanobind::module_ &m)
{    
    typedef GfFrustum This;

    nanobind::class_<This> c(m, "Frustum", "Basic view frustum");
    c
        .def(nanobind::init<>())
        .def(nanobind::init< const This & >())
        .def(nanobind::init<const GfVec3d &, const GfRotation &,
                  const GfRange2d &, const GfRange1d &,
                  GfFrustum::ProjectionType, double>(),
             nanobind::arg("position"), nanobind::arg("rotation"),
             nanobind::arg("window"), nanobind::arg("nearFar"),
             nanobind::arg("projectionType"), nanobind::arg("viewDistance") = 5.0)

        .def(nanobind::init<const GfMatrix4d &,
                  const GfRange2d &, const GfRange1d &,
                  GfFrustum::ProjectionType, double>(),
             nanobind::arg("camToWorldXf"),
             nanobind::arg("window"), nanobind::arg("nearFar"),
             nanobind::arg("projectionType"), nanobind::arg("viewDistance") = 5.0)

        .def(TfTypePythonClass())
        
        .def_prop_rw( "position", &This::GetPosition, &This::SetPosition,
                      nanobind::rv_policy::copy )
        .def_prop_rw( "rotation", &This::GetRotation, &This::SetRotation,
                      nanobind::rv_policy::copy )
        .def_prop_rw( "window",   &This::GetWindow,   &This::SetWindow,
                      nanobind::rv_policy::copy )
        .def_prop_rw( "nearFar",  &This::GetNearFar,  &This::SetNearFar,
                      nanobind::rv_policy::copy )
        .def_prop_rw( "viewDistance", &This::GetViewDistance,
                      &This::SetViewDistance )
        .def_prop_rw( "projectionType", &This::GetProjectionType,
                      &This::SetProjectionType )

        .def("SetPerspective",
             (void (This::*)(double, double, double, double))&This::SetPerspective,
             nanobind::arg("fovHeight"), nanobind::arg("aspectRatio"),
             nanobind::arg("nearDist"), nanobind::arg("farDist"))
        .def("SetPerspective",
             (void (This::*)(double, bool, double, double, double))&This::SetPerspective,
             nanobind::arg("fov"), nanobind::arg("isFovVertical"),
             nanobind::arg("aspectRatio"), nanobind::arg("nearDist"), nanobind::arg("farDist"))

        .def("GetPerspective", GetPerspectiveHelper,
             nanobind::arg("isFovVertical") = true,
             "Returns the current perspective frustum values suitable\n"
             "for use by SetPerspective.  If the current frustum is a\n"
             "perspective projection, the return value is a tuple of\n"
             "fieldOfView, aspectRatio, nearDistance, farDistance).\n"
             "If the current frustum is not perspective, the return\n"
             "value is None.")

        .def("GetFOV", &This::GetFOV,
             nanobind::arg("isFovVertical") = false,
             "Returns the horizontal fov of the frustum. The fov of the\n"
             "frustum is not necessarily the same value as displayed in\n"
             "the viewer. The displayed fov is a function of the focal\n"
             "length or FOV avar. The frustum's fov may be different due\n"
             "to things like lens breathing.\n"
             "\n"
             "If the frustum is not of type GfFrustum::Perspective, the\n"
             "returned FOV will be 0.0.")

        .def("SetOrthographic", &This::SetOrthographic)
        .def("GetOrthographic", GetOrthographicHelper)

        .def("GetPosition", &This::GetPosition, nanobind::rv_policy::copy)
        .def("SetPosition", &This::SetPosition)

        .def("GetRotation", &This::GetRotation, nanobind::rv_policy::copy)
        .def("SetRotation", &This::SetRotation)

        .def("SetPositionAndRotationFromMatrix",
             &This::SetPositionAndRotationFromMatrix, nanobind::arg("camToWorldXf"))

        .def("GetWindow", &This::GetWindow, nanobind::rv_policy::copy)
        .def("SetWindow", &This::SetWindow)

        .def("GetNearFar", &This::GetNearFar, nanobind::rv_policy::copy)
        .def("SetNearFar", &This::SetNearFar)

        .def("GetViewDistance", &This::GetViewDistance)
        .def("SetViewDistance", &This::SetViewDistance)

        .def("FitToSphere", &This::FitToSphere,
             nanobind::arg("center"), nanobind::arg("radius"), nanobind::arg("slack") = 0.0)

        .def("Transform", &This::Transform, nanobind::rv_policy::reference)

        .def("ComputeViewDirection", &This::ComputeViewDirection )
        .def("ComputeUpVector", &This::ComputeUpVector )

        .def("ComputeViewFrame", ComputeViewFrameHelper )

        .def("ComputeLookAtPoint", &This::ComputeLookAtPoint)

        .def("ComputeViewMatrix", &This::ComputeViewMatrix)

        .def("ComputeViewInverse", &This::ComputeViewInverse)

        .def("ComputeProjectionMatrix", &This::ComputeProjectionMatrix)

        .def("ComputeAspectRatio", &This::ComputeAspectRatio)

        .def("ComputeCorners", [](const This &self) {
             return TfNbSequenceToTuple(self.ComputeCorners());
        })

        .def("ComputeCornersAtDistance", [](const This &self, double d) {
             return TfNbSequenceToTuple(self.ComputeCornersAtDistance(d));
        })

        .def("ComputePlanes", [](const This &self) {
             return TfNbSequenceToTuple(self.ComputePlanes());
        })

        .def("ComputeNarrowedFrustum", 
            (GfFrustum (This::*)(const GfVec2d &, const GfVec2d &) const)
            &This::ComputeNarrowedFrustum)
        .def("ComputeNarrowedFrustum", 
            (GfFrustum (This::*)(const GfVec3d &, const GfVec2d &) const)
            &This::ComputeNarrowedFrustum)

        .def("ComputePickRay", (GfRay (This::*)(const GfVec2d &) const)
            &This::ComputePickRay)
        .def("ComputePickRay", (GfRay (This::*)(const GfVec3d &) const)
            &This::ComputePickRay)

        .def("Intersects", (bool (This::*)(const GfBBox3d &) const)
             &This::Intersects)
        .def("Intersects", (bool (This::*)(const GfVec3d &) const)
             &This::Intersects)
        .def("Intersects",
             (bool (This::*)(const GfVec3d &,
                             const GfVec3d &) const)
             &This::Intersects)
        .def("Intersects",
             (bool (This::*)(const GfVec3d &,
                             const GfVec3d &,
                             const GfVec3d &) const)
             &This::Intersects)

        .def("SetProjectionType", &This::SetProjectionType)
        .def("GetProjectionType", &This::GetProjectionType)

        .def_static("GetReferencePlaneDepth", 
                &This::GetReferencePlaneDepth)

        .def_static("IntersectsViewVolume", &This::IntersectsViewVolume)
        
        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self)

        .def("__repr__", _Repr)
        .def("__hash__", __hash__)
        ;

    TfNbWrapEnum<This::ProjectionType>(c);
}
