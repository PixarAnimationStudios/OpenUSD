//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/wrapArray.h"

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
    return TfStringify(self);
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
