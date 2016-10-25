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
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/return_value_policy.hpp>

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContext.h"

#include <boost/noncopyable.hpp>

using namespace boost::python;

void
wrapResolver()
{
    typedef ArResolver This;

    class_<This, boost::noncopyable>
        ("Resolver", no_init)
        .def("ConfigureResolverForAsset", &This::ConfigureResolverForAsset)
        .def("CreateDefaultContext", &This::CreateDefaultContext)
        .def("CreateDefaultContextForAsset", 
             &This::CreateDefaultContextForAsset)

        .def("AnchorRelativePath", &This::AnchorRelativePath)
        .def("Resolve", &This::Resolve)
        ;

    def("GetResolver", ArGetResolver,
        return_value_policy<reference_existing_object>());
}

#if defined(ARCH_COMPILER_MSVC) 
// There is a bug in the compiler which means we have to provide this
// implementation. See here for more information:
// https://connect.microsoft.com/VisualStudio/Feedback/Details/2852624
namespace boost
{
    template<>
    const volatile ArResolver* get_pointer(const volatile ArResolver* p)
    {
        return p;
    }
}
#endif // #if defined(ARCH_COMPILER_MSVC) 
