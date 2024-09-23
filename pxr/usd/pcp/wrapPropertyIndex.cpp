//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/propertyIndex.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/cache.h"

#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static SdfPropertySpecHandleVector
_WrapPropertyStack(const PcpPropertyIndex& propIndex)
{
    const PcpPropertyRange range = propIndex.GetPropertyRange();
    return SdfPropertySpecHandleVector(range.first, range.second);
}

static SdfPropertySpecHandleVector
_WrapLocalPropertyStack(const PcpPropertyIndex& propIndex)
{
    const PcpPropertyRange range = 
        propIndex.GetPropertyRange(/* localOnly= */ true);
    return SdfPropertySpecHandleVector(range.first, range.second);
}

static pxr_boost::python::tuple
_WrapBuildPrimPropertyIndex(
    const SdfPath &path, const PcpCache &cache, const PcpPrimIndex &primIndex)
{
    PcpErrorVector errors;
    PcpPropertyIndex propIndex;
    PcpBuildPrimPropertyIndex(path, cache, primIndex, &propIndex, &errors);

    return pxr_boost::python::make_tuple(
        pxr_boost::python::object(propIndex), errors);
}

} // anonymous namespace 

void
wrapPropertyIndex()
{
    typedef PcpPropertyIndex This;

    def("BuildPrimPropertyIndex", _WrapBuildPrimPropertyIndex);

    class_<This>
        ("PropertyIndex", "", no_init)
        .add_property("propertyStack", _WrapPropertyStack)
        .add_property("localPropertyStack", _WrapLocalPropertyStack)
        .add_property("localErrors", 
                      make_function(&This::GetLocalErrors,
                                    return_value_policy<TfPySequenceToList>()))
        ;
}
