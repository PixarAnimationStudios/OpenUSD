//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/timeCode.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/external/boost/python.hpp"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static std::string
_Repr(const SdfLayerOffset &self) {
    double offset = self.GetOffset();
    double scale = self.GetScale();

    std::stringstream s;
    s << TF_PY_REPR_PREFIX + "LayerOffset(";
    if (offset != 0.0 || scale != 1.0) {
        s << offset;
        if (scale != 1.0)
            s << ", " << scale;
    }
    s << ")";
    return s.str();
}

} // anonymous namespace 

void wrapLayerOffset()
{    
    typedef SdfLayerOffset This;

    TfPyContainerConversions::from_python_sequence<
        std::vector< SdfLayerOffset >,
        TfPyContainerConversions::variable_capacity_policy >();

    // Note: Since we have no support for nested proxies we wrap Sdf.LayerOffset
    //       as an immutable type to avoid confusion about code like this
    //       prim.referenceList.explicitItems[0].layerOffset.scale = 2
    //       This looks like it's updating the layerOffset for the prim's
    //       first explicit reference, but would instead modify a temporary
    //       Sdf.LayerOffset object.

    class_<This>( "LayerOffset" )
        .def(init<double, double>(
            ( arg("offset") = 0.0,
              arg("scale") = 1.0 ) ) )
        .def(init<const This &>())

        .add_property("offset", &This::GetOffset)
        .add_property("scale", &This::GetScale)

        .def("IsIdentity", &This::IsIdentity)
        .def("GetInverse", &This::GetInverse)
        
        .def( self == self )
        .def( self != self )
        .def( self * self )

        // This order is required to prevent doubles from implicitly converting
        // to SdfTimeCode when calling SdfLayerOffset * double.
        .def( self * SdfTimeCode() )
        .def( self * double() )

        .def("__repr__", _Repr)
        
        ;

    VtValueFromPython<SdfLayerOffset>();
}
