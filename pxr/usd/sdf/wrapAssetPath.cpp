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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/wrapArray.h"

#include <sstream>

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
static void
MakeAssetPath(SdfAssetPath *self, nanobind::args args, nanobind::kwargs kwArgs)
{
    if (nanobind::len(args) > 0) {
        TfPyThrowTypeError(
            "Use keyword arguments 'authoredPath', 'evaluatedPath', "
            "and/or 'resolvedPath'");
    }
    
    SdfAssetPathParams p;

    for (auto item : kwArgs) {
        std::string k = nanobind::cast<std::string>(item.first);

        if (k == "authoredPath") {
            p.Authored(nanobind::cast<std::string>(item.second));
        }
        else if (k == "evaluatedPath") {
            p.Evaluated(nanobind::cast<std::string>(item.second));
        }
        else if (k == "resolvedPath") {
            p.Resolved(nanobind::cast<std::string>(item.second));
        }
        else {
            TfPyThrowTypeError(
                "Keyword arguments must be 'authoredPath', 'evaluatedPath', "
                "or 'resolvedPath'");
        }
    }

    new (self) SdfAssetPath(p);
}

} // anonymous namespace 

void wrapAssetPath(nanobind::module_ &m)
{
    namespace nb = nanobind;
    typedef SdfAssetPath This;

    nb::class_<This>(m, "AssetPath")
        .def(nb::init<>())
        .def(nb::init<const This&>())
        .def(nb::init<const std::string &>(), nb::arg("authoredPath"))
        .def(nb::init<const std::string &, const std::string &>(),
             nb::arg("authoredPath"), nb::arg("resolvedPath"))
        .def("__init__", &MakeAssetPath)

        .def("__repr__", _Repr)
        .def("__bool__", _Nonzero)
        .def("__hash__", _Hash)

        .def( nb::self == nb::self )
        .def( nb::self != nb::self )
        .def( nb::self < nb::self )
        .def( nb::self > nb::self )
        .def( nb::self <= nb::self )
        .def( nb::self >= nb::self)
        .def("__str__", _Str)

        .def_prop_ro("path", GetAssetPath)
        .def_prop_rw("resolvedPath",
            GetResolvedPath, &This::SetResolvedPath)
        .def_prop_rw("authoredPath", 
            GetAuthoredPath, &This::SetAuthoredPath)
        .def_prop_rw("evaluatedPath", 
            GetEvaluatedPath, &This::SetEvaluatedPath)
        ;

    nb::implicitly_convertible<std::string, This>();

    // Let python know about us, to enable assignment from python back to C++
    VtValueFromPython<SdfAssetPath>();
}
