//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_PATH_TRANSLATION_H
#define PXR_USD_PCP_PATH_TRANSLATION_H

/// \file pcp/pathTranslation.h
/// Path translation.

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class PcpMapFunction;
class PcpNodeRef;
class SdfPath;

/// Translates \p pathInNodeNamespace from the namespace of the prim index node
/// \p sourceNode to the namespace of the prim index's root node. This applies 
/// all necessary namespace translations.
///
/// If the path is successfully translated and \p pathWasTranslated is supplied,
/// it will be set to \c true. In some cases, paths may fail to translate 
/// because they fall outside the set of paths that are allowed by nodes in the 
/// prim index. For instance, for a referenced model, paths referring to 
/// locations outside that model will not be translated. In these cases, this 
/// function will return an empty SdfPath and \p pathWasTranslated will be set
/// to \c false.
///
/// In Sd/Csd terminology, this is forward path translation from the 
/// namespace of the prim spec represented by \p sourceNode to the composed 
/// scene namespace.
PCP_API
SdfPath
PcpTranslatePathFromNodeToRoot(
    const PcpNodeRef& sourceNode, 
    const SdfPath& pathInNodeNamespace,
    bool* pathWasTranslated = 0);

/// Translates \p pathInRootNamespace from the namespace of the root of the
/// prim index that \p destNode belongs to to the namespace of \p destNode 
/// itself. This applies all necessary namespace translations.
///
/// If the path is successfully translated and \p pathWasTranslated is supplied,
/// it will be set to \c true. In some cases, paths may fail to translate 
/// because they fall outside the set of paths that are allowed by nodes in the 
/// prim index. For instance, for a referenced model, paths referring to 
/// locations outside that model will not be translated. In these cases, this 
/// function will return an empty SdfPath and \p pathWasTranslated will be set
/// to \c false.
///
/// In Sd/Csd terminology, this is reverse path translation from the
/// namespace of the composed scene to the namespace of the prim spec 
/// represented by \p destNode.
PCP_API
SdfPath
PcpTranslatePathFromRootToNode(
    const PcpNodeRef& destNode,
    const SdfPath& pathInRootNamespace,
    bool* pathWasTranslated = 0);

/// Same as \a PcpTranslatePathFromRootToNode, but explicitly for use when
/// translating paths intended for use as attribute connections or relationship
/// targets. The primary difference is that variant selections will never 
/// be included in the translated path. This is functionally equivalent to
/// calling StripAllVariantSelections() on the result of the referenced
/// function, but is more efficient.
PCP_API
SdfPath
PcpTranslateTargetPathFromRootToNode(
    const PcpNodeRef& destNode,
    const SdfPath& pathInRootNamespace,
    bool* pathWasTranslated = 0);

/// Convenience function like \a PcpTranslatePathFromRootToNode, but
/// takes a function rather than a node.
PCP_API
SdfPath
PcpTranslatePathFromRootToNodeUsingFunction(
    const PcpMapFunction &mapToRoot,
    const SdfPath &pathInRootNamespace,
    bool *pathWasTranslated = 0);

/// Convenience function like \a PcpTranslatePathFromNodeToRoot, but
/// takes a function rather than a node.
PCP_API
SdfPath
PcpTranslatePathFromNodeToRootUsingFunction(
    const PcpMapFunction &mapToRoot,
    const SdfPath &pathInNodeNamespace,
    bool *pathWasTranslated = 0);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_PATH_TRANSLATION_H
