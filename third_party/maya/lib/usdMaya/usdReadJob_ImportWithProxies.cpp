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
#include "usdMaya/usdReadJob.h"

#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"
#include "usdMaya/primReaderRegistry.h"
#include "usdMaya/stageCache.h"
#include "usdMaya/translatorModelAssembly.h"
#include "usdMaya/translatorUtil.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/stageCacheContext.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdUtils/pipeline.h"
#include "pxr/usd/usdUtils/stageCache.h"

#include <maya/MDagModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPxRepresentation.h>
#include <maya/MString.h>

#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((PointInstancerTypeName, "PxPointInstancer"))
    ((XformTypeName, "Xform"))
    ((GeomRootName, "Geom"))
    ((ScopePrimTypeName, "Scope"))

    ((MayaProxyShapeNodeName, "GeomProxy"))
    ((ExcludePrimPathsPlugName, "excludePrimPaths"))
);


/*******************************************************************************
*                                                                              *
* XXX: These functions could potentially be plugin points studios might want   *
*      to customize. At the moment, they reflect Pixar pipeline conventions.   *
*                                                                              *
*******************************************************************************/

static
bool
_ShouldImportAsSubAssembly(const UsdPrim& prim)
{
    // XXX: We need to identify dressGroups by prim type, since dressGroups
    // nested inside component models will have kind subcomponent rather than
    // dress_group.  We hardcode the typeName to avoid pulling in a pixar-only
    // dependency. Must fix.
    if (prim.GetTypeName() == _tokens->PointInstancerTypeName) {
        return true;
    }

    TfToken kind;
    UsdModelAPI usdModel(prim);
    usdModel.GetKind(&kind);

    if (KindRegistry::IsA(kind, KindTokens->component) ||
            KindRegistry::IsA(kind, KindTokens->assembly)) {
        return true;
    }

    return false;
}

static
bool
_IsCollapsePoint(const UsdPrim& prim)
{
    TfToken kind;
    UsdModelAPI usdModel(prim);
    usdModel.GetKind(&kind);

    if (KindRegistry::IsA(kind, KindTokens->subcomponent)) {
        return true;
    }

    return false;
}

// XXX: At the moment, the 'Geom' scope is just a convenient place to insert
// a model's top-level proxy. Ultimately, we should always create the top-level
// proxy for the imported model and not depend on the existence of this scope.
static
bool
_IsPxrGeomRoot(const UsdPrim& prim)
{
    if (prim.GetName() == _tokens->GeomRootName &&
            prim.GetParent() && prim.GetParent().IsModel()) {
        return true;
    }

    return false;
}

/*******************************************************************************
*                                                                              *
* XXX: End possible plugin section.                                            *
*                                                                              *
*******************************************************************************/

static
bool
_CreateParentTransformNodes(const UsdPrim& usdPrim,
                            const PxrUsdMayaPrimReaderArgs& args,
                            PxrUsdMayaPrimReaderContext* context)
{
    const UsdPrim parentPrim = usdPrim.GetParent();
    if (!parentPrim ||
            parentPrim == usdPrim.GetStage()->GetPseudoRoot()) {
        return true;
    }

    MObject parentNode = context->GetMayaNode(parentPrim.GetPath(), false);
    if (parentNode != MObject::kNullObj) {
        // A Maya node exists for usdPrim's immediate parent, so we're done.
        return true;
    }

    // usdPrim's parent does not have a Maya node yet, so create all of *its*
    // parents before we create a node for the parent itself.
    bool success = _CreateParentTransformNodes(parentPrim, args, context);
    if (!success) {
        return false;
    }

    // Get the parent node of parentPrim (usdPrim's grandparent) and use that
    // to create a node for parentPrim.
    MObject grandParentNode =
        context->GetMayaNode(parentPrim.GetPath().GetParentPath(), false);

    MStatus status;
    return PxrUsdMayaTranslatorUtil::CreateTransformNode(parentPrim,
                                                         grandParentNode,
                                                         args,
                                                         context,
                                                         &status,
                                                         &parentNode);
}

