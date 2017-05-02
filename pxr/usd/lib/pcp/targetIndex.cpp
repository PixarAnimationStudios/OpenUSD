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

#include "pxr/pxr.h"
#include "pxr/usd/pcp/targetIndex.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/node_Iterator.h"
#include "pxr/usd/pcp/pathTranslation.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/propertyIndex.h"

#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/tracelite/trace.h"

#include <boost/optional.hpp>

PXR_NAMESPACE_OPEN_SCOPE

// Helper structure for deferring the computation of a prim index
// until needed, then caching the result to avoid multiple lookups
struct Pcp_TargetIndexContext 
{
    Pcp_TargetIndexContext(
        PcpCache* cache, 
        PcpErrorVector* allErrors,
        const SdfPath& targetObjectPath)
        : _cache(cache)
        , _allErrors(allErrors)
        , _targetObjectPath(targetObjectPath)
        , _index(NULL) { }

    PcpCache* GetCache()
    {
        return _cache;
    }

    const PcpPrimIndex& GetTargetObjectPrimIndex()
    {
        if (!_index) {
            _index = &_cache->ComputePrimIndex(
                _targetObjectPath.GetPrimPath(), _allErrors);
        }
        return *_index;
    }

private:
    PcpCache* _cache;
    PcpErrorVector* _allErrors;
    SdfPath _targetObjectPath;
    const PcpPrimIndex* _index;
};

// Helper to determine if the connection path was authored in
// a class but points to an instance of the class.
static bool
_TargetInClassAndTargetsInstance(
    const SdfPath& connectionPathInNodeNS,
    const PcpNodeRef& nodeWhereConnectionWasAuthored,
    Pcp_TargetIndexContext& context)
{
    // Connections authored in an inherited class may not target
    // an object in an instance of that class, as doing so would
    // break reverse path translation.
    if (!PcpIsInheritArc(nodeWhereConnectionWasAuthored.GetArcType())) {
        return false;
    }

    TRACE_FUNCTION();

    // If the connection path targets a namespace descendent of the class,
    // we know we're not pointing at an object in an instance of the 
    // class.
    //
    // Otherwise, we compute the prim index for the target object and
    // check if it (or an ancestor) inherits from the class where 
    // the connection was authored. If so, issue an error.
    const bool connectionPathInsideInheritedClass = 
        connectionPathInNodeNS.HasPrefix(
            nodeWhereConnectionWasAuthored.GetPathAtIntroduction());

    if (!connectionPathInsideInheritedClass) {
        const PcpPrimIndex& targetPrimIndex = context.GetTargetObjectPrimIndex();
        const PcpLayerStackPtr& layerStackWhereConnectionWasAuthored = 
            nodeWhereConnectionWasAuthored.GetLayerStack();
        const SdfPath inheritedClassPath =
            nodeWhereConnectionWasAuthored.GetPathAtIntroduction();

        for (const PcpNodeRef &n: targetPrimIndex.GetNodeRange()) {
            if (PcpIsInheritArc(n.GetArcType())
                && (n.GetLayerStack() == layerStackWhereConnectionWasAuthored)
                && (n.GetPath().HasPrefix(inheritedClassPath))) {
                return true;
            }
        }
    }

    return false;
}

namespace {

enum Pcp_PathTranslationError
{
    NoError = 0,
    PermissionDenied,
    InvalidTarget
};

}

static Pcp_PathTranslationError
_CheckTargetPermittedBeneathNode(
    const SdfPath& connectionPathInRootNS, const PcpNodeRef& node)
{
    const bool targetObjectIsProperty = connectionPathInRootNS.IsPropertyPath();

    TF_FOR_ALL(it, Pcp_GetChildrenRange(node)) {
        const PcpNodeRef& child = *it;

        // If the prim has been marked private at this node, the
        // target is pointing at a restricted object, which is invalid.
        if (child.IsRestricted() ||
            child.GetPermission() == SdfPermissionPrivate) {
            return PermissionDenied;
        }

        // Translate the path from the root namespace to this child's 
        // namespace. If the path translation fails, this target is
        // invalid so we return a general 'invalid target' error.
        //
        // At this point, though, path translation should only fail if
        // the connection is pointing at the pre-relocated source of
        // an object that has been relocated. We could verify this is
        // the case by searching the relocates statements in this node's
        // layer stack for this object, but that is potentially expensive.
        // So for now, we just let this remain a general error.
        const SdfPath pathInChildNS = 
            PcpTranslatePathFromRootToNode(child, connectionPathInRootNS);
        if (pathInChildNS.IsEmpty()) {
            return InvalidTarget;
        }

        if (targetObjectIsProperty) {
            TF_FOR_ALL(layerIt, child.GetLayerStack()->GetLayers()) {
                // Check all property specs up to the owning prim to see if any
                // are marked private. This handles the case where the property
                // is a relational attribute; in this case, we'd need to check
                // not only the attribute, but its owning relationship.
                for (SdfPath p = pathInChildNS; 
                     !p.IsPrimPath(); p = p.GetParentPath()) {

                    if (p.IsPropertyPath()) {
                        SdfPropertySpecHandle propSpec =
                            (*layerIt)->GetPropertyAtPath(p);
                        if (propSpec &&
                            propSpec->GetPermission() == SdfPermissionPrivate) {
                            return PermissionDenied;
                        }
                    }
                }
            }
        }

        const Pcp_PathTranslationError errorUnderChild = 
            _CheckTargetPermittedBeneathNode(connectionPathInRootNS, child);
        if (errorUnderChild != NoError) {
            return errorUnderChild;
        }
    }
    
    return NoError;
}

