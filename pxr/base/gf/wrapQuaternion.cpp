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
#include "pxr/base/gf/quaternion.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/return_arg.hpp>

#include <string>

using namespace boost::python;

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( GetNormalized_overloads,
                                        GetNormalized, 0, 1 );
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( Normalize_overloads,
                                        Normalize, 0, 1 );

static GfQuaternion __truediv__(const GfQuaternion &self, double value)
{
    return self / value;
}

static GfQuaternion& __itruediv__(GfQuaternion &self, double value)
{
    return self /= value;
}

static string _Repr(GfQuaternion const &self) {
    return TF_PY_REPR_PREFIX + "Quaternion(" + TfPyRepr(self.GetReal()) + ", " +
        TfPyRepr(self.GetImaginary()) + ")";
}

static size_t __hash__(GfQuaternion const &self) { return TfHash{}(self); }

} // anonymous namespace 

void wrapQuaternion()
{    
    typedef GfQuaternion This;

    object getImaginary =
        make_function(&This::GetImaginary,return_value_policy<return_by_value>());

    def( "Dot", (double (*)(const GfQuaternion &, const GfQuaternion &))GfDot);
    def( "Slerp", (GfQuaternion (*)(double, const GfQuaternion&, const GfQuaternion&))GfSlerp);
    
    class_<This> cls( "Quaternion", "Quaternion class", init<>());
    cls

        .def(init<int>())

        .def(init<double, const GfVec3d &>())

        .def(init<This>())

        .def( TfTypePythonClass() )

        .def("GetZero", &This::GetZero)
        .staticmethod("GetZero")

        .def("GetIdentity", &This::GetIdentity)
        .staticmethod("GetIdentity")

        .add_property("real", &This::GetReal, &This::SetReal)
        .add_property("imaginary", getImaginary, &This::SetImaginary)

        .def("GetImaginary", getImaginary)
        .def("GetInverse", &This::GetInverse)
        .def("GetLength", &This::GetLength)
        .def("GetReal", &This::GetReal)

        .def("GetNormalized", &This::GetNormalized, GetNormalized_overloads())
        .def("Normalize", &This::Normalize,
             Normalize_overloads()[return_self<>()])

        .def( str(self) )
        .def( self == self )
        .def( self != self )
        .def( self *= self )
        .def( self *= double() )
        .def( self /= double() )
        .def( self += self )
        .def( self -= self )
        .def( self + self )
        .def( self - self )
        .def( self * self )
        .def( self * double() )
        .def( double() * self )
        .def( self / double() )
        .def("__repr__", _Repr)
        .def("__hash__", __hash__)

        ;
    to_python_converter<std::vector<This>,
        TfPySequenceToPython<std::vector<This> > >();

    if (!PyObject_HasAttrString(cls.ptr(), "__truediv__")) {
        // __truediv__ not added by .def( self / double() ) above, which
        // happens when building with python 2, but we need it to support
        // "from __future__ import division"
        cls.def("__truediv__", __truediv__);
    }
    if (!PyObject_HasAttrString(cls.ptr(), "__itruediv__")) {
        // __itruediv__ not added by .def( self /= double() ) above, which
        // happens when building with python 2, but we need it to support
        // "from __future__ import division". This is also a workaround for a 
        // bug in the current version of boost::python that incorrectly wraps
        // in-place division with __idiv__ when building with python 3.
        cls.def("__itruediv__", __itruediv__, return_self<>{});
    }
    
}
