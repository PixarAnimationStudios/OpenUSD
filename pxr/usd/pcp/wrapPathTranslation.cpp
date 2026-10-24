//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/pathTranslation.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/pyNanobindBridge.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static SdfPath
TranslatePathFromNodeToRoot(const PcpNodeRef& sourceNode,
                            const SdfPath& pathInNodeNamespace)
{
    return PcpTranslatePathFromNodeToRoot(sourceNode, pathInNodeNamespace);
}

static SdfPath
TranslatePathFromRootToNode(const PcpNodeRef& destNode,
                            const SdfPath& pathInRootNamespace)
{
    return PcpTranslatePathFromRootToNode(destNode, pathInRootNamespace);
}

} // anonymous namespace 

void wrapPathTranslation(nanobind::module_ &m)
{
    namespace nb = nanobind;

    m.def("TranslatePathFromNodeToRoot", TranslatePathFromNodeToRoot,
          nb::arg("sourceNode"), nb::arg("pathInNodeNamespace"));

    m.def("TranslatePathFromRootToNode", TranslatePathFromRootToNode,
          nb::arg("destNode"), nb::arg("pathInRootNamespace"));
}
