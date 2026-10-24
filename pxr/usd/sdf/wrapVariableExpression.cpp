//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/sdf/variableExpression.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_set.h>
#include <nanobind/stl/vector.h>

#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

template <class T>
static bool
_IsHolding(nb::handle obj)
{
    // extract<bool> will allow conversions from int to bool, so we need
    // directly check whether the argument is a bool using Python API.
    if constexpr (std::is_same_v<T, bool>) {
        return PyBool_Check(obj.ptr());
    }
    else {
        T val;
        return nb::try_cast<T>(obj, val);
    }
}

template <class T>
static bool
_TryExtractVector(const nb::list& elems, std::vector<T>* out)
{
    out->reserve(nb::len(elems));
    for (nb::handle elem : elems) {
        if (!_IsHolding<T>(elem)) {
            return false;
        }
        out->push_back(nb::cast<T>(elem));
    }
    return true;
}

static SdfVariableExpression::Builder
_MakeLiteralBuilder(nb::handle o)
{
    if (_IsHolding<bool>(o)) {
        return SdfVariableExpression::MakeLiteral(nb::cast<bool>(o));
    }
    else if (_IsHolding<int64_t>(o)) {
        return SdfVariableExpression::MakeLiteral(nb::cast<int64_t>(o));
    }
    else if (_IsHolding<std::string>(o)) {
        return SdfVariableExpression::MakeLiteral(nb::cast<std::string>(o));
    }
    TfPyThrowTypeError("literal argument must be bool, int, or str");
    return SdfVariableExpression::MakeNone(); // unreachable
}

static SdfVariableExpression
_MakeListOfLiteralsExpr(const nb::list& elems)
{
    if (nb::len(elems) == 0) {
        return SdfVariableExpression::MakeList();
    }
    std::vector<bool> boolElems;
    if (_TryExtractVector<bool>(elems, &boolElems)) {
        return SdfVariableExpression::MakeListOfLiterals(boolElems);
    }
    std::vector<int64_t> intElems;
    if (_TryExtractVector<int64_t>(elems, &intElems)) {
        return SdfVariableExpression::MakeListOfLiterals(intElems);
    }
    std::vector<std::string> strElems;
    if (_TryExtractVector<std::string>(elems, &strElems)) {
        return SdfVariableExpression::MakeListOfLiterals(strElems);
    }

    TfPyThrowTypeError(
        "argument must be an empty list or contain only bool, int, or str");
    return SdfVariableExpression(); // unreachable
}

void
wrapVariableExpression(nanobind::module_& m)
{
    using This = SdfVariableExpression;

    nb::class_<This> cls(m, "VariableExpression");
    cls.def(nb::init<>())
        .def(nb::init<const std::string&>(), nb::arg("expression"))

        .def("__bool__", &This::operator bool)
        .def("__str__", &This::GetString)
        .def("__repr__", 
            [](const This& expr) {
                return TfStringPrintf("%sVariableExpression('%s')",
                    TF_PY_REPR_PREFIX.c_str(), expr.GetString().c_str());
            })

        .def("GetErrors", &This::GetErrors)

        .def("Evaluate", &This::Evaluate,
            nb::arg("vars"))

        .def_static("IsExpression", &This::IsExpression)

        .def_static("IsValidVariableType", &This::IsValidVariableType,
            nb::arg("value").none())

        .def_static("MakeFunction",
            [](nb::args posArgs, nb::kwargs kwArgs) -> SdfVariableExpression {
                if (nb::len(kwArgs) != 0) {
                    TfPyThrowTypeError("unexpected keyword arguments");
                    return SdfVariableExpression(); // unreachable
                }

                const std::string fnName = nb::cast<std::string>(posArgs[0]);

                auto builder = This::MakeFunction(fnName);
                for (size_t i = 1, n = nb::len(posArgs); i < n; ++i) {
                    builder.AddArgument(nb::cast<SdfVariableExpression>(posArgs[i]));
                }

                return builder;
            })

        .def_static("MakeList",
            [](nb::args posArgs, nb::kwargs kwArgs) -> SdfVariableExpression {
                if (nb::len(kwArgs) != 0) {
                    TfPyThrowTypeError("unexpected keyword arguments");
                    return SdfVariableExpression(); // unreachable
                }

                auto builder = This::MakeList();
                for (nb::handle elem : posArgs) {
                    builder.AddElement(nb::cast<SdfVariableExpression>(elem));
                }
                return builder;
            })

        .def_static("MakeListOfLiterals",
            [](nb::args posArgs, nb::kwargs kwArgs) -> SdfVariableExpression {
                if (nb::len(kwArgs) != 0) {
                    TfPyThrowTypeError("unexpected keyword arguments");
                    return SdfVariableExpression(); // unreachable
                }

                const size_t numArgs = nb::len(posArgs);
                if (numArgs == 1) {
                    // Check if the argument was a list containing any of
                    // the supported literal types.
                    if (nb::isinstance<nb::list>(posArgs[0])) {
                        return _MakeListOfLiteralsExpr(
                            nb::borrow<nb::list>(posArgs[0]));
                    }
                }

                TfPyThrowTypeError(
                    "argument must be an empty list or contain only "
                    "bool, int, or str");
                return SdfVariableExpression();
            })

        .def_static("MakeLiteral",
            [](nb::handle o) -> SdfVariableExpression {
                return _MakeLiteralBuilder(o);
            })

        .def_static("MakeNone",
            []() -> SdfVariableExpression {
                return SdfVariableExpression::MakeNone();
            })

        .def_static("MakeVariable",
            [](const std::string& name, nb::object fallback)
                -> SdfVariableExpression
            {
                if (fallback.is_none()) {
                    return SdfVariableExpression::MakeVariable(name);
                }

                // Result of MakeList or MakeFunction.
                SdfVariableExpression expr;
                if (nb::try_cast<SdfVariableExpression>(fallback, expr)) {
                    return SdfVariableExpression::MakeVariable(name, expr);
                }

                if (nb::isinstance<nb::list>(fallback)) {
                    return SdfVariableExpression::MakeVariable(
                        name,
                        _MakeListOfLiteralsExpr(nb::borrow<nb::list>(fallback)));
                }

                return SdfVariableExpression::MakeVariable(
                    name, _MakeLiteralBuilder(fallback));
            },
            nb::arg("name"), nb::arg("fallbackValue").none() = nb::none())
        ;

    nb::class_<This::Result>(cls, "Result")
        .def_prop_ro("value", 
            [](const This::Result& r) -> nb::object {
                return r.value.IsHolding<This::EmptyList>() ?
                    nb::object(nb::list()) : nb::cast(r.value);
            })
        .def_prop_ro("errors", 
            [](const This::Result& r) { return r.errors; })
        .def_prop_ro("usedVariables", 
            [](const This::Result& r) { return r.usedVariables; })
        ;

}
