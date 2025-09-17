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

#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

void wrapPathTranslation()
{
    def("TranslatePathFromNodeToRoot", TranslatePathFromNodeToRoot,
        arg("sourceNode"), arg("pathInNodeNamespace"));

    def("TranslatePathFromRootToNode", TranslatePathFromRootToNode,
        arg("destNode"), arg("pathInRootNamespace"));
}
