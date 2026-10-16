//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

string _Repr(GfBBox3d const &self) {
    return TF_PY_REPR_PREFIX + "BBox3d(" + TfPyRepr(self.GetRange()) + ", " +
        TfPyRepr(self.GetMatrix()) + ")";
}

static size_t __hash__(GfBBox3d const &self) {
    return TfHash()(self);
}

} // anonymous namespace

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

void wrapBBox3d(nanobind::module_ &m)
{    
    typedef GfBBox3d This;

    nanobind::class_<This>(m, "BBox3d", "Arbitrarily oriented 3D bounding box")
        .def(nanobind::init<>())
        .def(nanobind::init<const This &>())
        .def(nanobind::init<const GfRange3d &>())
        .def(nanobind::init<const GfRange3d &, const GfMatrix4d &>())

        .def(TfTypePythonClass())

        .def("Set", [](This &self, const GfRange3d &box, const GfMatrix4d &matrix) -> This & {
            self.Set(box, matrix);
            return self;
        }, nanobind::rv_policy::reference)

        .def_prop_rw("box", &This::GetRange, &This::SetRange,
                     nanobind::rv_policy::copy)
        
        .def_prop_rw("matrix", &This::GetMatrix, &This::SetMatrix,
                     nanobind::rv_policy::copy)

        // In 2x, GetBox is a scriptable method instead of using the
        // "box" property.
        .def("GetBox", &This::GetRange, nanobind::rv_policy::copy)

        // But in 3x, GetBox was renamed to GetRange and was not
        // scriptable.  We'd like to use GetRange in code in the
        // future so we're going to make the same interface available
        // via script.
        .def("GetRange", &This::GetRange, nanobind::rv_policy::copy)

        .def("GetInverseMatrix", &This::GetInverseMatrix,
             nanobind::rv_policy::copy)
        
        .def("GetMatrix", &This::GetMatrix, nanobind::rv_policy::copy)

        .def_prop_rw("hasZeroAreaPrimitives",
                     &This::HasZeroAreaPrimitives,
                     &This::SetHasZeroAreaPrimitives)

        .def("GetVolume", &This::GetVolume)

        .def("HasZeroAreaPrimitives", &This::HasZeroAreaPrimitives)

        .def("SetHasZeroAreaPrimitives", &This::SetHasZeroAreaPrimitives)

        .def("SetMatrix", [](This &self, const GfMatrix4d &matrix) -> This & {
            self.SetMatrix(matrix);
            return self;
        }, nanobind::rv_policy::reference)

        .def("SetRange", [](This &self, const GfRange3d &box) -> This & {
            self.SetRange(box);
            return self;
        }, nanobind::rv_policy::reference)

        .def("Transform", [](This &self, const GfMatrix4d &matrix) -> This & {
            self.Transform(matrix);
            return self;
        }, nanobind::rv_policy::reference)

        // 2x defines ComputeAlignedBox
        .def("ComputeAlignedBox", &This::ComputeAlignedRange)

        .def("ComputeAlignedRange", &This::ComputeAlignedRange)

        .def("ComputeCentroid", &This::ComputeCentroid)

        .def_static("Combine", &This::Combine)
        
        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self)

        .def("__repr__", _Repr)
        .def("__hash__", __hash__)
        ;

    // Allow conversion of lists of GfBBox3d to std::vector<GfBBox3d>
    TfPyContainerConversions::from_python_sequence<
        std::vector<This>,
        TfPyContainerConversions::variable_capacity_policy >();
}
