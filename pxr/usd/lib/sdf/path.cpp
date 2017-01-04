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
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathNode.h"
#include "pxr/usd/sdf/pathParser.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/tracelite/trace.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include <boost/functional/hash.hpp>

using std::pair;
using std::string;
using std::vector;


static inline bool _IsValidIdentifier(TfToken const &name);

static VtValue
_CastFromSdfPathToTfToken(const VtValue &val)
{
    return VtValue(val.Get<SdfPath>().GetToken());
}

// XXX: Enable this define to make bad path strings
// cause runtime errors.  This can be useful when trying to track down cases
// of bad path strings originating from python code.
// #define PARSE_ERRORS_ARE_ERRORS

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfPath>();
    TfType::Define< vector<SdfPath> >()
        .Alias(TfType::GetRoot(), "vector<SdfPath>");    
}

// Register with VtValue that SdfPaths can be cast to TfTokens.  The only
// reason we need this is because we need to cast AnimSplines that contain
// SdfPaths to ones that contain TfTokens, and we need that to succeed.  The
// only reason we need that, is in execution we can't use SdfPaths directly
// due to performance and threadsafety reasons.
TF_REGISTRY_FUNCTION(VtValue)
{
    VtValue::RegisterCast<SdfPath, TfToken>(&_CastFromSdfPathToTfToken);
}

void
SdfPath::_InitWithString(const std::string &path) {

    TfAutoMallocTag2 tag("Sdf", "SdfPath");
    TfAutoMallocTag tag2("SdfPath::_InitWithString");
    TRACE_FUNCTION();

    Sdf_PathParserContext context;

    // Initialize the scanner, allowing it to be reentrant.
    pathYylex_init(&context.scanner);

    yy_buffer_state *b = pathYy_scan_bytes(path.c_str(), path.size(),
                                           context.scanner);
    if( pathYyparse(&context) != 0 ) {
#ifdef PARSE_ERRORS_ARE_ERRORS
        TF_RUNTIME_ERROR("Ill-formed SdfPath <%s>: %s",
                         path.c_str(), context.errStr.c_str());
#else
        TF_WARN("Ill-formed SdfPath <%s>: %s",
                path.c_str(), context.errStr.c_str());
#endif
        _pathNode.reset();
    } else {
        _pathNode.swap(context.node);
    }

    // Clean up.
    pathYy_delete_buffer(b, context.scanner);
    pathYylex_destroy(context.scanner);
}

SdfPath::SdfPath() {}

SdfPath::SdfPath(const std::string &path) {
    _InitWithString(path);
}

SdfPath::SdfPath(const Sdf_PathNodeConstRefPtr &pathNode) : _pathNode(pathNode) {}

TF_MAKE_STATIC_DATA(SdfPath, _emptyPath) {
    *_emptyPath = SdfPath();
}
TF_MAKE_STATIC_DATA(SdfPath, _absoluteRootPath) {
    *_absoluteRootPath = SdfPath("/");
}
TF_MAKE_STATIC_DATA(SdfPath, _relativeRootPath) {
    *_relativeRootPath = SdfPath(".");
}

const SdfPath & SdfPath::EmptyPath()
{
    return *_emptyPath;
}

const SdfPath & SdfPath::AbsoluteRootPath()
{
    return *_absoluteRootPath;
}

const SdfPath & SdfPath::ReflexiveRelativePath()
{
    return *_relativeRootPath;
}

size_t
SdfPath::GetPathElementCount() const {
    if (Sdf_PathNode const *node = boost::get_pointer(_pathNode))
        return node->GetElementCount();
    return 0;
}

bool
SdfPath::IsAbsolutePath() const {
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node && node->IsAbsolutePath();
}

bool
SdfPath::IsPrimPath() const {
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node &&
        ((node->GetNodeType() == Sdf_PathNode::PrimNode) ||
         (_pathNode == ReflexiveRelativePath()._pathNode));
}

bool
SdfPath::IsAbsoluteRootOrPrimPath() const {
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node &&
        ((node->GetNodeType() == Sdf_PathNode::PrimNode) ||
         (_pathNode == AbsoluteRootPath()._pathNode)     || 
         (_pathNode == ReflexiveRelativePath()._pathNode));
}

bool
SdfPath::IsRootPrimPath() const {
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node && node->IsAbsolutePath() && node->GetElementCount() == 1;
}

bool
SdfPath::IsPropertyPath() const {
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node &&
        ((node->GetNodeType() == Sdf_PathNode::PrimPropertyNode) ||
         (node->GetNodeType() == Sdf_PathNode::RelationalAttributeNode));
}

bool
SdfPath::IsPrimPropertyPath() const {
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node && node->GetNodeType() ==  Sdf_PathNode::PrimPropertyNode;
}

bool
SdfPath::IsNamespacedPropertyPath() const {
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node && node->IsNamespaced() && 
        // Currently this subexpression is always true is IsNamespaced() is.
        ((node->GetNodeType() == Sdf_PathNode::PrimPropertyNode) || 
         (node->GetNodeType() == Sdf_PathNode::RelationalAttributeNode));
}

bool
SdfPath::IsPrimVariantSelectionPath() const
{
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node && 
        node->GetNodeType() == Sdf_PathNode::PrimVariantSelectionNode;
}

bool
SdfPath::IsPrimOrPrimVariantSelectionPath() const
{
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node && 
        ((node->GetNodeType() == Sdf_PathNode::PrimNode) || 
         (node->GetNodeType() == Sdf_PathNode::PrimVariantSelectionNode) || 
         (_pathNode == ReflexiveRelativePath()._pathNode));
}

bool
SdfPath::ContainsPrimVariantSelection() const
{
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node && node->ContainsPrimVariantSelection();
}

bool
SdfPath::ContainsTargetPath() const
{
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node && node->ContainsTargetPath();
}

bool
SdfPath::IsRelationalAttributePath() const {
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node && 
        node->GetNodeType() ==  Sdf_PathNode::RelationalAttributeNode;
}

