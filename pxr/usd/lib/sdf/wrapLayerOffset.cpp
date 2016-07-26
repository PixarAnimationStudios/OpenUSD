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
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/base/vt/valueFromPython.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python.hpp>

using namespace boost::python;

using std::string;

static std::string
_Repr(const SdfLayerOffset &self) {
    double offset = self.GetOffset();
    double scale = self.GetScale();

    std::stringstream s;
    s << TF_PY_REPR_PREFIX + "LayerOffset(";
    if (offset != 0.0 or scale != 1.0) {
        s << offset;
        if (scale != 1.0)
            s << ", " << scale;
    }
    s << ")";
    return s.str();
}

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
        .def( self * double() )

        .def("__repr__", ::_Repr)
        
        ;

    VtValueFromPython<SdfLayerOffset>();
}