bool
usdReadJob::_ProcessProxyPrims(
        const std::vector<UsdPrim>& proxyPrims,
        const UsdPrim& pxrGeomRoot,
        const std::vector<std::string>& collapsePointPathStrings)
{
    TF_FOR_ALL(iter, proxyPrims) {
        const UsdPrim proxyPrim = *iter;
        PxrUsdMayaPrimReaderArgs args(proxyPrim,
                                      mArgs.shadingMode,
                                      mArgs.defaultMeshScheme,
                                      mArgs.readAnimData,
                                      mArgs.useCustomFrameRange,
                                      mArgs.startTime,
                                      mArgs.endTime,
                                      mArgs.useEulerFilter);
        PxrUsdMayaPrimReaderContext ctx(&mNewNodeRegistry);

        if (!_CreateParentTransformNodes(proxyPrim, args, &ctx)) {
            return false;
        }

        MObject parentNode = ctx.GetMayaNode(proxyPrim.GetPath().GetParentPath(), false);
        if (!PxrUsdMayaTranslatorModelAssembly::ReadAsProxy(proxyPrim,
                                                               mVariants,
                                                               parentNode,
                                                               args,
                                                               &ctx,
                                                               _proxyShapeTypeName)) {
            return false;
        }
    }

    // Author exclude paths on the top-level proxy using the list of collapse
    // points we found.
    if (!collapsePointPathStrings.empty()) {
        MStatus status;
        PxrUsdMayaPrimReaderContext ctx(&mNewNodeRegistry);

        // Get the geom root proxy shape node.
        SdfPath proxyShapePath = pxrGeomRoot.GetPath().AppendChild(
            _tokens->MayaProxyShapeNodeName);
        MObject proxyShapeObj = ctx.GetMayaNode(proxyShapePath, false);
        MFnDependencyNode depNodeFn(proxyShapeObj, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        std::string excludePathsString =
            TfStringJoin(collapsePointPathStrings, ",");

        // Set the excludePrimPaths attribute on the node.
        MDagModifier dagMod;
        MPlug excludePathsPlug = depNodeFn.findPlug(
            _tokens->ExcludePrimPathsPlugName.GetText(), true, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        status = dagMod.newPlugValueString(excludePathsPlug, excludePathsString.c_str());
        CHECK_MSTATUS_AND_RETURN(status, false);
        status = dagMod.doIt();
        CHECK_MSTATUS_AND_RETURN(status, false);
    }

    return true;
}

bool
usdReadJob::_ProcessSubAssemblyPrims(const std::vector<UsdPrim>& subAssemblyPrims)
{
    TF_FOR_ALL(iter, subAssemblyPrims) {
        const UsdPrim subAssemblyPrim = *iter;
        PxrUsdMayaPrimReaderArgs args(subAssemblyPrim,
                                      mArgs.shadingMode,
                                      mArgs.defaultMeshScheme,
                                      mArgs.readAnimData,
                                      mArgs.useCustomFrameRange,
                                      mArgs.startTime,
                                      mArgs.endTime,
                                      mArgs.useEulerFilter);
        PxrUsdMayaPrimReaderContext ctx(&mNewNodeRegistry);

        // We use the file path of the file currently being imported and
        // the path to the prim within that file when creating the
        // subassembly.
        std::string subAssemblyUsdFilePath = mFileName;
        SdfPath subAssemblyUsdPrimPath = subAssemblyPrim.GetPath();

        if (!_CreateParentTransformNodes(subAssemblyPrim, args, &ctx)) {
            return false;
        }

        MObject parentNode = ctx.GetMayaNode(subAssemblyPrim.GetPath().GetParentPath(), false);
        if (!PxrUsdMayaTranslatorModelAssembly::Read(subAssemblyPrim,
                                                        subAssemblyUsdFilePath,
                                                        subAssemblyUsdPrimPath,
                                                        parentNode,
                                                        args,
                                                        &ctx,
                                                        _assemblyTypeName,
                                                        mArgs.assemblyRep)) {
            return false;
        }
    }

    return true;
}

bool
usdReadJob::_ProcessCameraPrims(const std::vector<UsdPrim>& cameraPrims)
{
    TF_FOR_ALL(iter, cameraPrims) {
        const UsdPrim cameraPrim = *iter;
        PxrUsdMayaPrimReaderArgs args(cameraPrim,
                                      mArgs.shadingMode,
                                      mArgs.defaultMeshScheme,
                                      mArgs.readAnimData,
                                      mArgs.useCustomFrameRange,
                                      mArgs.startTime,
                                      mArgs.endTime,
                                      mArgs.useEulerFilter);
        PxrUsdMayaPrimReaderContext ctx(&mNewNodeRegistry);

        if (!_CreateParentTransformNodes(cameraPrim, args, &ctx)) {
            return false;
        }

        if (PxrUsdMayaPrimReaderRegistry::ReaderFactoryFn factoryFn =
                PxrUsdMayaPrimReaderRegistry::Find(cameraPrim.GetTypeName())) {
            PxrUsdMayaPrimReaderPtr primReader = factoryFn(args);
            if (primReader) {
                primReader->Read(&ctx);
            }
        }
    }

    return true;
}

bool
usdReadJob::_DoImportWithProxies(UsdPrimRange& range)
{
    MStatus status;

    // We'll iterate through the prims collecting the various types we're
    // interested in, but we defer creating any Maya nodes until we've finished
    // iterating. This way we'll know all the paths we'll need to re-create in
    // Maya, and we can create only the transforms necessary to produce those
    // paths.
    std::vector<UsdPrim> cameraPrims;
    std::vector<UsdPrim> subAssemblyPrims;
    std::vector<UsdPrim> proxyPrims;

    UsdPrim pxrGeomRoot;
    std::vector<std::string> collapsePointPathStrings;

    for(auto primIt = range.begin(); primIt != range.end(); ++primIt) {
        const UsdPrim& prim = *primIt;

        if (prim.IsA<UsdGeomCamera>()) {
            cameraPrims.push_back(prim);
            primIt.PruneChildren();
        } else if (_ShouldImportAsSubAssembly(prim)) {
            subAssemblyPrims.push_back(prim);
            primIt.PruneChildren();
        } else if (_IsPxrGeomRoot(prim)) {
            // This will be a top-level proxy node, so we do NOT prune the
            // iteration here. Collapse points below this prim will become
            // exclude paths.
            pxrGeomRoot = prim;
            proxyPrims.push_back(prim);
        } else if (pxrGeomRoot) {
            if (_IsCollapsePoint(prim)) {
                collapsePointPathStrings.push_back(prim.GetPath().GetString());
                proxyPrims.push_back(prim);
                primIt.PruneChildren();
            }
        } else if (prim.IsA<UsdGeomGprim>()) {
            proxyPrims.push_back(prim);
            primIt.PruneChildren();
        } else if (prim.GetTypeName() == _tokens->ScopePrimTypeName) {
            // XXX: This is completely wrong, but I don't want to deal
            // with the fallout of fixing it right now.
            MGlobal::displayWarning(
                TfStringPrintf("Scope \"%s\". Skipping all children.",
                               prim.GetPath().GetText()).c_str());
            primIt.PruneChildren();
        } else if (prim.GetTypeName() != _tokens->XformTypeName) {
            // Don't complain about Xform prims being unsupported. For the
            // "Expanded" representation of assemblies, we'll only create the
            // transforms we need to in order to reach supported prims.
            MGlobal::displayWarning(
                TfStringPrintf("Prim type \"%s\" unsupported in 'Expanded' "
                               "representation for prim \"%s\". Skipping...",
                               prim.GetTypeName().GetText(),
                               prim.GetPath().GetText()).c_str());
        }
    }

    // Create the proxy nodes and author exclude paths on the geom root proxy.
    if (!_ProcessProxyPrims(proxyPrims,
                               pxrGeomRoot,
                               collapsePointPathStrings)) {
        return false;
    }

    // Create all sub-assembly nodes.
    if (!_ProcessSubAssemblyPrims(subAssemblyPrims)) {
        return false;
    }

    // Create all camera nodes.
    if (!_ProcessCameraPrims(cameraPrims)) {
        return false;
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