bool
SdfPath::IsTargetPath() const {
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node && node->GetNodeType() == Sdf_PathNode::TargetNode;
}

bool
SdfPath::IsMapperPath() const {
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node && node->GetNodeType() == Sdf_PathNode::MapperNode;
}

bool
SdfPath::IsMapperArgPath() const {
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node && node->GetNodeType() == Sdf_PathNode::MapperArgNode;
}

bool
SdfPath::IsExpressionPath() const {
    Sdf_PathNode const *node = boost::get_pointer(_pathNode);
    return node && node->GetNodeType() == Sdf_PathNode::ExpressionNode;
}

bool
SdfPath::IsEmpty() const {
    return !_pathNode;
}

TfToken const &
SdfPath::GetToken() const
{
    if (Sdf_PathNode const *node = boost::get_pointer(_pathNode))
        return node->GetPathToken();
    return SdfPathTokens->empty;
}

const std::string &
SdfPath::GetString() const
{
    return GetToken().GetString();
}

const char *
SdfPath::GetText() const
{
    return GetToken().GetText();
}

SdfPathVector
SdfPath::GetPrefixes() const {
    SdfPathVector result;
    GetPrefixes(&result);
    return result;
}

void
SdfPath::GetPrefixes(SdfPathVector *prefixes) const
{
    if (Sdf_PathNode const *node = boost::get_pointer(_pathNode))
        node->GetPrefixes(prefixes);
}

const std::string &
SdfPath::GetName() const
{
    return GetNameToken().GetString();
}

const TfToken &
SdfPath::GetNameToken() const
{
    if (Sdf_PathNode const *node = boost::get_pointer(_pathNode))
        return node->GetName();
    return SdfPathTokens->empty;
}

string
SdfPath::GetElementString() const
{
    return GetElementToken().GetString();
}    

TfToken
SdfPath::GetElementToken() const
{
    return ARCH_LIKELY(_pathNode) ? _pathNode->GetElement() : TfToken();
}    

SdfPath
SdfPath::ReplaceName(TfToken const &newName) const
{
    if (IsPrimPath())
        return GetParentPath().AppendChild(newName);
    else if (IsPrimPropertyPath())
        return GetParentPath().AppendProperty(newName);
    else if (IsRelationalAttributePath())
        return GetParentPath().AppendRelationalAttribute(newName);

    TF_CODING_ERROR("%s is not a prim, property, "
                    "or relational attribute path", GetText());
    return SdfPath();
}

static Sdf_PathNodeConstRefPtr
_GetNextTargetNode(Sdf_PathNodeConstRefPtr curNode)
{
    if (!curNode || !curNode->ContainsTargetPath())
        return Sdf_PathNodeConstRefPtr();

    // Find nearest target or mapper node.
    while (curNode
           && curNode->GetNodeType() != Sdf_PathNode::TargetNode
           && curNode->GetNodeType() != Sdf_PathNode::MapperNode) {
        curNode = curNode->GetParentNode();
    }
    return curNode;
}

const SdfPath &
SdfPath::GetTargetPath() const {
    Sdf_PathNodeConstRefPtr targetNode = _GetNextTargetNode(_pathNode);
    return targetNode ? targetNode->GetTargetPath() : EmptyPath();
}

void
SdfPath::GetAllTargetPathsRecursively(SdfPathVector *result) const {
    for (Sdf_PathNodeConstRefPtr curNode = _GetNextTargetNode(_pathNode);
         curNode; curNode = _GetNextTargetNode(curNode->GetParentNode())) {
        SdfPath targetPath = curNode->GetTargetPath();
        result->push_back(targetPath);
        targetPath.GetAllTargetPathsRecursively(result);
    }
}

const pair<string, string>
SdfPath::GetVariantSelection() const
{
    TRACE_FUNCTION();

    if (not ContainsPrimVariantSelection())
        return pair<string, string>();

    // Find nearest variant selection node
    Sdf_PathNodeConstRefPtr curNode = _pathNode;
    while (curNode and curNode->GetNodeType() !=
                Sdf_PathNode::PrimVariantSelectionNode) {
        curNode = curNode->GetParentNode();
    }
    if (not curNode)
        return pair<string, string>();
    const Sdf_PathNode::VariantSelectionType& sel =
        curNode->GetVariantSelection();
    return pair<string, string>(sel.first.GetString(), sel.second.GetString());
}

bool
SdfPath::HasPrefix(const SdfPath &prefix) const
{
    if (prefix.IsEmpty() || IsEmpty())
        return false;

    if (IsAbsolutePath() && prefix == SdfPath::AbsoluteRootPath())
        return true;

    const Sdf_PathNodeConstRefPtr &prefixNode = prefix._pathNode;
    const size_t prefixDepth = prefixNode->GetElementCount();

    const Sdf_PathNodeConstRefPtr *curNode = &_pathNode;
    size_t curDepth = (*curNode)->GetElementCount();

    if (curDepth < prefixDepth)
        return false;

    while (curDepth > prefixDepth) {
        curNode = &((*curNode)->GetParentNode());
        --curDepth;
    }

    return *curNode == prefixNode;
}

SdfPath
SdfPath::GetParentPath() const {
    if (!_pathNode) {
        return EmptyPath();
    }
    if (ARCH_UNLIKELY(
            (_pathNode == Sdf_PathNode::GetRelativeRootNode()) || 
            (_pathNode->GetName() == SdfPathTokens->parentPathElement))) {
        // If this is the relative root ("." or the last path component
        // is "..", then the "parent" path is actually a new path with
        // and extra ".." appended.
        //
        // XXX: NOTE that this is NOT the way that
        // that Sdf_PathNode::GetParentNode works, and note that most of the
        // code in SdfPath uses GetParentNode intentionally.
        return SdfPath(Sdf_PathNode::
                      FindOrCreatePrim(_pathNode,
                                       SdfPathTokens->parentPathElement));
    } else {
        Sdf_PathNodeConstRefPtr const &parent = _pathNode->GetParentNode();
        return (parent ? SdfPath(parent) : EmptyPath());
    }
}

