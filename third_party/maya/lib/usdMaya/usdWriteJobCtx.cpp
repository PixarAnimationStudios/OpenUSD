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
#include "usdMaya/usdWriteJobCtx.h"

#include "usdMaya/MayaTransformWriter.h"
#include "usdMaya/instancedNodeWriter.h"
#include "usdMaya/stageCache.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/scope.h"

#include <maya/MDagPathArray.h>
#include <maya/MItDag.h>
#include <maya/MString.h>
#include <maya/MPxNode.h>

#include <sstream>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (Shape)
);

namespace {
    inline SdfPath
    _GetRootOverridePath(
        const PxrUsdMayaJobExportArgs& args,
        const SdfPath& path)
    {
        if (!args.usdModelRootOverridePath.IsEmpty() && !path.IsEmpty()) {
            return path.ReplacePrefix(
                    path.GetPrefixes()[0],
                    args.usdModelRootOverridePath);
        }
        return path;
    }

    const SdfPath INSTANCES_SCOPE_PATH("/InstanceSources");
}

usdWriteJobCtx::usdWriteJobCtx(const PxrUsdMayaJobExportArgs& args)
        : mArgs(args)
{
}

bool
usdWriteJobCtx::IsMergedTransform(const MDagPath& path) const
{
    if (!mArgs.mergeTransformAndShape) {
        return false;
    }

    // Only transforms are mergeable.
    if (!path.hasFn(MFn::kTransform)) {
        return false;
    }

    // If we're instancing, and the transform is instanced, then we want it
    // to stay a plain old Xform at the root of the master. Thus, we disallow
    // merging as a special case.
    // (See also: in _FindOrCreateInstanceMaster, we insert a dummy "Shape" prim
    // before any bare gprims, which we can avoid for transforms by not merging
    // here.)
    if (mArgs.exportInstances) {
        MFnDagNode dagNode(path);
        if (dagNode.isInstanced(/*indirect*/ false)) {
            return false;
        }
    }

    // Any transform with multiple (non-intermediate) shapes below is
    // non-mergeable.
    unsigned int numberOfShapesDirectlyBelow = 0u;
    path.numberOfShapesDirectlyBelow(numberOfShapesDirectlyBelow);
    if (numberOfShapesDirectlyBelow != 1) {
        return false;
    }

    // If the node has more than one exportable child, then it is
    // non-mergeable. (I.e., we still want to collapse if it has two shapes
    // below, but one of them is an intermediate object.)
    // For efficiency reasons, since (# exportable children <= # children),
    // check the total child count first before checking whether they're
    // exportable.
    const unsigned int childCount = path.childCount();
    if (childCount != 1) {
        MDagPath childDag(path);
        unsigned int numExportableChildren = 0u;
        for (unsigned int i = 0u; i < childCount; ++i) {
            childDag.push(path.child(i));
            if (needToTraverse(childDag)) {
                ++numExportableChildren;
                if (numExportableChildren > 1) {
                    return false;
                }
            }
            childDag.pop();
        }
    }

    return true;
}

SdfPath
usdWriteJobCtx::ConvertDagToUsdPath(const MDagPath& dagPath) const
{
    SdfPath path = PxrUsdMayaUtil::MDagPathToUsdPath(
            dagPath, false, mArgs.stripNamespaces);

    // If we're merging transforms and shapes and this is a shape node, then
    // write to the parent (transform) path instead.
    MDagPath parentDag(dagPath);
    parentDag.pop();
    if (IsMergedTransform(parentDag)) {
        path = path.GetParentPath();
    }

    if (!mParentScopePath.IsEmpty()) {
        // Since path is from MDagPathToUsdPath, it will always be
        // an absolute path...
        path = path.ReplacePrefix(
                SdfPath::AbsoluteRootPath(),
                mParentScopePath);
    }

    return _GetRootOverridePath(mArgs, path);
}

