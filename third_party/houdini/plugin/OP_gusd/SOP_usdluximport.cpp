//
// Copyright 2017 Pixar
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
#include "SOP_usdluximport.h"

#include "gusd/GU_USD.h"
#include "gusd/PRM_Shared.h"
#include "gusd/stageCache.h"
#include "gusd/stageEdit.h"
#include "gusd/USD_Traverse.h"
#include "gusd/USD_Utils.h"
#include "gusd/UT_Assert.h"
#include "gusd/error.h"
#include "gusd/UT_StaticInit.h"
#include "gusd/lightWrapper.h"

#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/fileUtils.h>


#include <GA/GA_AttributeFilter.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Director.h>
#include <OP/OP_OperatorTable.h>
#include <PI/PI_EditScriptedParms.h>
#include <PRM/PRM_AutoDeleter.h>
#include <UT/UT_WorkArgs.h>
#include <UT/UT_UniquePtr.h>
#include <PY/PY_Python.h>

PXR_NAMESPACE_OPEN_SCOPE

using std::string;

namespace {

enum ErrorChoice
{
    MISSINGFRAME_ERR, MISSINGFRAME_WARN
};

#define _NOTRAVERSE_NAME "none"


void _ConcatTemplates(UT_Array<PRM_Template>& array,
                      const PRM_Template* templates)
{
    int count = PRM_Template::countTemplates(templates);
    if (count > 0)
    {
        exint idx = array.size();
        array.bumpSize(array.size() + count);
        UTconvertArray(&array(idx), templates, count);
    }
}

int
_onTreeView(void* data, int index, fpreal t, const PRM_Template* tplate)
{
    GusdSOP_usdluximport* sop = static_cast<GusdSOP_usdluximport*>(data);
    UT_String path;
    sop->getFullPath(path);

    const std::string statement = TfStringPrintf(
            "hou.node('%s').setSelected(1)\n"
                    "treePane = hou.ui.curDesktop().createFloatingPaneTab("
                    "hou.paneTabType.PythonPanel, (1200, 600), (800, 500))\n"
                    "treePane.setActiveInterface(hou.pypanel.interfaceByName('UsdImport'))\n",
            path.c_str());

    PYrunPythonStatements(statement.c_str());
    return 1;
}

int
_onReload(void* data, int index, fpreal t, const PRM_Template* tplate)
{
    GusdSOP_usdluximport* sop = static_cast<GusdSOP_usdluximport*>(data);
    sop->Reload();
    return 1;
}

PRM_Template* _CreateTemplates()
{
    /* XXX: All names should be prefixed to ensure they don't
            collide with the templates of the traversal plugins.*/

    static PRM_Name fileName("import_file", "USD File");
    static PRM_Name primPathName("import_primpath", "Prim Path");
    static PRM_SpareData primPathSpareData(
            PRM_SpareArgs()
                    << PRM_SpareToken("fileprm", fileName.getToken())
                    << PRM_SpareToken("primpathprm", primPathName.getToken())
                    << PRM_SpareToken(PRM_SpareData::getEditorToken(), "1")
                    << PRM_SpareToken(PRM_SpareData::getEditorLinesRangeToken(),
                                      "1-10"));
    static PRM_Default primPathDefault(0, "/");

    static PRM_Default usdPathDefault(0, "");

    static PRM_Name treeViewName("treeview", "Tree View");
    static PRM_Name reloadName("reload", "Reload");

    static PRM_Name existingLightsName("existingLights", "Existing lights");
    static PRM_Default existingLightsDefault(0, "skip");
    static PRM_Name existingLightsChoices[] = {
            PRM_Name("skip", "skip import"),
            PRM_Name("duplicate", "duplicate light"),
            PRM_Name("overrideLight", "override light"),
            PRM_Name(0)
    };
    static PRM_ChoiceList existingLightsMenu(PRM_CHOICELIST_SINGLE, existingLightsChoices);


    static PRM_Name purposeName("purpose", "Purpose");
    static PRM_Default purposeDefault(0, "proxy");
    static PRM_Name purposeChoices[] = {
            PRM_Name("proxy", "proxy"),
            PRM_Name("render", "render"),
            PRM_Name("guide", "guide"),
            PRM_Name(0)
    };
    static PRM_ChoiceList purposeMenu(PRM_CHOICELIST_TOGGLE, purposeChoices);

    static PRM_Name useNetboxesName("use_netboxes", "Import with Netboxes");

    static PRM_Name missingFrameName("missingframe", "Missing Frame");
    static PRM_Default missingFrameDefault(0, "error");

    static PRM_Name errorChoices[] = {
            PRM_Name("error", "Report Error"),
            PRM_Name("warning", "Report Warning"),
            PRM_Name(0)
    };

    static PRM_ChoiceList errorMenu(PRM_CHOICELIST_SINGLE,
                                    errorChoices);

    // These next 3 parameters are required by the DT_importUsd
    // plugin, which uses these 3 hidden parameters to read/write
    // to this OP_Node.
    static PRM_Name parmNameUsdfile("_parmname_usdfile",
                                    "_parmname_usdfile");
    static PRM_Name parmNamePrimpaths("_parmname_primpaths",
                                      "_parmname_primpaths");
    static PRM_Name parmUiExpandState("_parm_uiexpandstate",
                                      "_parm_uiexpandstate");
    static PRM_Default parmDefaultUsdfile(0, fileName.getToken());
    static PRM_Default parmDefaultPrimpaths(0, primPathName.getToken());
    static PRM_Default parmDefaultUiExpandState(0, "");

    // Make the uiExpandState template here, so it can be
    // configured to not cook this SOP when it changes.
    PRM_Template uiExpandState(PRM_STRING | PRM_TYPE_INVISIBLE, 1,
                               &parmUiExpandState, &parmDefaultUiExpandState);
    uiExpandState.setNoCook(true);

    GusdPRM_Shared shared;

    static PRM_Template templates[] = {
            PRM_Template(PRM_FILE, 1, &fileName,
                    /*default*/ &usdPathDefault, /*choicelist*/ 0, /*range*/ 0,
                    /*callback*/ 0, &shared->usdFileROData),
            PRM_Template(PRM_CALLBACK, 1, &treeViewName, 0, 0, 0, &_onTreeView),
            PRM_Template(PRM_CALLBACK, 1, &reloadName, 0, 0, 0, &_onReload),
            PRM_Template(PRM_STRING, 1, &primPathName,
                    /*default*/ &primPathDefault, &shared->multiPrimMenu,
                    /*range*/ 0, /*callback*/ 0, &primPathSpareData),
            PRM_Template(PRM_SEPARATOR),
            PRM_Template(PRM_TOGGLE, 1, &useNetboxesName, PRMoneDefaults),
            PRM_Template(PRM_STRING, 1,
                         &purposeName,
                         &purposeDefault,
                         &purposeMenu),
            PRM_Template(PRM_STRING, 1,
                         &existingLightsName,
                         &existingLightsDefault,
                         &existingLightsMenu),
            PRM_Template(PRM_ORD, 1, &missingFrameName,
                         &missingFrameDefault, &errorMenu),
            PRM_Template(PRM_STRING | PRM_TYPE_INVISIBLE, 1,
                         &parmNameUsdfile, &parmDefaultUsdfile),
            PRM_Template(PRM_STRING | PRM_TYPE_INVISIBLE, 1,
                         &parmNamePrimpaths, &parmDefaultPrimpaths),
            uiExpandState,
            PRM_Template()
    };

    return templates;
}


auto _mainTemplates(GusdUT_StaticVal(_CreateTemplates));


} /*namespace*/