SdfPath
SdfPath::GetPrimPath() const {
    Sdf_PathNodeConstRefPtr curNode = _pathNode;
    // Walk up looking for a prim node.
    while (curNode) {
        if (curNode->GetNodeType() == Sdf_PathNode::PrimNode) {
            return SdfPath(curNode);
        }
        curNode = curNode->GetParentNode();
    }
    return EmptyPath();
}

SdfPath
SdfPath::GetPrimOrPrimVariantSelectionPath() const
{
    Sdf_PathNodeConstRefPtr curNode = _pathNode;
    // Walk up looking for a prim or prim variant selection node.
    while (curNode) {
        if (curNode->GetNodeType() == Sdf_PathNode::PrimNode || 
            curNode->GetNodeType() == Sdf_PathNode::PrimVariantSelectionNode) {
            return SdfPath(curNode);
        }
        curNode = curNode->GetParentNode();
    }
    return EmptyPath();
}

SdfPath
SdfPath::GetAbsoluteRootOrPrimPath() const {
    return (*this == AbsoluteRootPath()) ? *this : GetPrimPath();
}

static SdfPath
_AppendNode(const SdfPath &path, const Sdf_PathNodeConstRefPtr &node) {

    switch (node->GetNodeType()) {
        case Sdf_PathNode::PrimNode:
            return path.AppendChild(node->GetName());
        case Sdf_PathNode::PrimPropertyNode:
            return path.AppendProperty(node->GetName());
        case Sdf_PathNode::PrimVariantSelectionNode:
        {
            const Sdf_PathNode::VariantSelectionType& selection =
                node->GetVariantSelection();
            return path.AppendVariantSelection(selection.first.GetString(),
                                               selection.second.GetString());
        }
        case Sdf_PathNode::TargetNode:
            return path.AppendTarget( node->GetTargetPath());
        case Sdf_PathNode::RelationalAttributeNode:
            return path.AppendRelationalAttribute(node->GetName());
        case Sdf_PathNode::MapperNode:
            return path.AppendMapper(node->GetTargetPath());
        case Sdf_PathNode::MapperArgNode:
            return path.AppendMapperArg(node->GetName());
        case Sdf_PathNode::ExpressionNode:
            return path.AppendExpression();
        default:
            // CODE_COVERAGE_OFF
            // Should never get here.  All reasonable cases are
            // handled above.
            TF_CODING_ERROR("Unexpected node type %i", node->GetNodeType());
            return SdfPath::EmptyPath();
            // CODE_COVERAGE_ON
    }
}

SdfPath
SdfPath::StripAllVariantSelections() const {
    if (!ContainsPrimVariantSelection())
        return *this;
    TRACE_FUNCTION();
    std::vector<Sdf_PathNodeConstRefPtr> nodes;
    Sdf_PathNodeConstRefPtr curNode = _pathNode;
    while(curNode) {
        if (curNode->GetNodeType() != Sdf_PathNode::PrimVariantSelectionNode)
            nodes.push_back(curNode);
        curNode = curNode->GetParentNode();
    }

    vector<Sdf_PathNodeConstRefPtr>::reverse_iterator it;
    SdfPath stripPath(*(nodes.rbegin()));    
    // Step through all nodes except the last (which is the root node):
    for (it = ++(nodes.rbegin()); it != nodes.rend(); ++it) 
        stripPath = _AppendNode(stripPath, *it);
    return stripPath;
}


SdfPath
SdfPath::AppendPath(const SdfPath &newSuffix) const {
    if (*this == EmptyPath()) {
        TF_CODING_ERROR("Cannot append to invalid path");
        return EmptyPath();
    }
    if (newSuffix == EmptyPath()) {
        TF_CODING_ERROR("Cannot append invalid path to <%s>",
                        GetString().c_str());
        return EmptyPath();
    }
    if (newSuffix.IsAbsolutePath()) {
        TF_WARN("Cannot append absolute path <%s> to another path <%s>.",
                newSuffix.GetString().c_str(), GetString().c_str());
        return EmptyPath();
    }
    if (newSuffix == ReflexiveRelativePath()) {
        return *this;
    }
    if ((_pathNode->GetNodeType() != Sdf_PathNode::RootNode) && 
        (_pathNode->GetNodeType() != Sdf_PathNode::PrimNode) && 
        (_pathNode->GetNodeType() != Sdf_PathNode::PrimVariantSelectionNode)) {
        TF_WARN("Cannot append a path to another path that is not "
                    "a root or a prim path.");
        return EmptyPath();
    }

    // This list winds up in reverse order to what one might at first expect.
    vector<Sdf_PathNodeConstRefPtr> tailNodes;

    Sdf_PathNodeConstRefPtr curNode = newSuffix._pathNode;
    // Walk up to top of newSuffix.
    while (curNode != Sdf_PathNode::GetRelativeRootNode()) {
        tailNodes.push_back(curNode);
        curNode = curNode->GetParentNode();
    }
    if ((tailNodes.back()->GetNodeType() == Sdf_PathNode::PrimPropertyNode) && 
                (_pathNode == Sdf_PathNode::GetAbsoluteRootNode())) {
        TF_WARN("Cannot append a property path to the absolute root path.");
        return EmptyPath();
    }

    SdfPath result = *this;

    // We have a list of new nodes (in reverse order) to append to our node.
    vector<Sdf_PathNodeConstRefPtr>::reverse_iterator it = tailNodes.rbegin();
    while ((it != tailNodes.rend()) && (result != EmptyPath())) {
        result = _AppendNode(result, *it);
        ++it;
    }
    return result;
}

SdfPath
SdfPath::AppendChild(TfToken const &childName) const {
    if (!IsAbsoluteRootOrPrimPath()
        && !IsPrimVariantSelectionPath()
        && (*this != ReflexiveRelativePath())) {
        TF_WARN("Cannot append child '%s' to path '%s'.",
                childName.GetText(), GetText());
        return EmptyPath();
    }
    if (ARCH_UNLIKELY(childName == SdfPathTokens->parentPathElement)) {
        return GetParentPath();
    } else {
        if (ARCH_UNLIKELY(!_IsValidIdentifier(childName))) {
            TF_WARN("Invalid prim name '%s'", childName.GetText());
            return EmptyPath();
        }
        return SdfPath(Sdf_PathNode::FindOrCreatePrim(_pathNode, childName));
    }
}