// Helper function to determine if the object indicated by the given paths
// can be targeted by an attribute connection or relationship. There are two
// primary things we verify here:
//
// - Permissions
// A connection is invalid if the object it targets is marked private
// in a weaker site than where the connection was authored. For instance:
//
//        ref     ref
//     /A ---> /B ---> /C
//
// If we have a connection authored in /B, it's OK if the targeted object
// was marked private in /A or /B, but not if it was marked private in /C. 
// See ErrorTargetPermissionDenied for more examples.
//
// - Relocates
// A connection is invalid if the object it targets was relocated, and the
// connection is pointing to the object's path prior to relocation. This
// is verified indirectly -- see comment in _CheckTargetPermittedBenaethNode.
// See ErrorInvalidPreRelocateTargetPath for examples.
//
static Pcp_PathTranslationError
_TargetIsPermitted(
    const SdfPath& connectionPathInRootNS,
    const SdfPath& connectionPathInNodeNS,
    const PcpNodeRef& nodeWhereConnectionWasAuthored,
    Pcp_TargetIndexContext& context)
{
    TRACE_FUNCTION();

    // The approach for figuring out where the given connection is valid
    // is to compute the prim index for the target object in the root layer 
    // stack (i.e., the layer stack for the given cache), find the node
    // that corresponds to where the connection was authored, then check 
    // the subtree beneath that node.
    //
    // An alternative approach would be to compute the index for the target
    // object in the layer stack where the connection was authored and to 
    // use that when checking permissions. This would avoid the need to search
    // for a specific node. However, we wouldn't be able to use the given cache
    // and would have to compute the index from scratch each time, which is
    // too expensive.

    const SdfPath owningPrimInRootNS = connectionPathInRootNS.GetPrimPath();
    const PcpPrimIndex& owningPrimIndex = context.GetTargetObjectPrimIndex();

    // Search for the node for the owning prim where the connection was
    // authored. 
    const SdfPath owningPrimInNodeNS = connectionPathInNodeNS.GetPrimPath();
    const PcpLayerStackSite owningPrimSiteWhereConnectionWasAuthored(
        nodeWhereConnectionWasAuthored.GetLayerStack(),
        owningPrimInNodeNS);

    PcpNodeRef owningPrimNodeWhereConnectionWasAuthored;
    for (const PcpNodeRef &node: owningPrimIndex.GetNodeRange()) {
        if (node.GetSite() == owningPrimSiteWhereConnectionWasAuthored) {
            owningPrimNodeWhereConnectionWasAuthored = node;
            break;
        }
    }

    // It's possible that we won't find the node we're looking for because
    // it was culled out of the graph. This can happen in a few cases
    // (I think this is a complete list):
    //
    //  1. The target object doesn't exist, e.g., a bad path was authored.
    //  2. The target object does exist in the composed scene, but is brought
    //     in via a completely separate arc.
    //  3. The target object is in a payload, but the connection is authored
    //     outside the payload.
    //
    // See /CulledPermissions_{1, 2, 3} in ErrorPermissionDenied for examples
    // of each case.
    //
    // In all cases, we permit the connection. This ignores permissions that
    // usually are inherited down namespace but maintains legacy behavior 
    // from Csd. If we wanted to make the behavior consistent here, we would
    // have to walk up namespace to see if there were other permissions to
    // apply.
    //
    // If culling is disabled, we definitely expect to find the node, so
    // issue an error if we don't.
    if (!owningPrimNodeWhereConnectionWasAuthored) {
        TF_VERIFY(context.GetCache()->GetPrimIndexInputs().cull, 
            "Could not find expected node for site %s in prim index for <%s>",
            TfStringify(owningPrimSiteWhereConnectionWasAuthored).c_str(),
            owningPrimInRootNS.GetText());

        return NoError;
    }

    return _CheckTargetPermittedBeneathNode(
            connectionPathInRootNS, owningPrimNodeWhereConnectionWasAuthored);
}