UsdFileGetterFunc GusdSOP_usdluximport::s_usdFileGetter = nullptr;

void
GusdSOP_usdluximport::Register(OP_OperatorTable* table)
{
    OP_Operator* op =
            new OP_Operator("pixar::usdluximport",
                            "USD Lux Import",
                            Create,
                            *_mainTemplates,
                            0 /* min inputs */,
                            1 /* max inputs */,
                            0 /* variables  */,
                            OP_FLAG_GENERATOR);
    op->setIconName("pxh_gusdIcon.png");
    op->setOpTabSubMenuPath("Pixar");
    table->addOperator(op);
    table->setOpFirstName("pixar::usdluximport", "usdluximport");
}

OP_Node*
GusdSOP_usdluximport::Create(OP_Network* net, const char* name, OP_Operator* op)
{
    return new GusdSOP_usdluximport(net, name, op);
}

GusdSOP_usdluximport::GusdSOP_usdluximport(
        OP_Network* net, const char* name, OP_Operator* op)
        : SOP_Node(net, name, op), _group(nullptr) {}


bool
GusdSOP_usdluximport::updateParmsFlags()
{
    bool haveNoInput = getInput(0) == nullptr;

    return enableParm("import_file", haveNoInput) +
           enableParm("import_primpath", haveNoInput);
}

void
GusdSOP_usdluximport::Reload()
{
    UT_String file;
    const fpreal t = CHgetEvalTime();
    evalString(file, "import_file", 0, t);
    if (!file.isstring())
    {
        return;
    }

    // A different lastCookFilepath makes sure that we import the file again.
    m_lastCookFilepath = {};

    UT_StringSet paths;
    paths.insert(file);

    GusdStageCacheWriter cache;
    cache.ReloadStages(paths);
    forceRecook();
}

OP_ERROR
GusdSOP_usdluximport::_Cook(OP_Context& ctx)
{
    fpreal t = ctx.getTime();

    ErrorChoice errorMode = static_cast<ErrorChoice>(evalInt("missingframe", 0, t));

    UT_ErrorSeverity errorSev =
        errorMode == MISSINGFRAME_WARN ? UT_ERROR_WARNING : UT_ERROR_ABORT;

    UT_String traversalName = "std:lights";
    const auto& table = GusdUSD_TraverseTable::GetInstance();
    const GusdUSD_Traverse* trav = table.FindTraversal(traversalName);

    if (!trav)
    {
        GUSD_ERR().Msg("Failed locating traversal '%s'", traversalName.c_str());
        return error();
    }
    return _CreateNewPrims(ctx, trav, errorSev);
}

