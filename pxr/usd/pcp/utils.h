//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_UTILS_H
#define PXR_USD_PCP_UTILS_H

/// \file pcp/utils.h

#include "pxr/pxr.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/primIndex_Graph.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class PcpExpressionVariables;
class VtDictionary;

// Tokens for fields that have been removed from Sdf but still need to be
// accessed by Pcp for backwards compatibility or legacy reasons.
#define PCP_FIELD_TOKENS \
    ((primRelocates, "relocates"))

TF_DECLARE_PUBLIC_TOKENS(Pcp_Fields, PCP_FIELD_TOKENS);

// Returns the result of evaluating the variable expression \p expression using
// the variables \p expressionVars. Variables that are used during evaluation
// will be inserted in \p usedVariables. Any errors that occur during evaluation
// will be appended to \p errors. \p context (a string like "sublayer" or
// "reference"), \p sourceLayer, and \p sourcePath are used to describe the
// source of the expression for use in error messages.
// 
// The result of \p expression is assumed to be string-valued; if it is not, an
// error will be generated and the empty string will be returned.
std::string
Pcp_EvaluateVariableExpression(
    const std::string& expression,
    const PcpExpressionVariables& expressionVars,
    const std::string& context,
    const SdfLayerHandle& sourceLayer,
    const SdfPath& sourcePath,
    std::unordered_set<std::string>* usedVariables,
    PcpErrorVector* errors);

// Convenience overload of above that does not populate \p usedVariables or 
// \p errors.
std::string
Pcp_EvaluateVariableExpression(
    const std::string& expression,
    const PcpExpressionVariables& expressionVars);

// Returns true if \p str is a variable expression, false otherwise.
bool
Pcp_IsVariableExpression(
    const std::string& str);

// Returns an SdfLayer::FileFormatArguments object with the "target" argument
// set to \p target if \p target is not empty.
SdfLayer::FileFormatArguments 
Pcp_GetArgumentsForFileFormatTarget(
    const std::string& target);

// Returns an SdfLayer::FileFormatArguments object with the "target" argument
// set to \p target if \p target is not empty and a target is
// not embedded within the given \p identifier.
SdfLayer::FileFormatArguments 
Pcp_GetArgumentsForFileFormatTarget(
    const std::string& identifier,
    const std::string& target);

// \overload
// Same as above, but modifies \p args instead of returning by value.
void 
Pcp_GetArgumentsForFileFormatTarget(
    const std::string& identifier,
    const std::string& target,
    SdfLayer::FileFormatArguments* args);

// If a target argument is embedded in \p identifier, copies contents of 
// \p defaultArgs to \p localArgs, removes the "target" argument, and returns
// a const reference to \p localArgs. Otherwise, returns a const reference to
// \p defaultArgs. This lets us avoid making a copy of \p defaultArgs unless 
// needed.
const SdfLayer::FileFormatArguments&
Pcp_GetArgumentsForFileFormatTarget(
    const std::string& identifier,
    const SdfLayer::FileFormatArguments* defaultArgs,
    SdfLayer::FileFormatArguments* localArgs);

// Removes the "target" argument from \p args if it exists and its value
// is the same as \p target.
void
Pcp_StripFileFormatTarget(
    const std::string& target,
    SdfLayer::FileFormatArguments* args);