usdWriteJobCtx::_ExportAndRefPaths
usdWriteJobCtx::_GetInstanceMasterPaths(const MDagPath& instancePath) const
{
    TF_AXIOM(mInstancesPrim);

    std::string fullName;
    if (mArgs.stripNamespaces){
        fullName = PxrUsdMayaUtil::stripNamespaces(
                instancePath.fullPathName()).asChar();
    }
    else {
        fullName = instancePath.fullPathName().asChar();
    }

    // Trim leading pipe; it's superfluous because all Maya full paths have it.
    fullName = fullName.substr(1);
    // Avoid name conflicts with other special chars (like |:/), since
    // TfMakeValidIdentifier replaces them with underscores also.
    fullName = TfStringReplace(fullName, "_", "__");
    // This should make a valid path component.
    fullName = TfMakeValidIdentifier(fullName);

    const SdfPath path = _GetRootOverridePath(
            mArgs, mInstancesPrim.GetPath().AppendChild(TfToken(fullName)));

    // In Maya, you can directly instance gprims or transforms, but
    // UsdImaging really wants you to instance at the transform level.
    // So if this is a directly-instanced gprim, we export it one level down
    // by creating a dummy scope.
    // (See also: in IsMergedTransform, we avoid merging directly-instanced
    // transforms in order to avoid having to add the dummy scope below.)
    SdfPath exportPath;
    if (instancePath.hasFn(MFn::kTransform)) {
        // Can directly instance transforms.
        return std::make_pair(path, path);
    }
    else {
        // Cannot directly instance gprims, so this must be exported underneath
        // a fake scope.
        return std::make_pair(path.AppendChild(_tokens->Shape), path);
    }
}

usdWriteJobCtx::_ExportAndRefPaths
usdWriteJobCtx::_FindOrCreateInstanceMaster(const MDagPath& instancePath)
{
    const MObjectHandle handle(instancePath.node());
    const auto it = _objectsToMasterPaths.find(handle);
    if (it != _objectsToMasterPaths.end()) {
        return it->second;
    }
    else {
        MDagPathArray allInstances;
        if (!MDagPath::getAllPathsTo(instancePath.node(), allInstances) ||
                (allInstances.length() == 0)) {
            TF_RUNTIME_ERROR("Could not find any instances for '%s'",
                    instancePath.fullPathName().asChar());
            _objectsToMasterPaths[handle] = _ExportAndRefPaths();
            return _ExportAndRefPaths();
        }

        // We use the DAG path of the first instance to construct the name of
        // the master.
        const _ExportAndRefPaths masterPaths =
                _GetInstanceMasterPaths(allInstances[0]);
        const SdfPath& exportPath = masterPaths.first;
        const SdfPath& referencePath = masterPaths.second;

        // Export the master's hierarchy.
        // Force un-instancing when exporting to avoid an infinite loop (we've
        // got to actually export the prims un-instanced somewhere at least
        // once).
        std::vector<MayaPrimWriterSharedPtr> primWriters;
        CreatePrimWriterHierarchy(
                allInstances[0],
                exportPath,
                /*forceUninstance*/ true,
                /*exportRootVisibility*/ true,
                &primWriters);

        if (!primWriters.size()) {
            _objectsToMasterPaths[handle] = _ExportAndRefPaths();
            return _ExportAndRefPaths();
        }

        for (MayaPrimWriterSharedPtr& primWriter : primWriters) {
            primWriter->Write(UsdTimeCode::Default());
        }

        // For proper instancing, ensure that none of the prims from
        // referencePath down to exportPath have empty type names by converting
        // prims to Xforms if necessary.
        for (UsdPrim prim = mStage->GetPrimAtPath(exportPath);
                prim && prim.GetPath().HasPrefix(referencePath);
                prim = prim.GetParent()) {
            if (prim.GetTypeName().IsEmpty()) {
                UsdGeomXform::Define(mStage, prim.GetPath());
            }
        }

        _objectsToMasterPaths[handle] = masterPaths;
        _objectsToMasterWriters[handle] = std::make_pair(
                mMayaPrimWriterList.size(),
                mMayaPrimWriterList.size() + primWriters.size());
        mMayaPrimWriterList.insert(
                mMayaPrimWriterList.end(),
                primWriters.begin(),
                primWriters.end());

        return masterPaths;
    }
}

bool
usdWriteJobCtx::_GetInstanceMasterPrimWriters(
    const MDagPath& instancePath,
    std::vector<MayaPrimWriterSharedPtr>::const_iterator* begin,
    std::vector<MayaPrimWriterSharedPtr>::const_iterator* end) const
{
    const MObjectHandle handle(instancePath.node());
    const auto it = _objectsToMasterWriters.find(handle);
    if (it != _objectsToMasterWriters.end()) {
        std::pair<size_t, size_t> range = it->second;
        if (range.first < mMayaPrimWriterList.size() &&
                range.second <= mMayaPrimWriterList.size()) {
            *begin = mMayaPrimWriterList.cbegin() + range.first;
            *end = mMayaPrimWriterList.cbegin() + range.second;
            return true;
        }
    }

    return false;
}

