//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/sdf/variableExpression.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/vt/value.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/list.hpp"
#include "pxr/external/boost/python/raw_function.hpp"
#include "pxr/external/boost/python/scope.hpp"
#include "pxr/external/boost/python/stl_iterator.hpp"
#include "pxr/external/boost/python/tuple.hpp"

#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

template <class T>
static bool
_IsHolding(const object& obj)
{
    // extract<bool> will allow conversions from int to bool, so we need
    // directly check whether the argument is a bool using Python API.
    if constexpr (std::is_same_v<T, bool>) {
        return PyBool_Check(obj.ptr());
    }
    else {
        return extract<T>(obj).check();
    }
}

  static SdfVariableExpression::Builder
  _MakeLiteralBuilder(const object& o)
  {
      if (_IsHolding<bool>(o)) {
          return SdfVariableExpression::MakeLiteral(extract<bool>(o)());
      }
      else if (_IsHolding<int64_t>(o)) {
          return SdfVariableExpression::MakeLiteral(extract<int64_t>(o)());
      }
      else if (_IsHolding<std::string>(o)) {
          return SdfVariableExpression::MakeLiteral(extract<std::string>(o)());
      }
      TfPyThrowTypeError("literal argument must be bool, int, or str");
      return SdfVariableExpression::MakeNone(); // unreachable
  }

  static SdfVariableExpression
  _MakeListOfLiteralsExpr(const list& elems)
  {
      if (len(elems) == 0) {
          return SdfVariableExpression::MakeList();
      }
      else if (_IsHolding<bool>(elems[0])) {
          return SdfVariableExpression::MakeListOfLiterals(
              extract<std::vector<bool>>(elems)());
      }
      else if (_IsHolding<int64_t>(elems[0])) {
          return SdfVariableExpression::MakeListOfLiterals(
              extract<std::vector<int64_t>>(elems)());
      }
      else if (_IsHolding<std::string>(elems[0])) {
          return SdfVariableExpression::MakeListOfLiterals(
              extract<std::vector<std::string>>(elems)());
      }

      TfPyThrowTypeError(
          "argument must be an empty list or contain only bool, int, or str");
      return SdfVariableExpression(); // unreachable
  }


void
wrapVariableExpression()
{
    using This = SdfVariableExpression;

    scope s = class_<This>("VariableExpression")
        .def(init<>())
        .def(init<const std::string&>(arg("expression")))

        .def("__bool__", &This::operator bool)
        .def("__str__", &This::GetString,
            return_value_policy<return_by_value>())
        .def("__repr__", 
            +[](const This& expr) {
                return TfStringPrintf("%sVariableExpression('%s')",
                    TF_PY_REPR_PREFIX.c_str(), expr.GetString().c_str());
            })

        .def("GetErrors", &This::GetErrors,
            return_value_policy<TfPySequenceToList>())

        .def("Evaluate", &This::Evaluate,
            arg("vars"))

        .def("IsExpression", &This::IsExpression)
        .staticmethod("IsExpression")

        .def("IsValidVariableType", &This::IsValidVariableType)
        .staticmethod("IsValidVariableType")

        .def("MakeFunction", raw_function(
            +[](tuple posArgs, dict kwArgs) -> SdfVariableExpression {
                if (len(kwArgs) != 0) {
                    TfPyThrowTypeError("unexpected keyword arguments");
                    return SdfVariableExpression(); // unreachable
                }

                const std::string fnName = extract<std::string>(posArgs[0]);
                const object fnArgs = posArgs.slice(1, _);

                auto builder = This::MakeFunction(fnName);
                for (stl_input_iterator<SdfVariableExpression> i(fnArgs), e;
                     i != e; ++i) {
                    builder.AddArgument(*i);
                }

                return builder;
            }))
        .staticmethod("MakeFunction")

        .def("MakeList", raw_function(
            +[](tuple posArgs, dict kwArgs) -> SdfVariableExpression {
                if (len(kwArgs) != 0) {
                    TfPyThrowTypeError("unexpected keyword arguments");
                    return SdfVariableExpression(); // unreachable
                }

                auto builder = This::MakeList();
                for (stl_input_iterator<SdfVariableExpression> i(posArgs), e;
                     i != e; ++i) {
                    builder.AddElement(*i);
                }
                return builder;
            }))
        .staticmethod("MakeList")

        .def("MakeListOfLiterals", raw_function(
            +[](tuple posArgs, dict kwArgs) -> SdfVariableExpression {
                if (len(kwArgs) != 0) {
                    TfPyThrowTypeError("unexpected keyword arguments");
                    return SdfVariableExpression(); // unreachable
                }

                const size_t numArgs = len(posArgs);
                if (numArgs == 1) {
                    // Check if the argument was a list containing any of
                    // the supported literal types.
                    const extract<list> listExtract(posArgs[0]);
                    if (listExtract.check()) {
                        return _MakeListOfLiteralsExpr(listExtract());
                    }
                }

                TfPyThrowTypeError(
                    "argument must be an empty list or contain only "
                    "bool, int, or str");
                return SdfVariableExpression();
            }))
        .staticmethod("MakeListOfLiterals")

        .def("MakeLiteral",
            +[](object o) -> SdfVariableExpression {
                return _MakeLiteralBuilder(o);
            })
        .staticmethod("MakeLiteral")

        .def("MakeNone",
            +[]() -> SdfVariableExpression {
                return SdfVariableExpression::MakeNone();
            })
        .staticmethod("MakeNone")

        .def("MakeVariable",
            +[](const std::string& name, object fallback)
                -> SdfVariableExpression
            {
                if (fallback.is_none()) {
                    return SdfVariableExpression::MakeVariable(name);
                }

                // Result of MakeList or MakeFunction.
                const extract<SdfVariableExpression> exprExtract(fallback);
                if (exprExtract.check()) {
                    return SdfVariableExpression::MakeVariable(
                        name, exprExtract());
                }

                const extract<list> listExtract(fallback);
                if (listExtract.check()) {
                    return SdfVariableExpression::MakeVariable(
                        name, _MakeListOfLiteralsExpr(listExtract()));
                }

                return SdfVariableExpression::MakeVariable(
                    name, _MakeLiteralBuilder(fallback));
            },
            (arg("name"), arg("fallbackValue") = object()))
        .staticmethod("MakeVariable")
        ;

    class_<This::Result>("Result", no_init)
        .add_property("value", 
            +[](const This::Result& r) {
                return r.value.IsHolding<This::EmptyList>() ?
                    object(list()) : object(r.value);
            })
        .add_property("errors", 
            make_getter(
                &This::Result::errors,
                return_value_policy<TfPySequenceToList>()))
        .add_property("usedVariables", 
            make_getter(
                &This::Result::usedVariables,
                return_value_policy<TfPySequenceToSet>()))
        ;
}
