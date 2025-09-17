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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/copy_const_reference.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/return_arg.hpp"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

string _Repr(GfBBox3d const &self) {
    return TF_PY_REPR_PREFIX + "BBox3d(" + TfPyRepr(self.GetRange()) + ", " +
        TfPyRepr(self.GetMatrix()) + ")";
}

static size_t __hash__(GfBBox3d const &self) {
    return TfHash()(self);
}

} // anonymous namespace

void wrapBBox3d()
{    
    typedef GfBBox3d This;

    class_<This>( "BBox3d", "Arbitrarily oriented 3D bounding box", init<>() )
        .def(init< const This & >())
        .def(init< const GfRange3d & >())
        .def(init< const GfRange3d &, const GfMatrix4d & >())

        .def( TfTypePythonClass() )

        .def("Set", &This::Set, return_self<>())

        .add_property("box", make_function(&This::GetRange,
                                           return_value_policy
                                           <copy_const_reference>()),
                      &This::SetRange )
        
        .add_property("matrix", make_function(&This::GetMatrix,
                                              return_value_policy
                                              <copy_const_reference>()),
                      &This::SetMatrix )

        // In 2x, GetBox is a scriptable method instead of using the
        // "box" property.
        .def("GetBox", make_function(&This::GetRange,
                                     return_value_policy
                                     <copy_const_reference>()))

        // But in 3x, GetBox was renamed to GetRange and was not
        // scriptable.  We'd like to use GetRange in code in the
        // future so we're going to make the same interface available
        // via script.
        .def("GetRange", make_function(&This::GetRange,
                                       return_value_policy
                                       <copy_const_reference>()))

        .def("GetInverseMatrix",
             &This::GetInverseMatrix,
             return_value_policy<copy_const_reference>())
        
        .def("GetMatrix", make_function(&This::GetMatrix,
                                        return_value_policy
                                        <copy_const_reference>()))

        .add_property("hasZeroAreaPrimitives",
                      &This::HasZeroAreaPrimitives,
                      &This::SetHasZeroAreaPrimitives)

        .def("GetVolume", &This::GetVolume)

        .def("HasZeroAreaPrimitives", &This::HasZeroAreaPrimitives)

        .def("Set", &This::Set, return_self<>())

        .def("SetHasZeroAreaPrimitives", &This::SetHasZeroAreaPrimitives)

        .def("SetMatrix", &This::SetMatrix, return_self<>())

        .def("SetRange", &This::SetRange, return_self<>())

        .def("Transform", &This::Transform, return_self<>())

        // 2x defines ComputeAlignedBox
        .def("ComputeAlignedBox", &This::ComputeAlignedRange)

        .def("ComputeAlignedRange", &This::ComputeAlignedRange)

        .def("ComputeCentroid", &This::ComputeCentroid)

        .def("Combine", &This::Combine)
        .staticmethod("Combine")
        
        .def(str(self))
        .def(self == self)
        .def(self != self)

        .def("__repr__", _Repr)
        .def("__hash__", __hash__)
        
        ;
    to_python_converter<std::vector<This>,
        TfPySequenceToPython<std::vector<This> > >();

    // Allow conversion of lists of GfBBox3d to std::vector<GfBBox3d>
    TfPyContainerConversions::from_python_sequence<
        std::vector<This>,
        TfPyContainerConversions::variable_capacity_policy >();
}
