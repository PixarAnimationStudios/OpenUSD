//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/site.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static std::string
_PcpSiteStr(const PcpSite& x)
{
    return TfStringify(x);
}

static std::string
_PcpLayerStackSiteStr(const PcpLayerStackSite& x)
{
    return TfStringify(x);
}

} // anonymous namespace 

void wrapSite(nanobind::module_ &m)
{    
    namespace nb = nanobind;

    nb::class_<PcpSite>(m, "Site")
        .def_rw("layerStack", &PcpSite::layerStackIdentifier, nb::rv_policy::copy)
        .def_rw("path", &PcpSite::path, nb::rv_policy::copy)
        .def("__str__", &_PcpSiteStr)
        ;

    nb::class_<PcpLayerStackSite>(m, "LayerStackSite")
        .def_rw("layerStack", &PcpLayerStackSite::layerStack, nb::rv_policy::copy)
        .def_rw("path", &PcpLayerStackSite::path, nb::rv_policy::copy)
        .def("__str__", &_PcpLayerStackSiteStr)
        ;
}
