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
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/tuple.hpp>

#include "pxr/pxr.h"
#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/refPtr.h"

#include <boost/noncopyable.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

class Ar_PyAnnotatedBoolResult
    : public TfPyAnnotatedBoolResult<std::string>
{
public:
    Ar_PyAnnotatedBoolResult(bool val, const std::string& annotation)
        : TfPyAnnotatedBoolResult<std::string>(val, annotation)
    {
    }
};

static 
Ar_PyAnnotatedBoolResult
_CanWriteAssetToPath(
    ArResolver& resolver, const ArResolvedPath& resolvedPath)
{
    std::string whyNot;
    const bool rval = resolver.CanWriteAssetToPath(resolvedPath, &whyNot);
    return Ar_PyAnnotatedBoolResult(rval, whyNot);
}

void
wrapResolver()
{
    Ar_PyAnnotatedBoolResult::Wrap<Ar_PyAnnotatedBoolResult>
        ("_PyAnnotatedBoolResult", "whyNot");

    typedef ArResolver This;

    boost::python::class_<This, boost::noncopyable>
        ("Resolver", boost::python::no_init)

        .def("CreateDefaultContext", &This::CreateDefaultContext)
        .def("CreateDefaultContextForAsset", 
             &This::CreateDefaultContextForAsset,
             boost::python::args("assetPath"))

        .def("CreateContextFromString", 
             (ArResolverContext (This::*)(const std::string&) const)
                 &This::CreateContextFromString,
             boost::python::args("contextStr"))

        .def("CreateContextFromString", 
             (ArResolverContext (This::*)
                 (const std::string&, const std::string&) const)
                 &This::CreateContextFromString,
             (boost::python::arg("uriScheme"), boost::python::arg("contextStr")))

        .def("CreateContextFromStrings", &This::CreateContextFromStrings,
             boost::python::args("contextStrs"))

        .def("GetCurrentContext", &This::GetCurrentContext)

        .def("IsContextDependentPath", &This::IsContextDependentPath,
             boost::python::args("assetPath"))

        .def("CreateIdentifier", &This::CreateIdentifier,
             (boost::python::args("assetPath"), 
              boost::python::args("anchorAssetPath") = ArResolvedPath()))
        .def("CreateIdentifierForNewAsset", &This::CreateIdentifierForNewAsset,
             (boost::python::args("assetPath"), 
              boost::python::args("anchorAssetPath") = ArResolvedPath()))

        .def("Resolve", &This::Resolve,
             (boost::python::args("assetPath")))
        .def("ResolveForNewAsset", &This::ResolveForNewAsset,
             (boost::python::args("assetPath")))

        .def("GetAssetInfo", &This::GetAssetInfo,
             (boost::python::args("assetPath"), boost::python::args("resolvedPath")))
        .def("GetModificationTimestamp", &This::GetModificationTimestamp,
             (boost::python::args("assetPath"), boost::python::args("resolvedPath")))
        .def("GetExtension", &This::GetExtension,
             boost::python::args("assetPath"))

        .def("CanWriteAssetToPath", &_CanWriteAssetToPath,
             boost::python::args("resolvedPath"))

        .def("RefreshContext", &This::RefreshContext)
        ;

    boost::python::def("GetResolver", ArGetResolver,
        boost::python::return_value_policy<boost::python::reference_existing_object>());

    boost::python::def("SetPreferredResolver", ArSetPreferredResolver,
        boost::python::arg("resolverTypeName"));

    boost::python::def("GetUnderlyingResolver", ArGetUnderlyingResolver,
        boost::python::return_value_policy<boost::python::reference_existing_object>());
}

TF_REFPTR_CONST_VOLATILE_GET(ArResolver)