SdfPath
SdfPath::AppendProperty(TfToken const &propName) const {
    if (!IsValidNamespacedIdentifier(propName.GetString())) {
        //TF_WARN("Invalid property name.");
        return EmptyPath();
    }
    if (!IsPrimVariantSelectionPath() && 
        !IsPrimPath() && 
                (_pathNode != Sdf_PathNode::GetRelativeRootNode())) {
        TF_WARN("Can only append a property '%s' to a prim path (%s)",
                propName.GetText(), GetText());
        return EmptyPath();
    }
    return SdfPath(Sdf_PathNode::FindOrCreatePrimProperty(_pathNode, propName));
}

SdfPath
SdfPath::AppendVariantSelection(const string &variantSet,
                               const string &variant) const
{
    if (!IsPrimOrPrimVariantSelectionPath() && 
        (_pathNode != Sdf_PathNode::GetRelativeRootNode())) {
        TF_CODING_ERROR("Cannot append variant selection %s = %s to <%s>; "
                        "can only append a variant selection to a prim or "
                        "prim variant selection path.",
                        variantSet.c_str(), variant.c_str(),
                        GetText());
        return EmptyPath();
    }
    return SdfPath(Sdf_PathNode::
                  FindOrCreatePrimVariantSelection(_pathNode,
                                                   TfToken(variantSet),
                                                   TfToken(variant)));
}

SdfPath
SdfPath::AppendTarget(const SdfPath &targetPath) const {
    if (!IsPropertyPath()) {
        TF_WARN("Can only append a target to a property path.");
        return EmptyPath();
    }
    if (targetPath == EmptyPath()) {
        TF_WARN("Target path cannot be invalid.");
        return EmptyPath();
    }
    return SdfPath(Sdf_PathNode::FindOrCreateTarget(_pathNode,
                                                  targetPath._pathNode));
}

SdfPath
SdfPath::AppendRelationalAttribute(TfToken const &attrName) const {
    if (!IsValidNamespacedIdentifier(attrName)) {
        TF_WARN("Invalid property name.");
        return EmptyPath();
    }
    if (!IsTargetPath()) {
        TF_WARN("Can only append a relational attribute to a target path.");
        return EmptyPath();
    }
    return SdfPath(Sdf_PathNode::
                  FindOrCreateRelationalAttribute(_pathNode, attrName));
}

SdfPath
SdfPath::AppendMapper(const SdfPath &targetPath) const {
    if (!IsPropertyPath()) {
        TF_WARN("Cannnot append mapper '%s' to non-property path <%s>.",
                targetPath.GetString().c_str(), GetString().c_str());
        return EmptyPath();
    }
    if (targetPath == EmptyPath()) {
        TF_WARN("Cannot append an empty mapper target path to <%s>",
                GetString().c_str());
        return EmptyPath();
    }
    return SdfPath(Sdf_PathNode::FindOrCreateMapper(_pathNode,
                                                  targetPath._pathNode));
}

SdfPath
SdfPath::AppendMapperArg(TfToken const &argName) const {
    if (!_IsValidIdentifier(argName)) {
        TF_WARN("Invalid arg name.");
        return EmptyPath();
    }
    if (!IsMapperPath()) {
        TF_WARN("Can only append a mapper arg to a mapper path.");
        return EmptyPath();
    }
    return SdfPath(Sdf_PathNode::FindOrCreateMapperArg(_pathNode, argName));
}

SdfPath
SdfPath::AppendExpression() const {
    if (!IsPropertyPath()) {
        TF_WARN("Can only append an expression to a property path.");
        return EmptyPath();
    }
    return SdfPath(Sdf_PathNode::FindOrCreateExpression(_pathNode));
}

SdfPath
SdfPath::AppendElementString(const std::string &element) const
{
    return AppendElementToken(TfToken(element));
}

SdfPath
SdfPath::AppendElementToken(const TfToken &elementTok) const
{
    std::string const &element = elementTok.GetString();

    if (ARCH_UNLIKELY((!_pathNode) || element.empty())){
        if (!_pathNode){
            TF_CODING_ERROR("Cannot append element \'%s\' to the EmptyPath.",
                            element.c_str());
        }
        else {
            TF_CODING_ERROR("Cannot append EmptyPath as a path element.");
        }
        return EmptyPath();
    }
    /* This is a somewhat unfortunate replication of a subset of the
     * logic contained in the full-path-parser.  We can't invoke the
     * parser on just a single element out of context (and probably
     * wouldn't want to for cost reasons if we could).  Can't think of
     * a more elegant way to do this.  1/13 */
    char const *txt = element.c_str();
    // No static tokens for variant chars...
    if (txt[0] == '{') {
        
        vector<string> tokens = TfStringTokenize(element, "{=}");
        TfToken variantSel;
        if (tokens.size() == 2){
            variantSel = TfToken(tokens[1]);
        } 
        else if (tokens.size() != 1){
            return EmptyPath();
        }
        return AppendVariantSelection(TfToken(tokens[0]),
                                      variantSel);
        
    } 
    else if (txt[0] == SdfPathTokens->relationshipTargetStart.GetString()[0]) {
        SdfPath  target(element.substr(1, element.length()-2));
        return AppendTarget(target);
    } 
    else if (txt[0] == SdfPathTokens->propertyDelimiter.GetString()[0]) {
        // This is the ambiguous one.  First check for the special symbols,
        // and if it looks like a "plain old property", consult parent type
        // to determine what the property sub-type should be.
        static string mapperStr = SdfPathTokens->propertyDelimiter.GetString() +
            SdfPathTokens->mapperIndicator.GetString() +
            SdfPathTokens->relationshipTargetStart.GetString();
        static string expressionStr =
            SdfPathTokens->propertyDelimiter.GetString()
            + SdfPathTokens->expressionIndicator.GetString();
        
        if ( element == expressionStr ){
            return IsPropertyPath() ? AppendExpression() : 
                AppendProperty(SdfPathTokens->expressionIndicator);
        } 
        else if (TfStringStartsWith(element, mapperStr)){
            const size_t prefixSz(mapperStr.length());
            SdfPath  target(element.substr(prefixSz, 
                                          element.length()-(prefixSz+1)));
            return AppendMapper(target);
        }
        else {
            TfToken  property(element.substr(1));
            
            if (IsMapperPath()){
                return AppendMapperArg(property);
            }
            else if (IsTargetPath()){
                return AppendRelationalAttribute(property);
            }
            else {
                return AppendProperty(property);
            }
        }
    } 
    else {
        return AppendChild(elementTok);
    }
        
}


