//
// Copyright 2020 Pixar
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

#include "pxr/usd/ar/resolvedPath.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_value_policy.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdArWrapResolvedPath {

static std::string
_Repr(const ArResolvedPath& p)
{
    return TfStringPrintf(
        "%sResolvedPath(%s)",
        TF_PY_REPR_PREFIX.c_str(), 
        !p ? "" : TfStringPrintf("'%s'", p.GetPathString().c_str()).c_str());
}

static bool
_NonZero(const ArResolvedPath& p)
{
    return static_cast<bool>(p);
}

} // pxrUsdArWrapResolvedPath

void
wrapResolvedPath()
{
    using This = ArResolvedPath;

    boost::python::class_<This>("ResolvedPath")
        .def(boost::python::init<>())
        .def(boost::python::init<const std::string&>())

        .def(boost::python::self == boost::python::self)
        .def(boost::python::self != boost::python::self)
        .def(boost::python::self < boost::python::self)
        .def(boost::python::self > boost::python::self)
        .def(boost::python::self <= boost::python::self)
        .def(boost::python::self >= boost::python::self)

        .def(boost::python::self == std::string())
        .def(boost::python::self != std::string())
        .def(boost::python::self < std::string())
        .def(boost::python::self > std::string())
        .def(boost::python::self <= std::string())
        .def(boost::python::self >= std::string())

        .def(TfPyBoolBuiltinFuncName, pxrUsdArWrapResolvedPath::_NonZero)
        .def("__hash__", &This::GetHash)
        .def("__repr__", &pxrUsdArWrapResolvedPath::_Repr)
        .def("__str__", &This::GetPathString,
             boost::python::return_value_policy<boost::python::return_by_value>())

        .def("GetPathString", &This::GetPathString,
             boost::python::return_value_policy<boost::python::return_by_value>())
        ;

    boost::python::implicitly_convertible<This, std::string>();
}
