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
#include "usdMaya/stageCache.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/scope.h"

#include <maya/MDagPathArray.h>
#include <maya/MString.h>
#include <maya/MPxNode.h>

#include <sstream>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE


namespace {
    inline
    SdfPath& rootOverridePath(
            const PxrUsdMayaJobExportArgs& args,
            SdfPath& path) {
        if (!args.usdModelRootOverridePath.IsEmpty() && !path.IsEmpty()) {
            path = path.ReplacePrefix(path.GetPrefixes()[0], args.usdModelRootOverridePath);
        }
        return path;
    }

    const SdfPath instancesScopePath("/InstanceSources");
}

usdWriteJobCtx::usdWriteJobCtx(const PxrUsdMayaJobExportArgs& args)
        : mArgs(args), mNoInstances(true)
{

}

SdfPath usdWriteJobCtx::getOrCreateMasterPath(const MDagPath& dg)
{
    const MObjectHandle handle(dg.node());
    const auto it = mMasterToUsdPath.find(handle);
    if (it != mMasterToUsdPath.end()) {
        return it->second;
    } else {
        MDagPathArray allInstances;
        auto status = MDagPath::getAllPathsTo(dg.node(), allInstances);
        if (!status || (allInstances.length() == 0)) { return SdfPath(); }
        // we are looking for the instance with the lowest number here
        // which is still exported
        auto dagCopy = allInstances[0];
        const auto usdPath = getUsdPathFromDagPath(dagCopy, true);
        dagCopy.pop();

        // This will get auto destroyed, because we are not storing it in the list
        MayaTransformWriterPtr transformPrimWriter(new MayaTransformWriter(dagCopy, usdPath.GetParentPath(), true, *this));
        if (!transformPrimWriter || !transformPrimWriter->IsValid()) {
            return SdfPath();
        }

        transformPrimWriter->Write(UsdTimeCode::Default());
        mMasterToUsdPath[handle] = transformPrimWriter->GetUsdPath();

        auto primWriter = _createPrimWriter(allInstances[0], SdfPath(), true);
        if (!primWriter) { // Note that _createPrimWriter ensures validity.
            return SdfPath();
        }

        primWriter->Write(UsdTimeCode::Default());
        mMasterToPrimWriter[handle] = mMayaPrimWriterList.size();
        mMayaPrimWriterList.push_back(primWriter);
        return transformPrimWriter->GetUsdPath();
    }
}

const MayaPrimWriterPtr
usdWriteJobCtx::getMasterPrimWriter(const MDagPath& dg) const
{
    const MObjectHandle handle(dg.node());
    const auto it = mMasterToPrimWriter.find(handle);
    if (it != mMasterToPrimWriter.end()) {
        size_t i = it->second;
        if (i < mMayaPrimWriterList.size()) {
            return mMayaPrimWriterList[i];
        }
    }

    return nullptr;
}

bool usdWriteJobCtx::needToTraverse(const MDagPath& curDag)
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

    return true;
}

SdfPath usdWriteJobCtx::getUsdPathFromDagPath(const MDagPath& dagPath, bool instanceSource /* = false */)
{
    SdfPath path;
    if (instanceSource) {
        if (mInstancesPrim) {
            mNoInstances = false;
            std::stringstream ss;
            ss << mInstancesPrim.GetPath().GetString();
            MObject node = dagPath.node();
            ss << "/";
            if (mArgs.stripNamespaces){
                ss << PxrUsdMayaUtil::stripNamespaces(dagPath.fullPathName()).asChar() + 1;
            } else{
                ss << dagPath.fullPathName().asChar() + 1;
            }
            if (!node.hasFn(MFn::kTransform)) {
                ss << "/Shape";
            }
            auto pathName = ss.str();
            pathName = TfStringReplace(pathName, "_", "__");  // avoid any issue with |: / _ name clashes
            std::replace(pathName.begin(), pathName.end(), '|', '_');
            std::replace(pathName.begin(), pathName.end(), ':', '_');
            path = SdfPath(pathName);
        } else {
            return SdfPath();
        }
    } else {
        path = PxrUsdMayaUtil::MDagPathToUsdPath(dagPath, false, mArgs.stripNamespaces);
        if (!mParentScopePath.IsEmpty())
        {
            // Since path is from MDagPathToUsdPath, it will always be
            // an absolute path...
            path = path.ReplacePrefix(
                    SdfPath::AbsoluteRootPath(),
                    mParentScopePath);
        }
    }
    return rootOverridePath(mArgs, path);
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
        mInstancesPrim = mStage->OverridePrim(instancesScopePath);
    }

    return true;
}

void usdWriteJobCtx::processInstances()
{
    if (mArgs.exportInstances) {
        if (mNoInstances) {
            mStage->RemovePrim(mInstancesPrim.GetPrimPath());
        } else {
            mInstancesPrim.SetSpecifier(SdfSpecifierOver);
        }
    }
}

MayaPrimWriterPtr usdWriteJobCtx::createPrimWriter(
    const MDagPath& curDag, const SdfPath& usdPath)
{
    return _createPrimWriter(curDag, usdPath, false);
}

MayaPrimWriterPtr usdWriteJobCtx::_createPrimWriter(
    const MDagPath& curDag, const SdfPath& usdPath, bool instanceSource)
{
    if (curDag.length() == 0) {
        // This is the world root node. It can't have a prim writer.
        return nullptr;
    }

    MObject ob = curDag.node();
    const SdfPath writePath = usdPath.IsEmpty() ?
            getUsdPathFromDagPath(curDag, instanceSource) : usdPath;

    if (mArgs.exportInstances && curDag.isInstanced() && !instanceSource) {
        // Deal with instances -- we just create a transform for them.
        MayaTransformWriterPtr primPtr = std::make_shared<MayaTransformWriter>(
                curDag, writePath, instanceSource, *this);
        if (primPtr->IsValid()) {
            return primPtr;
        }
    }
    else {
        // Deal with non-instances. Try to look up a writer plugin.
        // We search through the node's type ancestors, working backwards until
        // we find a prim writer plugin.
        MFnDependencyNode depNodeFn(ob);
        std::string mayaTypeName(depNodeFn.typeName().asChar());
        if (PxrUsdMayaPrimWriterRegistry::WriterFactoryFn primWriterFactory =
                _FindWriter(mayaTypeName)) {
            MayaPrimWriterPtr primPtr(primWriterFactory(
                    curDag, writePath, instanceSource, *this));
            if (primPtr && primPtr->IsValid()) {
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


PXR_NAMESPACE_CLOSE_SCOPE