SdfPath
SdfPath::ReplacePrefix(const SdfPath &oldPrefix, const SdfPath &newPrefix,
                      bool fixTargetPaths) const
{
    TRACE_FUNCTION();

    if (oldPrefix == newPrefix) {
        return *this;
    }
    if (oldPrefix.IsEmpty() || newPrefix.IsEmpty()) {
        return EmptyPath();
    }

    return _ReplacePrefix(oldPrefix, newPrefix, fixTargetPaths);
}

SdfPath
SdfPath::_ReplacePrefix(const SdfPath &oldPrefix, const SdfPath &newPrefix,
                       bool fixTargetPaths) const
{
    if (*this == oldPrefix) {
        // Base case: we've reached oldPrefix.
        return newPrefix;
    }

    if (GetPathElementCount() == 0) {
        // Empty paths have nothing to replace.
        return *this;
    }

    // If we've recursed above the oldPrefix, we can bail as long as there
    // are no target paths we need to fix.
    if (GetPathElementCount() <= oldPrefix.GetPathElementCount() &&
        (!fixTargetPaths || !_pathNode->ContainsTargetPath())) {
        // We'll never see oldPrefix beyond here, so return.
        return *this;
    }

    // Recursively translate the parent.
    SdfPath parent =
        GetParentPath()._ReplacePrefix(oldPrefix, newPrefix, fixTargetPaths);

    // Translation of the parent may fail; it will have emitted an error.
    // Return here so we don't deref an invalid _pathNode below.
    if (parent.IsEmpty())
        return SdfPath();

    // Append the tail component.  Use _AppendNode() except in these cases:
    // - For prims and properties, we construct child nodes directly
    //   so as to not expand out ".." components and to avoid the cost
    //   of unnecessarily re-validating identifiers.
    // - For embedded target paths, translate the target path.
    switch (_pathNode->GetNodeType()) {
    case Sdf_PathNode::PrimNode:
        return SdfPath(Sdf_PathNode::FindOrCreatePrim(parent._pathNode,
                                                    _pathNode->GetName()));
    case Sdf_PathNode::PrimPropertyNode:
        return SdfPath(Sdf_PathNode::FindOrCreatePrimProperty(
                                parent._pathNode, _pathNode->GetName()));
    case Sdf_PathNode::TargetNode:
        if (fixTargetPaths) {
            return parent.AppendTarget( _pathNode->GetTargetPath()
                ._ReplacePrefix(oldPrefix, newPrefix, fixTargetPaths));
        } else {
            return _AppendNode(parent, _pathNode);
        }
    case Sdf_PathNode::MapperNode:
        if (fixTargetPaths) {
            return parent.AppendMapper( _pathNode->GetTargetPath()
                ._ReplacePrefix(oldPrefix, newPrefix, fixTargetPaths));
        } else {
            return _AppendNode(parent, _pathNode);
        }
    default:
        return _AppendNode(parent, _pathNode);
    }
}

SdfPath
SdfPath::GetCommonPrefix(const SdfPath &path) const {

    if (path == SdfPath()) {
        TF_WARN("GetCommonPrefix(): invalid path.");
        return SdfPath();
    }

    SdfPath path1 = *this;
    SdfPath path2 = path;

    int count1 = path1.GetPathElementCount();
    int count2 = path2.GetPathElementCount();

    if (count1 > count2) {
        for (int i=0; i < (count1-count2); ++i) {
            path1 = path1.GetParentPath();
        }
    } else {
        for (int i=0; i < (count2-count1); ++i) {
            path2 = path2.GetParentPath();
        }
    }

    while (path1 != path2) {
        path1 = path1.GetParentPath();
        path2 = path2.GetParentPath();
    }

    return path1;
}

std::pair<SdfPath, SdfPath>
SdfPath::RemoveCommonSuffix(const SdfPath& otherPath, bool stopAtRootPrim) const {
    std::pair<Sdf_PathNodeConstRefPtr, Sdf_PathNodeConstRefPtr> result =
        Sdf_PathNode::RemoveCommonSuffix(_pathNode, otherPath._pathNode,
                                        stopAtRootPrim);
    return std::make_pair(SdfPath(result.first), SdfPath(result.second));
}

SdfPath
SdfPath::ReplaceTargetPath(const SdfPath &newTargetPath) const {

    if (!_pathNode) {
        return SdfPath();
    }

    if (newTargetPath == SdfPath()) {
        TF_WARN("ReplaceTargetPath(): invalid new target path.");
        return SdfPath();
    }

    Sdf_PathNode::NodeType type = _pathNode->GetNodeType();
    if (type == Sdf_PathNode::TargetNode) {
        return GetParentPath().AppendTarget(newTargetPath);
    } else if (type == Sdf_PathNode::RelationalAttributeNode) {
        return GetParentPath().ReplaceTargetPath(newTargetPath).
                    AppendRelationalAttribute(_pathNode->GetName());
    } else if (type == Sdf_PathNode::MapperNode) {
        return GetParentPath().AppendMapper(newTargetPath);
    } else if (type == Sdf_PathNode::MapperArgNode) {
        return GetParentPath().ReplaceTargetPath(newTargetPath).
                    AppendMapperArg(_pathNode->GetName());
    } else if (type == Sdf_PathNode::ExpressionNode) {
        return GetParentPath().ReplaceTargetPath(newTargetPath).
                    AppendExpression();
    }

    // no target to replace
    // return path unchanged
    return *this;
}

