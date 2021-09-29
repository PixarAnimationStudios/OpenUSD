//
// Copyright 2021 Pixar
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

#include "resolverContext.h"

#include "pxr/usd/ar/pyResolverContext.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/return_value_policy.hpp>

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;

static
size_t
_Hash(const UsdResolverExampleResolverContext& ctx)
{
    return hash_value(ctx);
}

static
std::string
_Repr(const UsdResolverExampleResolverContext& ctx)
{
    return TF_PY_REPR_PREFIX + "ResolverContext" +
        (ctx.GetMappingFile().empty() ? "()" : 
            "('" + ctx.GetMappingFile() + "')");
}

void
wrapResolverContext()
{
    using This = UsdResolverExampleResolverContext;

    class_<UsdResolverExampleResolverContext>("ResolverContext")
        .def(init<const std::string&>(args("mappingFile")))
        
        .def("__hash__", _Hash)
        .def("__repr__", _Repr)

        .def("GetMappingFile", &This::GetMappingFile,
            return_value_policy<return_by_value>())
        ;

    ArWrapResolverContextForPython<This>();
}
