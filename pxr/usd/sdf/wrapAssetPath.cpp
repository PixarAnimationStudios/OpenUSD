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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/implicit.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/raw_function.hpp"
#include "pxr/external/boost/python/stl_iterator.hpp"

#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
         << "authoredPath=" << TfPyRepr(self.GetAuthoredPath());

    const std::string & evaluatedPath = self.GetEvaluatedPath();
    const std::string & resolvedPath = self.GetResolvedPath();
    if (!evaluatedPath.empty()) {
        repr << ", evaluatedPath=" << TfPyRepr(evaluatedPath);
    }
    if (!resolvedPath.empty()) {
        repr << ", resolvedPath=" << TfPyRepr(resolvedPath);
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
GetAuthoredPath(SdfAssetPath const &ap) {
    return ap.GetAuthoredPath();
}

static std::string
GetEvaluatedPath(SdfAssetPath const &ap) {
    return ap.GetEvaluatedPath();
}

static std::string
GetResolvedPath(SdfAssetPath const &ap) {
    return ap.GetResolvedPath();
}

// This attempts to mimic Python's keyword-only arguments behavior from
// PEP 3102. Clients passing in 3 or more parameters are required to
// use keywords to avoid confusion when using positional arguments about
// which parameter corresponds to which path type.
static object
MakeAssetPath(tuple const& args, dict const& kwArgs)
{
    object self = args[0];

    const tuple posArgs(args.slice(1, _));
    if (len(posArgs) > 0) {
        TfPyThrowTypeError(
            "Use keyword arguments 'authoredPath', 'evaluatedPath', "
            "and/or 'resolvedPath'");
    }
    
    SdfAssetPathParams p;

    for (stl_input_iterator<std::string> k(kwArgs.keys()), e; 
            k != e; ++k) {

        if (*k == "authoredPath") {
            p.Authored(extract<std::string>(kwArgs.get(*k, std::string())));
        }
        else if (*k == "evaluatedPath") {
            p.Evaluated(extract<std::string>(kwArgs.get(*k, std::string())));
        }
        else if (*k == "resolvedPath") {
            p.Resolved(extract<std::string>(kwArgs.get(*k, std::string())));
        }
        else {
            TfPyThrowTypeError(
                "Keyword arguments must be 'authoredPath', 'evaluatedPath', "
                "or 'resolvedPath'");
        }
    }

    return self.attr("__init__")(SdfAssetPath(p));
}

} // anonymous namespace 

void wrapAssetPath()
{
    typedef SdfAssetPath This;

    class_<This>("AssetPath", init<>())
        .def("__init__", raw_function(&MakeAssetPath))
        .def(init<const This&>())
        .def(init<const std::string &>(arg("authoredPath")))
        .def(init<const std::string &, const std::string &>(
                (arg("authoredPath"), arg("resolvedPath"))))

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
        .add_property("resolvedPath",
            GetResolvedPath, &This::SetResolvedPath)
        .add_property("authoredPath", 
            GetAuthoredPath, &This::SetAuthoredPath)
        .add_property("evaluatedPath", 
            GetEvaluatedPath, &This::SetEvaluatedPath)
        ;

    implicitly_convertible<std::string, This>();

    // Let python know about us, to enable assignment from python back to C++
    VtValueFromPython<SdfAssetPath>();
}