OP_ERROR
GusdSOP_usdluximport::_CreateNewPrims(OP_Context& ctx,
                                      const GusdUSD_Traverse* traverse,
                                      UT_ErrorSeverity errorSev)
{
    fpreal t = ctx.getTime();

    bool useNetboxes = evalInt("use_netboxes", 0, t);

    UT_String overridePolicy;
    evalString(overridePolicy, "existingLights", 0, 0);

    UT_String file;
    evalString(file, "import_file", 0, t);

    UT_String primPath;
    evalString(primPath, "import_primpath", 0, t);


    if (!file.isstring() || !primPath.isstring())
    {
        // Nothing to do.
        return UT_ERROR_NONE;
    }

    // Check if we already imported the file in the previous cook.
    if (m_lastCookFilepath == file)
    {
        // Already imported this file. Don't import again.
        return UT_ERROR_NONE;
    }

    /* The prim path may be a list of prims.
       Additionally, those prim paths may include variants
       (eg., /some/model{variant=sel}/subscope ).
       Including multiple variants may mean that we need to access
       multiple stages.

       Resolve the actual set of prims and variants first.*/

    UT_Array<SdfPath> primPaths, variants;
    if (!GusdUSD_Utils::GetPrimAndVariantPathsFromPathList(
            primPath, primPaths, variants, errorSev))
        return error();

    GusdDefaultArray <UT_StringHolder> filePaths;
    filePaths.SetConstant(file);

    // Get stage edits applying any of our variants.
    GusdDefaultArray <GusdStageEditPtr> edits;
    edits.GetArray().setSize(variants.size());
    for (exint i = 0; i < variants.size(); ++i)
    {
        if (!variants(i).IsEmpty())
        {
            GusdStageBasicEdit* edit = new GusdStageBasicEdit;
            edit->GetVariants().append(variants(i));
            edits.GetArray()(i).reset(edit);
        }
    }

    // Load the root prims.
    UT_Array<UsdPrim> rootPrims;
    {
        rootPrims.setSize(primPaths.size());

        GusdStageCacheReader cache;
        if (!cache.GetPrims(filePaths, primPaths, edits,
                            rootPrims.data(),
                            GusdStageOpts::LoadAll(), errorSev))
            return error();
    }

    for (exint rootPrimIdx = 0; rootPrimIdx < rootPrims.size(); ++rootPrimIdx)
    {
        auto& rootPrim = rootPrims[rootPrimIdx];
        processPrim(rootPrim, overridePolicy, useNetboxes);
    }

    m_lastCookFilepath = file;
    // Harden the m_lastCookFilepath to make sure we don't just have a shallow
    // copy of the string which would be out of scope the next time we call cook().
    m_lastCookFilepath.harden();

    return error();
}

void
GusdSOP_usdluximport::processPrim(const UsdPrim& prim, const UT_String& overridePolicy, const bool useNetboxes)
{
    if (prim.IsPseudoRoot())
    {
        for (auto childPrim: prim.GetChildren())
        {
            processPrim(childPrim, overridePolicy, useNetboxes);
        }
        return;
    }

    auto& typeName = prim.GetTypeName();
    if (typeName == TfToken("Xform"))
    {
        importSubnet(prim, overridePolicy, useNetboxes);

        for (auto childPrim: prim.GetChildren())
        {
            processPrim(childPrim, overridePolicy, useNetboxes);
        }
        return;
    }

    UsdLuxLight light(prim);
    if (light)
    {
        importLight(prim, overridePolicy, useNetboxes);
    }
}

void
GusdSOP_usdluximport::importLight(const UsdPrim& prim, const UT_String& overridePolicy, const bool useNetboxes)
{
    UsdLightWrapper::read(prim, overridePolicy, useNetboxes, m_transformMapping);
}

void
GusdSOP_usdluximport::importSubnet(const UsdPrim& prim, const UT_String& overridePolicy, const bool useNetboxes)
{
    OP_Node* node = UsdLightWrapper::read(prim, overridePolicy, useNetboxes, m_transformMapping);
    if (node)
    {
        m_transformMapping[prim.GetPath()] = node->getFullPath().c_str();
    }
}

OP_ERROR
GusdSOP_usdluximport::cookMySop(OP_Context& ctx)
{
    OP_AutoLockInputs lock(this);
    if (lock.lock(ctx) >= UT_ERROR_ABORT)
        return error();

    // Local var support.
    setCurGdh(0, myGdpHandle);
    setupLocalVars();

    if (getInput(0))
        duplicateSource(0, ctx);
    else
        gdp->clearAndDestroy();

    _Cook(ctx);

    resetLocalVarRefs();

    return error();
}


void
GusdSOP_usdluximport::finishedLoadingNetwork(bool isChildCall)
{
    SOP_Node::finishedLoadingNetwork(isChildCall);
}

PXR_NAMESPACE_CLOSE_SCOPE