SdfPath
SdfPath::MakeAbsolutePath(const SdfPath & anchor) const {

    if (anchor == SdfPath()) {
        TF_WARN("MakeAbsolutePath(): anchor is the empty path.");
        return SdfPath();
    }

    // Check that anchor is an absolute path
    if (!anchor.IsAbsolutePath()) {
        TF_WARN("MakeAbsolutePath() requires an absolute path as an argument.");
        return SdfPath();
    }

    // Check that anchor is a component path
    if (!anchor.IsAbsoluteRootOrPrimPath() && 
        !anchor.IsPrimVariantSelectionPath()) {
        TF_WARN("MakeAbsolutePath() requires a prim path as an argument.");
        return SdfPath();
    }

    // If we're invalid, just return a copy of ourselves.
    if (IsEmpty())
        return *this;

    SdfPath result = *this;

    // If we're not already absolute, do our own path using anchor as the
    // relative base.
    if (!IsAbsolutePath()) {
        // This list winds up in reverse order to what one might at
        // first expect.
        vector<Sdf_PathNodeConstRefPtr> relNodes;

        Sdf_PathNodeConstRefPtr relRoot = Sdf_PathNode::GetRelativeRootNode();
        Sdf_PathNodeConstRefPtr curNode = _pathNode;
        // Walk up looking for oldPrefix node.
        while (curNode) {
            if (curNode == relRoot) {
                break;
            }
            relNodes.push_back(curNode);
            curNode = curNode->GetParentNode();
        }
        if (!curNode) {
            // Didn't find relative root
            // should never get here since all relative paths should have a
            // relative root node
            // CODE_COVERAGE_OFF
            TF_CODING_ERROR("Didn't find relative root");
            return SdfPath();
            // CODE_COVERAGE_ON
        }

        result = anchor;

        // Got the list, now add nodes similar to relNodes to anchor
        // relNodes needs to be iterated in reverse since the closest ancestor
        // node was pushed on last.
        vector<Sdf_PathNodeConstRefPtr>::reverse_iterator it = relNodes.rbegin();
        while (it != relNodes.rend()) {
            result = _AppendNode(result, *it);
            ++it;
        }
    }

    // Now make target path absolute (recursively) if we need to.
    // We need to use result's prim path as the anchor for the target path.
    SdfPath const &targetPath = result.GetTargetPath();
    if (!targetPath.IsEmpty()) {
        SdfPath primPath = result.GetPrimPath();
        SdfPath newTargetPath = targetPath.MakeAbsolutePath(primPath);
        result = result.ReplaceTargetPath(newTargetPath);
    }

    return result;
}

SdfPath
SdfPath::MakeRelativePath(const SdfPath & anchor) const
{
    TRACE_FUNCTION();

    // Check that anchor is a valid path
    if ( anchor == SdfPath() ) {
        TF_WARN("MakeRelativePath(): anchor is the invalid path.");
        return SdfPath();
    }

    // Check that anchor is an absolute path
    if (!anchor.IsAbsolutePath()) {
        TF_WARN("MakeRelativePath() requires an absolute path as an argument.");
        return SdfPath();
    }

    // Check that anchor is a component path
    if (!anchor.IsAbsoluteRootOrPrimPath() && !anchor.IsPrimVariantSelectionPath()) {
        TF_WARN("MakeRelativePath() requires a component path as an argument (got '%s').",
                 anchor.GetString().c_str());
        return SdfPath();
    }

    // If we're invalid, just return a copy of ourselves.
    if (!_pathNode) {
        return SdfPath();
    }

    if (!IsAbsolutePath()) {
        // Canonicalize... make sure the relative path has the
        // fewest possible dot-dots.
        SdfPath absPath = MakeAbsolutePath(anchor);

        return absPath.MakeRelativePath(anchor);
    }

    // We are absolute, we want to be relative

    // This list winds up in reverse order to what one might at first expect.
    vector<Sdf_PathNodeConstRefPtr> relNodes;

    // We need to crawl up the this path until we are the same length as
    // the anchor.
    // Then we crawl up both till we find the matching nodes.
    // As we crawl, we build the relNodes vector.
    size_t thisCount = _pathNode->GetElementCount();
    size_t anchorCount = anchor._pathNode->GetElementCount();

    // these pointers avoid construction/destruction
    // of ref pointers
    Sdf_PathNodeConstRefPtr curThisNode = _pathNode;
    Sdf_PathNodeConstRefPtr curAnchorNode = anchor._pathNode;

    // walk to the same depth
    size_t dotdotCount = 0;

    while (thisCount > anchorCount) {
        relNodes.push_back(curThisNode);
        curThisNode = curThisNode->GetParentNode();
        --thisCount;
    }

    while (thisCount < anchorCount) {
        ++dotdotCount;
        curAnchorNode = curAnchorNode->GetParentNode();
        --anchorCount;
    }

    // now we're at the same depth
    TF_AXIOM(thisCount == anchorCount);

    // walk to a common prefix
    while (curThisNode != curAnchorNode) {
        ++dotdotCount;
        relNodes.push_back(curThisNode);
        curThisNode   = curThisNode->GetParentNode();
        curAnchorNode = curAnchorNode->GetParentNode();
    }

    // Now relNodes the nodes of this path after the prefix
    // common to anchor and this path.
    SdfPath result = ReflexiveRelativePath();

    // Start by adding dotdots
    while (dotdotCount--) {
        result = result.GetParentPath();
    }

    // Now add nodes similar to relNodes to the ReflexiveRelativePath()
    // relNodes needs to be iterated in reverse since the closest ancestor
    // node was pushed on last.
    vector<Sdf_PathNodeConstRefPtr>::reverse_iterator it = relNodes.rbegin();
    while (it != relNodes.rend()) {
        result = _AppendNode(result, *it);
        ++it;
    }

    return result;
}

