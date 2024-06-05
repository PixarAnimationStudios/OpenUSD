//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