// Find the starting node of the class hierarchy of which node n is a part.
// This is the prim that starts the class chain, aka the 'instance' of the
// class hierarchy. Also returns the node for the first class in the
// chain that the instance inherits opinions from.
//
// For example, consider an inherits chain like this: I --> C1 --> C2 --> C3.  
// When given either C1, C2, or C3, this method will return (I, C1).
// What will it do when given I?  Keep reading.
//
// One tricky aspect is that we need to distinguish nested class
// hierarchies at different levels of namespace, aka ancestral classes.
// Returning to the example above, consider if I -> ... -> C3 were all
// nested as sibling children under a class, G, with instance M:
//
//          inherits
// M ------------------------> G
// |                           |                 
// +- I  (depth=0)             +- I  (depth=1)
// |  :                        |  :
// |  : inherits               |  : inherits
// |  v                        |  v
// +- C1 (depth=2)             +- C1 (depth=2)
// |  :                        |  :
// |  : inherits               |  : inherits
// |  v                        |  v
// +- C2 (depth=2)             +- C2 (depth=2)
// |  :                        |  :
// |  : inherits               |  : inherits
// |  v                        |  v
// +- C3 (depth=2)             +- C3 (depth=2)
//
// "depth" refers to the value of PcpNode::GetNamespaceDepth, which is the
// number of path components in the parent of the node when that node was
// introduced. Note that M/I's depth is 0 as it is the root node of the
// prim index for M/I and G/I's depth is 1 because the node was actually 
// introduced in the ancestral inherit arc between M and G.
//
// Asking for the starting node of M/C1 .. M/C3 should all return (M/I, M/C1).
// Asking for the starting node of G/C1 .. G/C3 should all return (G/I, G/C1).
//
// However, asking for the starting node of G/I should return (M/I, G/I),
// because it is walking up the ancestral classes (M->G) instead.
//
// We distinguish ancestral class chains by considering, for the
// nodes being examined, how far they are below the point in namespace
// where they were introduced, using PcpNode::GetDepthBelowIntroduction().
// This function returns the difference between number of path components
// in the parent node and the recorded namespace depth.
//
// This lets us distinguish the hierarchy connecting the children G/C1, G/C2,
// and G/C3 (all with parent path components = 2 and depth=2) from the ancestral
// hierarchy connecting G/I to M/I, which was introduced at depth=1 and thus up
// one level of ancestry.
//
// Note that this approach also handles a chain of classes that
// happen to live at different levels of namespace but which are not
// ancestrally connected to one another.  For example, consider if C2 
// was tucked under a parent scope D:
//
//          inherits
// M ------------------------> G
// |                           |                 
// +- I  (depth=0)             +- I  (depth=1)  
// |  :                        |  :             
// |  : inherits               |  : inherits    
// |  v                        |  v             
// +- C1 (depth=2)             +- C1 (depth=2)  
// |    :                      |    :           
// +- D  : inherits            +- D  : inherits
// |  |  v                     |  |  v          
// |  +- C2 (depth=2)          |  +- C2 (depth=2)
// |    :                      |    :          
// |   : inherits              |   : inherits 
// |  v                        |  v          
// +- C3 (depth=3)             +- C3 (depth=3)
//
// Here, G/C1, G/D/C2, and G/C3 are all still identified as part of
// the same hierarchy because they have the same value of 0 for
// GetDepthBelowIntroduction() -- G/C1 and G/D/C2 have 2 parent path
// components and depth 2, and G/C3 has 3 parent path components and
// depth 3.
//
// If this function used namespace depth to identify hierarchies
// instead of depth below introduction, G/C3 would have been
// incorrectly excluded.
std::pair<PcpNodeRef, PcpNodeRef>
Pcp_FindStartingNodeOfClassHierarchy(const PcpNodeRef& n);

// Translate the given path (which must be a prim or prim variant selection
// path) from the namespace of the given node to the namespace of the root node
// of the prim index that node belongs to. If that translation succeeds, returns
// the translated path and the root node. If that translation fails, translate
// the path to the ancestor node closest to the root node where the mapping is
// successful and return the translated path and the ancestor node.
std::pair<SdfPath, PcpNodeRef>
Pcp_TranslatePathFromNodeToRootOrClosestNode(
    const PcpNodeRef& node,
    const SdfPath& path);