static void
_RemoveTargetPathErrorsForPath(
    const SdfPath& composedTargetPath,
    PcpErrorVector* targetPathErrors)
{
    if (targetPathErrors->empty()) {
        return;
    }

    PcpErrorVector::iterator 
        it = targetPathErrors->begin(), end = targetPathErrors->end();
    while (it != end) {
        if (const PcpErrorTargetPathBase* targetPathError = 
            dynamic_cast<const PcpErrorTargetPathBase*>(it->get())) {
            if (targetPathError->composedTargetPath == composedTargetPath) {
                it = targetPathErrors->erase(it);
                end = targetPathErrors->end();
                continue;
            }
        }
        ++it;
    }
}

// Callback used to translate paths as path list operations from
// various nodes are applied.
static boost::optional<SdfPath>
_PathTranslateCallback(
    SdfListOpType opType,
    const PcpSite &propSite,
    const PcpNodeRef& node, 
    const SdfPath& inPath,
    const SdfPropertySpecHandle& owningProp,
    const SdfSpecType relOrAttrType,
    PcpCache* cacheForValidation, 
    PcpErrorVector* targetPathErrors,
    PcpErrorVector* otherErrors)
{
    bool pathIsMappable = false;

    const SdfPath translatedPath = 
        PcpTranslatePathFromNodeToRoot(node, inPath, &pathIsMappable);

    // If the given path is part of a delete list operation, we don't
    // need to perform any of the validation below. Since the specified
    // path is being deleted from the composed result, we want to delete
    // any errors associated with that path from our list of errors.
    //
    // This is similar to handling for explicit list operations
    // in PcpBuildFilteredTargetIndex.
    if (opType == SdfListOpTypeDeleted) {
        if (pathIsMappable && !translatedPath.IsEmpty()) {
            _RemoveTargetPathErrorsForPath(translatedPath, targetPathErrors);
            return translatedPath;
        }
        return boost::optional<SdfPath>();
    }
    
    if (!pathIsMappable) {
        PcpErrorInvalidExternalTargetPathPtr err =
            PcpErrorInvalidExternalTargetPath::New();
        err->rootSite = propSite;
        err->targetPath = inPath;
        err->owningPath = owningProp->GetPath();
        err->ownerSpecType = relOrAttrType;
        err->ownerArcType = node.GetArcType();
        err->ownerIntroPath = node.GetIntroPath();
        err->layer = owningProp->GetLayer();
        err->composedTargetPath = SdfPath();
        targetPathErrors->push_back(err);
        return boost::optional<SdfPath>();
    }

    if (translatedPath.IsEmpty()) {
        return boost::optional<SdfPath>();
    }

    if (cacheForValidation) {
        Pcp_TargetIndexContext context(
            cacheForValidation, otherErrors, translatedPath);

        // Check if this target has been authored in a class but targets
        // an instance of the class.
        if (_TargetInClassAndTargetsInstance(inPath, node, context)) {
            PcpErrorInvalidInstanceTargetPathPtr err = 
                PcpErrorInvalidInstanceTargetPath::New();
            err->rootSite = propSite;
            err->targetPath = inPath;
            err->owningPath = owningProp->GetPath();
            err->ownerSpecType = relOrAttrType;
            err->layer = owningProp->GetLayer();
            err->composedTargetPath = translatedPath;
            targetPathErrors->push_back(err);
            return boost::optional<SdfPath>();
        }

        // Check if the connection is invalid due to permissions or
        // relocates. We do not do this check for Usd caches, since Usd 
        // does not use either feature.
        if (!cacheForValidation->IsUsd()) {

            switch (_TargetIsPermitted(translatedPath, inPath, node, context)) {
            case PermissionDenied:
            {
                PcpErrorTargetPermissionDeniedPtr err =
                    PcpErrorTargetPermissionDenied::New();
                err->rootSite = propSite;
                err->targetPath = inPath;
                err->owningPath = owningProp->GetPath();
                err->ownerSpecType = relOrAttrType;
                err->layer = owningProp->GetLayer();
                err->composedTargetPath = translatedPath;
                targetPathErrors->push_back(err);
                return boost::optional<SdfPath>();
            }

            case InvalidTarget:
            {
                PcpErrorInvalidTargetPathPtr err = 
                    PcpErrorInvalidTargetPath::New();
                err->rootSite = propSite;
                err->targetPath = inPath;
                err->owningPath = owningProp->GetPath();
                err->ownerSpecType = relOrAttrType;
                err->layer = owningProp->GetLayer();
                err->composedTargetPath = translatedPath;
                targetPathErrors->push_back(err);
                return boost::optional<SdfPath>();
            }

            case NoError:
                // Do nothing.
                break;
            }
        }
    }
    
    return translatedPath;
}
 
