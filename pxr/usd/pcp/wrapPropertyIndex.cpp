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

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/sdf/pySpec.h"

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

static nanobind::tuple
_WrapBuildPrimPropertyIndex(
    const SdfPath &path, const PcpCache &cache, const PcpPrimIndex &primIndex)
{
    PcpErrorVector errors;
    PcpPropertyIndex propIndex;
    PcpBuildPrimPropertyIndex(path, cache, primIndex, &propIndex, &errors);

    return nanobind::make_tuple(propIndex, errors);
}

} // anonymous namespace 

void
wrapPropertyIndex(nanobind::module_ &m)
{
    namespace nb = nanobind;
    typedef PcpPropertyIndex This;

    m.def("BuildPrimPropertyIndex", _WrapBuildPrimPropertyIndex);

    nb::class_<This>(m, "PropertyIndex")
        .def_prop_ro("propertyStack", _WrapPropertyStack)
        .def_prop_ro("localPropertyStack", _WrapLocalPropertyStack)
        .def_prop_ro("localErrors", &This::GetLocalErrors)
        ;

}
