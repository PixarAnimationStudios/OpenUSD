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
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/vt/wrapArray.h"

#include <boost/functional/hash.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/operators.hpp>

#include <sstream>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

TF_REGISTRY_FUNCTION(VtValue)
{
    VtRegisterValueCastsFromPythonSequencesToArray<SdfAssetPath>();
}

namespace {

static std::string _Str(SdfAssetPath const &self)
{
    return boost::lexical_cast<std::string>(self);
}

static std::string
_Repr(SdfAssetPath const &self)
{
    std::ostringstream repr;
    repr << TF_PY_REPR_PREFIX << "AssetPath("
         << TfPyRepr(self.GetAssetPath());

    const std::string & resolvedPath = self.GetResolvedPath();
    if (!resolvedPath.empty()) {
        repr << ", " << TfPyRepr(resolvedPath);
    }
    repr << ")";
    return repr.str();
}

static bool _Nonzero(SdfAssetPath const &self)
{
    return !self.GetAssetPath().empty();
}

static size_t _Hash(SdfAssetPath const &self)
{
    return self.GetHash();
}

static std::string
GetAssetPath(SdfAssetPath const &ap) {
    return ap.GetAssetPath();
}

static std::string
GetResolvedPath(SdfAssetPath const &ap) {
    return ap.GetResolvedPath();
}


} // anonymous namespace 

void wrapAssetPath()
{
    typedef SdfAssetPath This;

    class_<This>("AssetPath", init<>())
        .def(init<const This&>())
        .def(init<const std::string &>())
        .def(init<const std::string &, const std::string &>())

        .def("__repr__", _Repr)
        .def("__bool__", _Nonzero)
        .def("__hash__", _Hash)

        .def( self == self )
        .def( self != self )
        .def( self < self )
        .def( self > self )
        .def( self <= self )
        .def( self >= self)
        .def("__str__", _Str)

        .add_property("path", GetAssetPath)
        .add_property("resolvedPath", GetResolvedPath)
        ;

    implicitly_convertible<std::string, This>();

    // Let python know about us, to enable assignment from python back to C++
    VtValueFromPython<SdfAssetPath>();
}