void
PcpBuildFilteredTargetIndex(
    const PcpSite& propSite,
    const PcpPropertyIndex& propertyIndex,
    const SdfSpecType relOrAttrType,
    const bool localOnly,
    const SdfSpecHandle &stopProperty,
    const bool includeStopProperty,
    PcpCache *cacheForValidation,
    PcpTargetIndex *targetIndex,
    PcpErrorVector *allErrors)
{
    TRACE_FUNCTION();

    if (!(relOrAttrType == SdfSpecTypeRelationship ||
             relOrAttrType == SdfSpecTypeAttribute)) {
        TF_CODING_ERROR("relOrAttrType msut be either SdfSpecTypeRelationship"
                        " or SdfSpecTypeAttribute");
        return;
    }

    if (propertyIndex.IsEmpty()) {
        return;
    }

    const PcpPropertyRange propertyRange =
        propertyIndex.GetPropertyRange(localOnly);

    // Verify that the type of object at propSite.path matches what
    // we expect. We only need to check the first spec in the stack since all
    // other specs should have the same type. This is enforced in the
    // population of the property index.
    if (!TF_VERIFY(
            (*propertyRange.first)->GetSpecType() == relOrAttrType,
            "<%s> is not %s", 
            propSite.path.GetText(),
            relOrAttrType == SdfSpecTypeAttribute ? 
                "an attribute" : "a relationship")) {
        return;
    }

    const TfToken& fieldName = 
        relOrAttrType == SdfSpecTypeAttribute ? 
            SdfFieldKeys->ConnectionPaths : SdfFieldKeys->TargetPaths;

    SdfPathVector paths;
    PcpErrorVector targetPathErrors;

    // Walk the property stack from weakest to strongest, applying path list 
    // operations with the appropriate path translations to targetPaths.
    TF_REVERSE_FOR_ALL(propIt, propertyRange) {
        const SdfPropertySpecHandle& property = *propIt;
        if (!includeStopProperty && property == stopProperty) {
            break;
        }
        const VtValue& pathValue = property->GetField(fieldName);
        if (pathValue.IsEmpty() ||
            !TF_VERIFY(pathValue.IsHolding<SdfPathListOp>())) {
            continue;
        }
        const SdfPathListOp& pathListOps =
            pathValue.UncheckedGet<SdfPathListOp>();
        if (pathListOps.HasKeys()) {

            // If this list op is explicit, its contents will overwrite
            // everything we've composed up to this point. Because of this,
            // we can clear all of the target path errors we've accumulated
            // since the errorneous paths are being overridden.
            if (pathListOps.IsExplicit()) {
                targetPathErrors.clear();
            }

            SdfPathListOp::ApplyCallback pathTranslationCallback = 
                boost::bind(&_PathTranslateCallback, 
                            _1, boost::ref(propSite),
                            propIt.base().GetNode(), _2,
                            boost::ref(property), relOrAttrType,
                            cacheForValidation, 
                            &targetPathErrors, allErrors);
            pathListOps.ApplyOperations(&paths,
                                        pathTranslationCallback);
        }
        if (property == stopProperty) {
            break;
        }
    }

    allErrors->insert(
        allErrors->end(), targetPathErrors.begin(), targetPathErrors.end());

    targetIndex->paths.swap(paths);
    targetIndex->localErrors.swap(targetPathErrors);
}

void
PcpBuildTargetIndex(
    const PcpSite& propSite,
    const PcpPropertyIndex& propIndex,
    const SdfSpecType relOrAttrType,
    PcpTargetIndex *targetIndex,
    PcpErrorVector *allErrors)
{
    PcpBuildFilteredTargetIndex(
        propSite, propIndex, relOrAttrType,
        /* localOnly = */ false,
        /* stopProperty = */ SdfSpecHandle(),
        /* includeStopProperty = */ false,
        /* cacheForValidation = */ 0,
        targetIndex, allErrors );
}

PXR_NAMESPACE_CLOSE_SCOPE
