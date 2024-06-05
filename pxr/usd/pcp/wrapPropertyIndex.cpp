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

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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

static boost::python::tuple
_WrapBuildPrimPropertyIndex(
    const SdfPath &path, const PcpCache &cache, const PcpPrimIndex &primIndex)
{
    PcpErrorVector errors;
    PcpPropertyIndex propIndex;
    PcpBuildPrimPropertyIndex(path, cache, primIndex, &propIndex, &errors);

    return boost::python::make_tuple(
        boost::python::object(propIndex), errors);
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
