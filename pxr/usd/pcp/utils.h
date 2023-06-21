//
// Copyright 2016 Pixar
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
#ifndef PXR_USD_PCP_UTILS_H
#define PXR_USD_PCP_UTILS_H

/// \file pcp/utils.h

#include "pxr/pxr.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/sdf/layer.h"

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class PcpExpressionVariables;
class VtDictionary;

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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_UTILS_H