// Returns true if the given node is a specializes node that
// has been propagated to the root of the graph for strength
// ordering purposes.
inline bool
Pcp_IsPropagatedSpecializesNode(
    const PcpNodeRef& node)
{
    return (PcpIsSpecializeArc(node._graph->GetArcType(node._nodeIdx)) &&
            node.GetParentNode() == node.GetRootNode() &&
            node.HasSameSiteAs(node.GetOriginNode()));
}

// Returns true if the given node was elided during prim indexing, so that
// neither it nor any node implied from it may contribute opinions.  Unlike
// culling, this is independent of PcpPrimIndexInputs::Cull, and unlike
// inertness, it distinguishes these nodes from placeholders.
inline bool
Pcp_IsElided(
    const PcpNodeRef& node)
{
    return node._graph->_unshared[node._nodeIdx].elided;
}

// Marks the given node as elided.  See Pcp_IsElided.
inline void
Pcp_SetElided(
    const PcpNodeRef& node)
{
    node._graph->_unshared[node._nodeIdx].elided = true;
}

// Returns the path element count, in the given node's layer stack, of the
// namespace of the instance that sealed the node, or 0 if the node is not
// sealed.  A node is sealed when it is made inert because a namespace
// ancestor of the prim index is an instance, and when it is added beneath a
// sealed node, unless it is a class arc to a path outside that namespace.
// Sealed nodes lie outside the arcs that the instance key records and their
// opinions are about the instance's descendants, so they may not contribute:
// instances with equal keys must compose equal descendants.  The count is
// that of the node's own path when it was first sealed, and nodes sealed
// beneath it copy it.
inline size_t
Pcp_GetSealedDepth(
    const PcpNodeRef& node)
{
    return node._graph->_GetNode(node._nodeIdx).smallInts.sealedDepth;
}

// Returns true if the given node is sealed.  See Pcp_GetSealedDepth.
inline bool
Pcp_IsSealed(
    const PcpNodeRef& node)
{
    return Pcp_GetSealedDepth(node) != 0;
}

// Returns the path, in the given sealed node's layer stack, of the instance
// that sealed it: the node's path, without variant selections, truncated to
// the sealed depth.  Returns the empty path if the node is not sealed, or
// if its path is shallower than the sealed depth, which is a coding error.
inline SdfPath
Pcp_GetSealingInstancePath(
    const PcpNodeRef& node)
{
    const size_t depth = Pcp_GetSealedDepth(node);
    if (depth == 0) {
        return SdfPath();
    }
    SdfPath path = node.GetPath().StripAllVariantSelections();
    if (!TF_VERIFY(path.GetPathElementCount() >= depth,
                   "Sealed node <%s> is shallower than its instance",
                   node.GetPath().GetText())) {
        return SdfPath();
    }
    while (path.GetPathElementCount() > depth) {
        path = path.GetParentPath();
    }
    return path;
}

// Marks the given node as sealed by an instance whose namespace has the
// given nonzero path element count.  See Pcp_GetSealedDepth.
inline void
Pcp_SetSealed(
    const PcpNodeRef& node, uint16_t depth)
{
    // Skip unchanged writes so they do not detach a shared node pool.
    if (node._graph->_GetNode(node._nodeIdx).smallInts.sealedDepth != depth) {
        node._graph->_GetWriteableNode(node._nodeIdx).smallInts.sealedDepth =
            depth;
    }
}

// Returns true if an arc at this node's level of namespace, which would have
// been a direct dependency, was skipped because this node, or the root of a
// subtree containing it, has the same site.  The node then provides the
// opinions that arc's subtree would have, so instancing treats it as part of
// the instance even if it is ancestral.
inline bool
Pcp_IsDirectDuplicate(
    const PcpNodeRef& node)
{
    return node._graph->_unshared[node._nodeIdx].directDuplicate;
}

// Sets or clears the flag returned by Pcp_IsDirectDuplicate.
inline void
Pcp_SetDirectDuplicate(
    const PcpNodeRef& node, bool value)
{
    node._graph->_unshared[node._nodeIdx].directDuplicate = value;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_UTILS_H