bool usdWriteJobCtx::needToTraverse(const MDagPath& curDag) const
{
    MObject ob = curDag.node();
    // NOTE: Already skipping all intermediate objects
    // skip all intermediate nodes (and their children)
    if (PxrUsdMayaUtil::isIntermediate(ob)) {
        return false;
    }

    // skip nodes that aren't renderable (and their children)

    if (mArgs.excludeInvisible && !PxrUsdMayaUtil::isRenderable(ob)) {
        return false;
    }

    if (!mArgs.exportDefaultCameras && ob.hasFn(MFn::kTransform) && curDag.length() == 1) {
        // Ignore transforms of default cameras
        MString fullPathName = curDag.fullPathName();
        if (fullPathName == "|persp" ||
            fullPathName == "|top" ||
            fullPathName == "|front" ||
            fullPathName == "|side") {
            return false;
        }
    }

    if (mArgs.GetFilteredTypeIds().size() > 0) {
        MFnDependencyNode mfnNode(ob);
        if (mArgs.GetFilteredTypeIds().find(mfnNode.typeId().id())
                != mArgs.GetFilteredTypeIds().end()) {
            return false;
        }
    }

    return true;
}

bool usdWriteJobCtx::openFile(const std::string& filename, bool append)
{
    ArResolverContext resolverCtx = ArGetResolver().GetCurrentContext();
    if (append) {
        mStage = UsdStage::Open(SdfLayer::FindOrOpen(filename), resolverCtx);
        if (!mStage) {
            TF_RUNTIME_ERROR(
                    "Failed to open stage file '%s'", filename.c_str());
            return false;
        }
    } else {
        // If we're exporting over a file that was previously imported, there
        // may still be stages in the stage cache that have that file as a root
        // layer. Creating a new stage with that file will fail because the
        // layer already exists in the layer registry, so we try to clear the
        // layer from the registry by erasing any stages in the stage cache
        // with that root layer.
        UsdMayaStageCache::EraseAllStagesWithRootLayerPath(filename);

        mStage = UsdStage::CreateNew(filename, resolverCtx);
        if (!mStage) {
            TF_RUNTIME_ERROR(
                    "Failed to create stage file '%s'", filename.c_str());
            return false;
        }
    }

    if (!mArgs.parentScope.IsEmpty()) {
        mParentScopePath = mArgs.parentScope;
        // Note that we only need to create the parentScope prim if we're not
        // using a usdModelRootOverridePath - if we ARE using
        // usdModelRootOverridePath, then IT will take the name of our parent
        // scope, and will be created when we writ out the model variants
        if (mArgs.usdModelRootOverridePath.IsEmpty()) {
            mParentScopePath = UsdGeomScope::Define(mStage,
                                                    mParentScopePath).GetPrim().GetPrimPath();
        }
    }

    if (mArgs.exportInstances) {
        mInstancesPrim = mStage->OverridePrim(INSTANCES_SCOPE_PATH);
    }

    return true;
}

void usdWriteJobCtx::processInstances()
{
    if (mArgs.exportInstances) {
        if (_objectsToMasterWriters.empty()) {
            mStage->RemovePrim(mInstancesPrim.GetPrimPath());
        } else {
            mInstancesPrim.SetSpecifier(SdfSpecifierOver);
        }
    }
}

