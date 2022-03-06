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
#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>

#include <string>



PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrBaseGfWrapBBox3d {

std::string _Repr(GfBBox3d const &self) {
    return TF_PY_REPR_PREFIX + "BBox3d(" + TfPyRepr(self.GetRange()) + ", " +
        TfPyRepr(self.GetMatrix()) + ")";
}

} // anonymous namespace

void wrapBBox3d()
{    
    typedef GfBBox3d This;

    boost::python::class_<This>( "BBox3d", "Arbitrarily oriented 3D bounding box", boost::python::init<>() )
        .def(boost::python::init< const This & >())
        .def(boost::python::init< const GfRange3d & >())
        .def(boost::python::init< const GfRange3d &, const GfMatrix4d & >())

        .def( TfTypePythonClass() )

        .def("Set", &This::Set, boost::python::return_self<>())

        .add_property("box", boost::python::make_function(&This::GetRange,
                                           boost::python::return_value_policy
                                           <boost::python::copy_const_reference>()),
                      &This::SetRange )
        
        .add_property("matrix", boost::python::make_function(&This::GetMatrix,
                                              boost::python::return_value_policy
                                              <boost::python::copy_const_reference>()),
                      &This::SetMatrix )

        // In 2x, GetBox is a scriptable method instead of using the
        // "box" property.
        .def("GetBox", boost::python::make_function(&This::GetRange,
                                     boost::python::return_value_policy
                                     <boost::python::copy_const_reference>()))

        // But in 3x, GetBox was renamed to GetRange and was not
        // scriptable.  We'd like to use GetRange in code in the
        // future so we're going to make the same interface available
        // via script.
        .def("GetRange", boost::python::make_function(&This::GetRange,
                                       boost::python::return_value_policy
                                       <boost::python::copy_const_reference>()))

        .def("GetInverseMatrix",
             &This::GetInverseMatrix,
             boost::python::return_value_policy<boost::python::copy_const_reference>())
        
        .def("GetMatrix", boost::python::make_function(&This::GetMatrix,
                                        boost::python::return_value_policy
                                        <boost::python::copy_const_reference>()))

        .add_property("hasZeroAreaPrimitives",
                      &This::HasZeroAreaPrimitives,
                      &This::SetHasZeroAreaPrimitives)

        .def("GetVolume", &This::GetVolume)

        .def("HasZeroAreaPrimitives", &This::HasZeroAreaPrimitives)

        .def("Set", &This::Set, boost::python::return_self<>())

        .def("SetHasZeroAreaPrimitives", &This::SetHasZeroAreaPrimitives)

        .def("SetMatrix", &This::SetMatrix, boost::python::return_self<>())

        .def("SetRange", &This::SetRange, boost::python::return_self<>())

        .def("Transform", &This::Transform, boost::python::return_self<>())

        // 2x defines ComputeAlignedBox
        .def("ComputeAlignedBox", &This::ComputeAlignedRange)

        .def("ComputeAlignedRange", &This::ComputeAlignedRange)

        .def("ComputeCentroid", &This::ComputeCentroid)

        .def("Combine", &This::Combine)
        .staticmethod("Combine")
        
        .def(boost::python::self_ns::str(boost::python::self))
        .def(boost::python::self == boost::python::self)
        .def(boost::python::self != boost::python::self)

        .def("__repr__", pxrBaseGfWrapBBox3d::_Repr)
        
        ;
    boost::python::to_python_converter<std::vector<This>,
        TfPySequenceToPython<std::vector<This> > >();
    
}
