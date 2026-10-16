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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

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

void wrapLayerOffset(nanobind::module_ &m)
{    
    namespace nb = nanobind;
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

    nb::class_<This>( m, "LayerOffset" )
        .def(nb::init<double, double>(),
             nb::arg("offset") = 0.0,
             nb::arg("scale") = 1.0)
        .def(nb::init<const This &>())

        .def_prop_ro("offset", &This::GetOffset)
        .def_prop_ro("scale", &This::GetScale)

        .def("IsIdentity", &This::IsIdentity)
        .def("GetInverse", &This::GetInverse)
        
        .def( nb::self == nb::self )
        .def( nb::self != nb::self )
        .def( nb::self * nb::self )

        // This order is required to prevent doubles from implicitly converting
        // to GfTimeCode when calling SdfLayerOffset * double.
        .def( nb::self * double() )
        .def( nb::self * GfTimeCode() )
        .def( nb::self * GfDuration() )

        .def("__repr__", _Repr)
        
        ;

    VtValueFromPython<SdfLayerOffset>();
}