MayaPrimWriterSharedPtr usdWriteJobCtx::CreatePrimWriter(
    const MDagPath& curDag,
    const SdfPath& usdPath,
    const bool forceUninstance)
{
    if (curDag.length() == 0) {
        // This is the world root node. It can't have a prim writer.
        return nullptr;
    }

    MObject ob = curDag.node();
    const SdfPath writePath = usdPath.IsEmpty()
            ? ConvertDagToUsdPath(curDag)
            : usdPath;

    MFnDagNode dagNode(curDag);
    const bool instanced = dagNode.isInstanced(/*indirect*/ false);

    if (mArgs.exportInstances && instanced && !forceUninstance) {
        // Deal with instances -- we use a special internal writer for them.
        return std::make_shared<PxrUsdMaya_InstancedNodeWriter>(
                curDag, writePath, *this);
    }
    else {
        // Deal with non-instances. Try to look up a writer plugin.
        // We search through the node's type ancestors, working backwards until
        // we find a prim writer plugin.
        MFnDependencyNode depNodeFn(ob);
        std::string mayaTypeName(depNodeFn.typeName().asChar());
        if (PxrUsdMayaPrimWriterRegistry::WriterFactoryFn primWriterFactory =
                _FindWriter(mayaTypeName)) {
            if (MayaPrimWriterSharedPtr primPtr = primWriterFactory(
                    curDag, writePath, *this)) {
                // We found a registered user prim writer that handles this node
                // type, so return now.
                return primPtr;
            }
        }
    }

    // Could not create a writer for this node.
    return nullptr;
}

PxrUsdMayaPrimWriterRegistry::WriterFactoryFn
usdWriteJobCtx::_FindWriter(const std::string& mayaNodeType)
{
    // Check if type is already cached locally.
    auto iter = mWriterFactoryCache.find(mayaNodeType);
    if (iter != mWriterFactoryCache.end()) {
        return iter->second;
    }

    // Search up the ancestor hierarchy for a writer plugin.
    const std::vector<std::string> ancestorTypes =
            PxrUsdMayaUtil::GetAllAncestorMayaNodeTypes(mayaNodeType);
    for (auto i = ancestorTypes.rbegin(); i != ancestorTypes.rend(); ++i) {
        if (PxrUsdMayaPrimWriterRegistry::WriterFactoryFn primWriterFactory =
                PxrUsdMayaPrimWriterRegistry::Find(*i)) {
            mWriterFactoryCache[mayaNodeType] = primWriterFactory;
            return primWriterFactory;
        }
    }

    // No writer found, so mark the type as unknown in the local cache.
    mWriterFactoryCache[mayaNodeType] = nullptr;
    return nullptr;
}

void
usdWriteJobCtx::CreatePrimWriterHierarchy(
    const MDagPath& rootDag,
    const SdfPath& rootUsdPath,
    const bool forceUninstance,
    const bool exportRootVisibility,
    std::vector<MayaPrimWriterSharedPtr>* primWritersOut)
{
    if (!primWritersOut) {
        TF_CODING_ERROR("primWritersOut is null");
        return;
    }

    // The USD path of the prototype root if it were exported at its current
    // Maya location.
    const SdfPath rootComputedUsdPath = this->ConvertDagToUsdPath(rootDag);

    MItDag itDag(MItDag::kDepthFirst, MFn::kInvalid);
    itDag.reset(rootDag);
    for (; !itDag.isDone(); itDag.next()) {
        MDagPath curDagPath;
        itDag.getPath(curDagPath);

        if (!this->needToTraverse(curDagPath)) {
            itDag.prune();
            continue;
        }

        // The USD path of this prototype descendant prim if it were exported
        // at its current Maya location.
        const SdfPath curComputedUsdPath =
                this->ConvertDagToUsdPath(curDagPath);

        SdfPath curActualUsdPath;
        if (rootUsdPath.IsEmpty()) {
            // Just use the actual computed current path.
            curActualUsdPath = curComputedUsdPath;
        }
        else {
            // Compute the current prim's relative path w/r/t the prototype
            // root, and use this to re-anchor it under the USD stage location
            // where we want to write out the prototype.
            const SdfPath curRelPath = curComputedUsdPath.MakeRelativePath(
                    rootComputedUsdPath);
            curActualUsdPath = rootUsdPath.AppendPath(curRelPath);
        }

        // Currently, forceUninstance only applies to the root DAG path but not
        // to descendant nodes (i.e. nested instancing will always occur).
        // Its purpose is to allow us to do the actual write of the master.
        MayaPrimWriterSharedPtr writer = this->CreatePrimWriter(
                curDagPath,
                curActualUsdPath,
                curDagPath == rootDag ? forceUninstance : false);
        if (!writer) {
            continue;
        }

        if (!exportRootVisibility && writer->GetUsdPath() == rootUsdPath) {
            writer->SetExportVisibility(false);
        }

        if (writer->ShouldPruneChildren()) {
            itDag.prune();
        }

        primWritersOut->push_back(writer);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
