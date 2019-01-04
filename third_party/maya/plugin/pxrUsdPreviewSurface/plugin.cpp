//
// Copyright 2018 Pixar
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
#include "pxrUsdPreviewSurface/api.h"

#include "pxrUsdPreviewSurface/usdPreviewSurface.h"
#include "pxrUsdPreviewSurface/usdPreviewSurfaceShadingNodeOverride.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/thisPlugin.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

#include <maya/MDrawRegistry.h>
#include <maya/MFnPlugin.h>
#include <maya/MFragmentManager.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPxNode.h>
#include <maya/MShaderManager.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>

#include <string>


PXR_NAMESPACE_USING_DIRECTIVE


static const MString _RegistrantId("pxrUsdPreviewSurfacePlugin");

static const TfTokenVector _FragmentNames = {
    PxrMayaUsdPreviewSurfaceShadingNodeTokens->Float4ToFloatXFragmentName,
    PxrMayaUsdPreviewSurfaceShadingNodeTokens->Float4ToFloatYFragmentName,
    PxrMayaUsdPreviewSurfaceShadingNodeTokens->Float4ToFloatZFragmentName,
    PxrMayaUsdPreviewSurfaceShadingNodeTokens->Float4ToFloatWFragmentName,
    PxrMayaUsdPreviewSurfaceShadingNodeTokens->LightingStructFragmentName,
    PxrMayaUsdPreviewSurfaceShadingNodeTokens->LightingFragmentName,
    PxrMayaUsdPreviewSurfaceShadingNodeTokens->CombinerFragmentName
};

static const TfTokenVector _FragmentGraphNames = {
    PxrMayaUsdPreviewSurfaceShadingNodeTokens->SurfaceFragmentGraphName
};


static
std::string
_GetResourcePath(const std::string& resource)
{
    static PlugPluginPtr plugin = PLUG_THIS_PLUGIN;
    if (!TF_VERIFY(plugin, "Could not get plugin\n")) {
        return std::string();
    }

    const std::string path = PlugFindPluginResource(plugin, resource);
    TF_VERIFY(!path.empty(), "Could not find resource: %s\n", resource.c_str());

    return path;
}

static
MStatus
_RegisterFragments()
{
    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer) {
        return MS::kFailure;
    }

    MHWRender::MFragmentManager* fragmentManager =
        theRenderer->getFragmentManager();
    if (!fragmentManager) {
        return MS::kFailure;
    }

    // Register all fragments.
    for (const TfToken& fragNameToken : _FragmentNames) {
        const MString fragName(fragNameToken.GetText());

        if (fragmentManager->hasFragment(fragName)) {
            continue;
        }

        const std::string fragXmlFile =
            TfStringPrintf("%s.xml", fragName.asChar());
        const std::string fragXmlPath = _GetResourcePath(fragXmlFile);

        const MString addedName =
            fragmentManager->addShadeFragmentFromFile(
                fragXmlPath.c_str(),
                false);

        if (addedName != fragName) {
            MGlobal::displayError(
                TfStringPrintf("Failed to register fragment '%s' from file: %s",
                               fragName.asChar(),
                               fragXmlPath.c_str()).c_str());
            return MS::kFailure;
        }
    }

    // Register all fragment graphs.
    for (const TfToken& fragGraphNameToken : _FragmentGraphNames) {
        const MString fragGraphName(fragGraphNameToken.GetText());

        if (fragmentManager->hasFragment(fragGraphName)) {
            continue;
        }

        const std::string fragGraphXmlFile =
            TfStringPrintf("%s.xml", fragGraphName.asChar());
        const std::string fragGraphXmlPath = _GetResourcePath(fragGraphXmlFile);

        const MString addedName =
            fragmentManager->addFragmentGraphFromFile(fragGraphXmlPath.c_str());
        if (addedName != fragGraphName) {
            MGlobal::displayError(
                TfStringPrintf("Failed to register fragment graph '%s' from file: %s",
                               fragGraphName.asChar(),
                               fragGraphXmlPath.c_str()).c_str());
            return MS::kFailure;
        }
    }

    return MS::kSuccess;
}

static
MStatus
_DeregisterFragments()
{
    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer) {
        return MS::kFailure;
    }

    MHWRender::MFragmentManager* fragmentManager =
        theRenderer->getFragmentManager();
    if (!fragmentManager) {
        return MS::kFailure;
    }

    // De-register all fragment graphs.
    for (const TfToken& fragGraphNameToken : _FragmentGraphNames) {
        const MString fragGraphName(fragGraphNameToken.GetText());

        if (!fragmentManager->removeFragment(fragGraphName)) {
            MGlobal::displayWarning(
                TfStringPrintf("Failed to remove fragment graph: %s",
                               fragGraphName.asChar()).c_str());
            return MS::kFailure;
        }
    }

    // De-register all fragments.
    for (const TfToken& fragNameToken : _FragmentNames) {
        const MString fragName(fragNameToken.GetText());

        if (!fragmentManager->removeFragment(fragName)) {
            MGlobal::displayWarning(
                TfStringPrintf("Failed to remove fragment: %s",
                               fragName.asChar()).c_str());
            return MS::kFailure;
        }
    }

#if MAYA_API_VERSION >= 201700
    // Clear the shader manager's effect cache as well so that any changes to
    // the fragments will get picked up if they are re-registered.
    const MHWRender::MShaderManager* shaderMgr = theRenderer->getShaderManager();
    if (!shaderMgr) {
        return MS::kFailure;
    }

    MStatus status = shaderMgr->clearEffectCache();
    if (status != MS::kSuccess) {
        MGlobal::displayWarning("Failed to clear shader manager effect cache");
        return status;
    }
#endif

    return MS::kSuccess;
}

PXRUSDPREVIEWSURFACE_API_H
MStatus
initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj, "Pixar", "1.0", "Any");

    status = plugin.registerNode(
        PxrMayaUsdPreviewSurface::typeName,
        PxrMayaUsdPreviewSurface::typeId,
        PxrMayaUsdPreviewSurface::creator,
        PxrMayaUsdPreviewSurface::initialize,
        MPxNode::kDependNode,
        &PxrMayaUsdPreviewSurface::fullClassification);
    CHECK_MSTATUS(status);

    if (MGlobal::mayaState() == MGlobal::kInteractive) {
        status = _RegisterFragments();
        CHECK_MSTATUS(status);

        status =
            MHWRender::MDrawRegistry::registerSurfaceShadingNodeOverrideCreator(
                PxrMayaUsdPreviewSurface::drawDbClassification,
                _RegistrantId,
                PxrMayaUsdPreviewSurfaceShadingNodeOverride::creator);
        CHECK_MSTATUS(status);
    }

    return status;
}

PXRUSDPREVIEWSURFACE_API_H
MStatus
uninitializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj);

    if (MGlobal::mayaState() == MGlobal::kInteractive) {
        status =
            MHWRender::MDrawRegistry::deregisterSurfaceShadingNodeOverrideCreator(
                PxrMayaUsdPreviewSurface::drawDbClassification,
                _RegistrantId);
        CHECK_MSTATUS(status);

        status = _DeregisterFragments();
        CHECK_MSTATUS(status);
    }

    status = plugin.deregisterNode(PxrMayaUsdPreviewSurface::typeId);
    CHECK_MSTATUS(status);

    return status;
}
