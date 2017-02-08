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
#include "pxr/base/gf/rect2i.h"
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

static string _Repr(GfRect2i const &self) {
    return TF_PY_REPR_PREFIX + "Rect2i(" + TfPyRepr(self.GetLower()) + ", " +
        TfPyRepr(self.GetHigher()) + ")";
}

void wrapRect2i()
{    
    typedef GfRect2i This;

    object getLower = make_function(&This::GetLower,
                                    return_value_policy<return_by_value>());

    object getHigher = make_function(&This::GetHigher,
                                     return_value_policy<return_by_value>());

    class_<This>( "Rect2i", init<>() )
        .def(init<const This &>())
        .def(init<const GfVec2i &, const GfVec2i &>())
        .def(init<const GfVec2i &, int, int >())

        .def( TfTypePythonClass() )

        .def("IsNull", &This::IsNull)
        .def("IsEmpty", &This::IsEmpty)
        .def("IsValid", &This::IsValid)

        .add_property("lower", getLower, &This::SetLower)
        .add_property("higher", getHigher, &This::SetHigher)

        .add_property("bottom", &This::GetBottom, &This::SetBottom) 
        .add_property("left", &This::GetLeft, &This::SetLeft) 
        .add_property("right", &This::GetRight, &This::SetRight) 
        .add_property("top", &This::GetTop, &This::SetTop) 

        .def("GetLower", getLower)
        .def("GetHigher", getHigher)

        .def("GetBottom", &This::GetBottom)
        .def("GetLeft", &This::GetLeft)
        .def("GetRight", &This::GetRight)
        .def("GetTop", &This::GetTop)

        .def("SetLower", &This::SetLower)
        .def("SetHigher", &This::SetHigher)

        .def("SetBottom", &This::SetBottom)
        .def("SetLeft", &This::SetLeft)
        .def("SetRight", &This::SetRight)
        .def("SetTop", &This::SetTop)

        .def("GetArea", &This::GetArea)
        .def("GetCenter", &This::GetCenter)
        .def("GetHeight", &This::GetHeight)
        .def("GetSize", &This::GetSize)
        .def("GetWidth", &This::GetWidth)

        .def("Translate", &This::Translate, return_self<>())
        .def("GetNormalized", &This::GetNormalized)
        .def("GetIntersection", &This::GetIntersection)
        .def("GetUnion", &This::GetUnion)

        .def("Contains", &This::Contains )

        .def( str(self) )
        .def( self == self )
        .def( self != self )
        .def( self += self )
        .def( self + self )
        
        .def("__repr__", _Repr)
        
        ;
    to_python_converter<std::vector<This>,
        TfPySequenceToPython<std::vector<This> > >();
    
}

PXR_NAMESPACE_CLOSE_SCOPE