static inline bool _IsValidIdentifier(TfToken const &name)
{
    return TfIsValidIdentifier(name.GetString());
}

bool
SdfPath::IsValidIdentifier(const std::string &name)
{
    return TfIsValidIdentifier(name);
}

bool
SdfPath::IsValidNamespacedIdentifier(const std::string &name)
{
    // A valid C/Python identifier except we also allow the namespace delimiter
    // and if we tokenize on that delimiter then all tokens are valid C/Python
    // identifiers.  That means following a delimiter there must be an '_' or
    // alphabetic character.

    // This code currently assumes the namespace delimiter is one character.
    const char namespaceDelimiter =
        SdfPathTokens->namespaceDelimiter.GetText()[0];

    std::string::const_iterator first = name.begin();
    std::string::const_iterator last = name.end();

    // Not empty and first character is alpha or '_'.
    if (first == last || !(isalpha(*first) || (*first == '_')))
        return false;
    // Last character is not the namespace delimiter.
    if (*(last - 1) == namespaceDelimiter)
        return false;

    for (++first; first != last; ++first) {
        // Allow a namespace delimiter.
        if (*first == namespaceDelimiter) {
            // Skip delimiter.  We know we will not go beyond the end of
            // the string because we checked before the loop that the
            // last character was not the delimiter.
            ++first;

            // First character.
            if (!(isalpha(*first) || (*first == '_')))
                return false;
        }
        else {
            // Next character 
            if (!(isalnum(*first) || (*first == '_')))
                return false;
        }
    }

    return true;
}

std::vector<std::string>
SdfPath::TokenizeIdentifier(const std::string &name)
{
    std::vector<std::string> result;

    // This code currently assumes the namespace delimiter is one character.
    const char namespaceDelimiter =
        SdfPathTokens->namespaceDelimiter.GetText()[0];

    std::string::const_iterator first = name.begin();
    std::string::const_iterator last = name.end();

    // Not empty and first character is alpha or '_'.
    if (first == last || !(isalpha(*first) || (*first == '_')))
        return result;
    // Last character is not the namespace delimiter.
    if (*(last - 1) == namespaceDelimiter)
        return result;

    // Count delimiters and reserve space in result.
    result.reserve(1 + std::count(first, last, namespaceDelimiter));

    std::string::const_iterator anchor = first;
    for (++first; first != last; ++first) {
        // Allow a namespace delimiter.
        if (*first == namespaceDelimiter) {
            // Record token.
            result.push_back(std::string(anchor, first));

            // Skip delimiter.  We know we will not go beyond the end of
            // the string because we checked before the loop that the
            // last character was not the delimiter.
            anchor = ++first;

            // First character.
            if (!(isalpha(*first) || (*first == '_'))) {
                TfReset(result);
                return result;
            }
        }
        else {
            // Next character 
            if (!(isalnum(*first) || (*first == '_'))) {
                TfReset(result);
                return result;
            }
        }
    }

    // Record the last token.
    result.push_back(std::string(anchor, first));

    return result;
}

TfTokenVector
SdfPath::TokenizeIdentifierAsTokens(const std::string &name)
{
    std::vector<std::string> tmp = TokenizeIdentifier(name);
    TfTokenVector result(tmp.size());
    for (size_t i = 0, n = tmp.size(); i != n; ++i) {
        TfToken(tmp[i]).Swap(result[i]);
    }
    return result;
}

std::string
SdfPath::JoinIdentifier(const std::vector<std::string>& names)
{
    return TfStringJoin(names, SdfPathTokens->namespaceDelimiter.GetText());
}

std::string
SdfPath::JoinIdentifier(const TfTokenVector& names)
{
    std::vector<std::string> tmp(names.size());
    for (size_t i = 0, n = names.size(); i != n; ++i) {
        tmp[i] = names[i].GetString();
    }
    return TfStringJoin(tmp, SdfPathTokens->namespaceDelimiter.GetText());
}

std::string
SdfPath::JoinIdentifier(const std::string &lhs, const std::string &rhs)
{
    if (lhs.empty()) {
        return rhs;
    }
    else if (rhs.empty()) {
        return lhs;
    }
    else {
        return lhs + SdfPathTokens->namespaceDelimiter.GetText() + rhs;
    }
}

std::string
SdfPath::JoinIdentifier(const TfToken &lhs, const TfToken &rhs)
{
    return JoinIdentifier(lhs.GetString(), rhs.GetString());
}

std::string
SdfPath::StripNamespace(const std::string &name)
{
    // This code currently assumes the namespace delimiter is one character.
    const char namespaceDelimiter =
        SdfPathTokens->namespaceDelimiter.GetText()[0];
    const std::string::size_type n = name.rfind(namespaceDelimiter);
    return n == std::string::npos ? name : name.substr(n + 1);
}

TfToken
SdfPath::StripNamespace(const TfToken &name)
{
    return TfToken(StripNamespace(name.GetString()));
}

bool
SdfPath::IsValidPathString(const std::string &pathString,
                          std::string *errMsg)
{
    Sdf_PathParserContext context;

    // Initialize the scanner, allowing it to be reentrant.
    pathYylex_init(&context.scanner);

    yy_buffer_state *b =
        pathYy_scan_bytes(pathString.c_str(), pathString.size(), 
                          context.scanner);

    bool valid = (pathYyparse(&context) == 0);

    if (!valid && errMsg)
        *errMsg = context.errStr;

    // Clean up.
    pathYy_delete_buffer(b, context.scanner);
    pathYylex_destroy(context.scanner);

    return valid;
}

bool
SdfPath::IsBuiltInMarker(const std::string &marker)
{
    // XXX seems a little strange that this knowledge would live
    // in SdfPath, which knows nothing else about "markers"
    return (marker == ""         || 
            marker == "current"  || 
            marker == "authored" || 
            marker == "final"    || 
            marker == "initial");
}

