//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/resolvedPath.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_value_policy.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static std::string
_Repr(const ArResolvedPath& p)
{
    return TfStringPrintf(
        "%sResolvedPath(%s)",
        TF_PY_REPR_PREFIX.c_str(), 
        !p ? "" : TfStringPrintf("'%s'", p.GetPathString().c_str()).c_str());
}

static bool
_IsValid(const ArResolvedPath& p)
{
    return static_cast<bool>(p);
}

void
wrapResolvedPath()
{
    using This = ArResolvedPath;

    class_<This>("ResolvedPath")
        .def(init<>())
        .def(init<const std::string&>())

        .def(self == self)
        .def(self != self)
        .def(self < self)
        .def(self > self)
        .def(self <= self)
        .def(self >= self)

        .def(self == std::string())
        .def(self != std::string())
        .def(self < std::string())
        .def(self > std::string())
        .def(self <= std::string())
        .def(self >= std::string())

        .def("__bool__", _IsValid)
        .def("__hash__", &This::GetHash)
        .def("__repr__", &_Repr)
        .def("__str__", &This::GetPathString,
             return_value_policy<return_by_value>())

        .def("GetPathString", &This::GetPathString,
             return_value_policy<return_by_value>())
        ;

    implicitly_convertible<This, std::string>();
}
