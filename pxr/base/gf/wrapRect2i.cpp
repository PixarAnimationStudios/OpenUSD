//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/rect2i.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/copy_const_reference.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/return_arg.hpp"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static string _Repr(GfRect2i const &self) {
    return TF_PY_REPR_PREFIX + "Rect2i(" + TfPyRepr(self.GetMin()) + ", " +
        TfPyRepr(self.GetMax()) + ")";
}

static size_t __hash__(GfRect2i const &self) {
    return TfHash()(self);
}

} // anonymous namespace 

void wrapRect2i()
{    
    using This = GfRect2i;

    object getMin = make_function(&This::GetMin,
                                    return_value_policy<return_by_value>());

    object getMax = make_function(&This::GetMax,
                                     return_value_policy<return_by_value>());

    class_<This>( "Rect2i", init<>() )
        .def(init<const This &>())
        .def(init<const GfVec2i &, const GfVec2i &>())
        .def(init<const GfVec2i &, int, int >())

        .def( TfTypePythonClass() )

        .def("IsNull", &This::IsNull)
        .def("IsEmpty", &This::IsEmpty)
        .def("IsValid", &This::IsValid)

        .add_property("min", getMin, &This::SetMin)
        .add_property("max", getMax, &This::SetMax)

        .add_property("minX", &This::GetMinX, &This::SetMinX) 
        .add_property("maxX", &This::GetMaxX, &This::SetMaxX) 
        .add_property("minY", &This::GetMinY, &This::SetMinY) 
        .add_property("maxY", &This::GetMaxY, &This::SetMaxY) 

        .def("GetMin", getMin)
        .def("GetMax", getMax)

        .def("GetMinX", &This::GetMinX)
        .def("GetMaxX", &This::GetMaxX)
        .def("GetMinY", &This::GetMinY)
        .def("GetMaxY", &This::GetMaxY)

        .def("SetMin", &This::SetMin)
        .def("SetMax", &This::SetMax)

        .def("SetMinX", &This::SetMinX)
        .def("SetMaxX", &This::SetMaxX)
        .def("SetMinY", &This::SetMinY)
        .def("SetMaxY", &This::SetMaxY)

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
        .def("__hash__", __hash__)
        ;
    to_python_converter<std::vector<This>,
        TfPySequenceToPython<std::vector<This> > >();
    
}