bool
SdfPath::_LessThanInternal(Sdf_PathNodeConstRefPtr const &lhsRefPtr,
                          Sdf_PathNodeConstRefPtr const &rhsRefPtr)
{
    // Note that it's the caller's responsibility to make sure lhsRefPtr and
    // rhsRefPtr are not NULL, so we don't check here.

    // Use raw pointers in this function because it's rather performance
    // sensitive, and we don't want to pay for the null check on every deref.
    Sdf_PathNode const *lhs = boost::get_pointer(lhsRefPtr);
    Sdf_PathNode const *rhs = boost::get_pointer(rhsRefPtr);

    SdfPath const &absRoot = SdfPath::AbsoluteRootPath();

    if (lhs->IsAbsolutePath() != rhs->IsAbsolutePath()) {
        return lhs->IsAbsolutePath();
    } else if (lhsRefPtr == absRoot._pathNode) {
        return true;
    } else if (rhsRefPtr == absRoot._pathNode) {
        return false;
    }

    // Both absolute or both relative
    // We need to crawl up the longer path until both are the same length.
    // The we crawl up both till we find the nodes whose parents match.
    // Then we can compare those nodes.
    size_t thisCount = lhs->GetElementCount();
    size_t rhsCount = rhs->GetElementCount();

    Sdf_PathNode const *curThisNode = lhs;
    Sdf_PathNode const *curRhsNode = rhs;

    // walk up to the same depth
    size_t curCount = thisCount;

    while (curCount > rhsCount) {
        curThisNode = boost::get_pointer(curThisNode->GetParentNode());
        --curCount;
    }

    curCount = rhsCount;

    while (curCount > thisCount) {
        curRhsNode = boost::get_pointer(curRhsNode->GetParentNode());
        --curCount;
    }

    // Now the cur nodes are at the same depth in the node tree
    if (curThisNode == curRhsNode) {
        // They differ only in the tail.  If there's no tail, they are equal.
        // If rhs has the tail, then this is less, otherwise rhs is less.
        return thisCount < rhsCount;
    }

    // Crawl up both chains till we find an equal parent
    while ( curThisNode->GetParentNode() != curRhsNode->GetParentNode() ) {
        curThisNode = boost::get_pointer(curThisNode->GetParentNode());
        curRhsNode = boost::get_pointer(curRhsNode->GetParentNode());
    }

    // Now parents are equal, compare the current child nodes.
    return curThisNode->Compare<Sdf_PathNode::LessThan>(*curRhsNode);
}

std::ostream & operator<<( std::ostream &out, const SdfPath &path ) {
    return out << path.GetString();
}

SdfPathVector
SdfPath::GetConciseRelativePaths(const SdfPathVector& paths) {

    SdfPathVector primPaths;
    SdfPathVector anchors;
    SdfPathVector labels;

    // initialize the vectors
    TF_FOR_ALL(iter, paths) {

        if(!iter->IsAbsolutePath()) {
            TF_WARN("argument to GetConciseRelativePaths contains a relative path.");
            return paths;
        }

        // first, get the prim paths
        SdfPath primPath = iter->GetPrimPath();
        SdfPath anchor = primPath.GetParentPath();

        primPaths.push_back(primPath);
        anchors.push_back(anchor);

        // we have to special case root anchors, since MakeRelativePath can't handle them
        if(anchor == SdfPath::AbsoluteRootPath())
          labels.push_back(primPath);
        else
          labels.push_back(primPath.MakeRelativePath(anchor));

    }

    // each ambiguous path must be raised to its parent
    bool ambiguous;
    do {

        ambiguous = false;

        // the next iteration of labels
        SdfPathVector newAnchors;
        SdfPathVector newLabels;

        // find ambiguous labels
        for(size_t i=0;i<labels.size();++i) {

           int ok = true;

           // search for some other path that makes this one ambiguous
           for(size_t j=0;j<labels.size();++j) {
              if(i != j && labels[i] == labels[j] && primPaths[i] != primPaths[j]) {
                  ok = false;
                  break;
              }
           }

           if(!ok) {

               // walk the anchor up one node
               SdfPath newAnchor = anchors[i].GetParentPath();

               newAnchors.push_back(newAnchor);
               newLabels.push_back( newAnchor == SdfPath::AbsoluteRootPath() ? primPaths[i]
                                       : primPaths[i].MakeRelativePath( newAnchor ) );
               ambiguous = true;

           } else {
               newAnchors.push_back(anchors[i]);
               newLabels.push_back(labels[i]);
           }

        }

        anchors = newAnchors;
        labels = newLabels;

    } while(ambiguous);

    // generate the final set from the anchors
    SdfPathVector result;

    for(size_t i=0; i<anchors.size();++i) {

       if(anchors[i] == SdfPath::AbsoluteRootPath()) {
          result.push_back( paths[i] );
       } else {
          result.push_back( paths[i].MakeRelativePath( anchors[i] ));
       }

    }

    return result;

}

void
SdfPath::RemoveDescendentPaths(SdfPathVector *paths)
{
    // To remove descendents, first partition paths into prefix-related groups
    // via sort.
    std::sort(paths->begin(), paths->end());

    // Now unique and erase all descendents.  The equivalence predicate returns
    // true if rhs has lhs as a prefix.
    paths->erase(std::unique(paths->begin(), paths->end(),
                             boost::bind(&SdfPath::HasPrefix, _2, _1)),
                 paths->end());
}

void
SdfPath::RemoveAncestorPaths(SdfPathVector *paths)
{
    // To remove descendents, first parition paths into prefix-related groups
    // via sort.
    std::sort(paths->begin(), paths->end());

    // Now unique and erase ancestors.  The equivalence predicate returns true
    // if lhs has rhs as a prefix.
    paths->erase(paths->begin(),
                 std::unique(paths->rbegin(), paths->rend(),
                             boost::bind(&SdfPath::HasPrefix, _1, _2)).base());
}

// Overload hash_value for SdfPath.
size_t hash_value(SdfPath const &path) {
    return path.GetHash();
}
