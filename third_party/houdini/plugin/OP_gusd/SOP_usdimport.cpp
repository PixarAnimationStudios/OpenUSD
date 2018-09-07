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
#include "SOP_usdimport.h"

#include "OP_Utils.h"

#include "gusd/error.h"
#include "gusd/GU_USD.h"
#include "gusd/PRM_Shared.h"
#include "gusd/stageCache.h"
#include "gusd/stageEdit.h"
#include "gusd/USD_Traverse.h"
#include "gusd/USD_Utils.h"
#include "gusd/UT_Assert.h"
#include "gusd/UT_StaticInit.h"
#include "gusd/GT_PrimCache.h"
#include "gusd/USD_XformCache.h"
#include "gusd/USD_VisCache.h"


#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/fileUtils.h>

#include <GA/GA_AttributeFilter.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Director.h>
#include <OP/OP_OperatorTable.h>
#include <PI/PI_EditScriptedParms.h>
#include <PRM/PRM_AutoDeleter.h>
#include <UT/UT_WorkArgs.h>
#include <UT/UT_UniquePtr.h>
#include <PY/PY_Python.h>

PXR_NAMESPACE_OPEN_SCOPE

using std::cerr;
using std::endl;
using std::string;

namespace {

enum ErrorChoice { MISSINGFRAME_ERR, MISSINGFRAME_WARN };

#define _NOTRAVERSE_NAME "none"


int _TraversalChangedCB(void* data, int idx, fpreal64 t,
                        const PRM_Template* tmpl)
{
    auto& sop = *reinterpret_cast<GusdSOP_usdimport*>(data);
    sop.UpdateTraversalParms();
    return 0;
}


void _ConcatTemplates(UT_Array<PRM_Template>& array,
                      const PRM_Template* templates)
{
    int count = PRM_Template::countTemplates(templates);
    if(count > 0) {
        exint idx = array.size();
        array.bumpSize(array.size() + count);
        UTconvertArray(&array(idx), templates, count);
    }
}


PRM_ChoiceList& _CreateTraversalMenu()
{
    static PRM_Name noTraverseName(_NOTRAVERSE_NAME, "No Traversal");

    static UT_Array<PRM_Name> names;
    names.append(noTraverseName);

    const auto& table = GusdUSD_TraverseTable::GetInstance();
    for(const auto& pair : table)
        names.append(pair.second->GetName());
    
    names.stdsort(
        [](const PRM_Name& a, const PRM_Name& b)    
        { return UT_String(a.getLabel()) < UT_String(b.getLabel()); });
    names.append(PRM_Name());

    static PRM_ChoiceList menu(PRM_CHOICELIST_SINGLE, &names(0));
    return menu;
}

int
_onTreeView(void *data, int index, fpreal t, const PRM_Template *tplate)
{
    GusdSOP_usdimport* sop = static_cast<GusdSOP_usdimport*>(data);
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
_onReload(void *data, int index, fpreal t, const PRM_Template *tplate)
{
    GusdSOP_usdimport* sop = static_cast<GusdSOP_usdimport*>(data);
    sop->Reload();
    return 1;
}

PRM_Template*   _CreateTemplates()
{
    /* XXX: All names should be prefixed to ensure they don't
            collide with the templates of the traversal plugins.*/

    static PRM_Name className("import_class", "Class");
    static PRM_Name groupName("import_group", "Group");

    static PRM_Name fileName("import_file", "USD File");
    static PRM_Name primPathName("import_primpath", "Prim Path");
    static PRM_SpareData primPathSpareData(
        PRM_SpareArgs()
            << PRM_SpareToken("fileprm", fileName.getToken())
            << PRM_SpareToken("primpathprm", primPathName.getToken())
            << PRM_SpareToken(PRM_SpareData::getEditorToken(), "1")
            << PRM_SpareToken(PRM_SpareData::getEditorLinesRangeToken(),
                              "1-10"));

    static PRM_Name treeViewName( "treeview", "Tree View");

    static PRM_Name reloadName( "reload", "Reload" );
    static PRM_Name timeName("import_time", "Time");
    static PRM_Default timeDef(0, "$RFSTART");

    static PRM_Name traversalName("import_traversal", "Traversal");
    static PRM_Default traversalDef(0, "none" );

    static PRM_Name deloldName("import_delold", "Delete Old Points/Prims");

    static PRM_Name    viewportlodName("viewportlod", "Display As");
    static PRM_Default viewportlodDefault(0, "full");

    static PRM_Conditional disableWhenNotPrims("{ import_class != \"primitive\" }");

    static PRM_Name purposeName("purpose", "Purpose");
    static PRM_Default purposeDefault(0, "proxy");
    static PRM_Name purposeChoices[] = {
        PRM_Name("proxy", "proxy"),
        PRM_Name("render", "render"),
        PRM_Name("guide", "guide"),
        PRM_Name(0)
    };
    static PRM_ChoiceList purposeMenu(PRM_CHOICELIST_TOGGLE, purposeChoices );

    static PRM_Name    missingFrameName("missingframe", "Missing Frame");
    static PRM_Default missingFrameDefault(1, "warning");

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
        PRM_Template(PRM_STRING, 1, &groupName, /*default*/ 0),
        PRM_Template(PRM_ORD, 1, &className, /*default*/ 0,
                     &PRMentityMenuPointsAndPrimitives),
        PRM_Template(PRM_TOGGLE, 1, &deloldName, PRMoneDefaults),
        PRM_Template(PRM_FILE, 1, &fileName,
                     /*default*/ 0, /*choicelist*/ 0, /*range*/ 0,
                     /*callback*/ 0, &shared->usdFileROData),
        PRM_Template(PRM_CALLBACK, 1, &treeViewName, 0, 0, 0, &_onTreeView ),
        PRM_Template(PRM_STRING, 1, &primPathName,
                     /*default*/ 0, &shared->multiPrimMenu,
                     /*range*/ 0, /*callback*/ 0, &primPathSpareData),
        PRM_Template(PRM_CALLBACK, 1, &reloadName, 0, 0, 0, &_onReload ),

        PRM_Template(PRM_FLT, 1, &timeName, &timeDef),
        PRM_Template(PRM_ORD, 1, &traversalName,
                     &traversalDef, &_CreateTraversalMenu(),
                     /*range*/ 0, _TraversalChangedCB),
        PRM_Template(PRM_SEPARATOR),
        PRM_Template(PRM_ORD, 1, &viewportlodName, 
                     &viewportlodDefault, &PRMviewportLODMenu,
                     0 /* range */,
                     0 /* callback */,
                     0 /* spare */,
                     0 /* group */,
                     0 /* help */,
                     &disableWhenNotPrims ),
        PRM_Template(PRM_STRING, 1, 
                     &purposeName,
                     &purposeDefault,
                     &purposeMenu ),
        PRM_Template(PRM_ORD, 1, &missingFrameName,
                     &missingFrameDefault, &errorMenu ),
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


void
GusdSOP_usdimport::Register(OP_OperatorTable* table)
{
    OP_Operator* op =
        new OP_Operator("pixar::usdimport",
                        "USD Import",
                        Create,
                        *_mainTemplates,
                        /* min inputs */ 0,
                        /* max inputs */ 1,
                       /* variables  */ 0,
                        OP_FLAG_GENERATOR);
    op->setIconName("pxh_gusdIcon.png");
    op->setOpTabSubMenuPath( "Pixar" );
    table->addOperator(op);
    table->setOpFirstName( "pixar::usdimport", "usdimport" );
}


OP_Node*
GusdSOP_usdimport::Create(OP_Network* net, const char* name, OP_Operator* op)
{
    return new GusdSOP_usdimport(net, name, op);
}


GusdSOP_usdimport::GusdSOP_usdimport(
    OP_Network* net, const char* name, OP_Operator* op)
  : SOP_Node(net, name, op), _group(NULL)
{}


bool
GusdSOP_usdimport::updateParmsFlags()
{
    bool haveInput = getInput(0);
    
    return enableParm("import_group", haveInput) +
           enableParm("import_delold", haveInput) +
           enableParm("import_file", !haveInput) +
           enableParm("import_primpath", !haveInput);
}


void
GusdSOP_usdimport::UpdateTraversalParms()
{
    if(getIsChangingSpareParms())
        return;

    UT_String traversal;
    evalString(traversal, "import_traversal", 0, 0);

    const auto& table = GusdUSD_TraverseTable::GetInstance();

    const PRM_Template* customTemplates = NULL;
    if(traversal != _NOTRAVERSE_NAME) {
        if(const auto* type = table.Find(traversal))
            customTemplates = type->GetTemplates();
    }

    _templates.clear();
    const int nCustom = customTemplates ?
        PRM_Template::countTemplates(customTemplates) : 0;
    if(nCustom > 0) {
        /* Build a template list that puts the main
           templates in one tab, and the custom templates in another.*/
        static const int nMainTemplates =
            PRM_Template::countTemplates(*_mainTemplates);

        _tabs[0] = PRM_Default(nMainTemplates, "Main");
        _tabs[1] = PRM_Default(nCustom, "Advanced");

        static PRM_Name tabsName("import_tabs", "");
        
        _templates.append(PRM_Template(PRM_SWITCHER, 2, &tabsName, _tabs));
        
        _ConcatTemplates(_templates, *_mainTemplates);
        _ConcatTemplates(_templates, customTemplates);
    }
    _templates.append(PRM_Template());
                       

    /* Add the custom templates as spare parms.*/
    PI_EditScriptedParms parms(this, &_templates(0), /*spare*/ true,
                               /*skip-reserved*/ false, /*init links*/ false);
    UT_String errs;
    GusdUTverify_ptr(OPgetDirector())->changeNodeSpareParms(this, parms, errs);
    
    _AddTraversalParmDependencies();
}


void
GusdSOP_usdimport::_AddTraversalParmDependencies()
{
    PRM_ParmList* parms = GusdUTverify_ptr(getParmList());
    for(int i = 0; i < parms->getEntries(); ++i) {
        PRM_Parm* parm = GusdUTverify_ptr(parms->getParmPtr(i));
        if(parm->isSpareParm()) {
            for(int j = 0; j < parm->getVectorSize(); ++j)
                addExtraInput(parm->microNode(j));
        }
    }
}

void
GusdSOP_usdimport::Reload()
{
    UT_String file;
    const fpreal t = CHgetEvalTime();
    evalString(file, "import_file", 0, t);
    if(!file.isstring()) {
        return;
    }

    UT_StringSet paths;
    paths.insert(file);
    GusdOP_Utils::ReloadStagesAndClearCaches(paths);
    forceRecook();
}

OP_ERROR
GusdSOP_usdimport::_Cook(OP_Context& ctx)
{
    fpreal t = ctx.getTime();

    UT_String traversal;
    evalString( traversal, "import_traversal", 0, t );

    ErrorChoice errorMode = static_cast<ErrorChoice>(evalInt("missingframe", 0, t ));

    UT_ErrorSeverity errorSev =
        errorMode == MISSINGFRAME_WARN ? UT_ERROR_WARNING : UT_ERROR_ABORT;

    const GusdUSD_Traverse* trav = NULL;
    if(traversal != _NOTRAVERSE_NAME) {
        const auto& table = GusdUSD_TraverseTable::GetInstance();
        trav = table.FindTraversal(traversal);
        
        if(!trav) {
            GUSD_ERR().Msg("Failed locating traversal '%s'", traversal.c_str());
            return error();
        }
    }
    return getInput(0) ? _ExpandPrims(ctx, trav, errorSev)
                       : _CreateNewPrims(ctx, trav, errorSev);
}


namespace {


GusdStageBasicEdit*
_CreateEdit(const SdfPath& variantSelPath)
{
    if(!variantSelPath.IsEmpty()) {
        GusdStageBasicEdit* edit = new GusdStageBasicEdit();
        edit->GetVariants().append(variantSelPath);
        return edit;
    }
    return nullptr;
}


void
_GetStageEditsForVariants(const UT_Array<SdfPath>& variants,
                          GusdDefaultArray<GusdStageEditPtr>& edits)
{
    if(variants.size() > 0) {
        // Check if there are any variant selections, and if so, if they all match.
        bool varying = false;
        SdfPath firstVariant = variants(0);
        for(exint i = 1; i < variants.size(); ++i) {
            if(variants(i) != firstVariant) {
                varying = true;
                break;
            }
        }

        if(varying) {
            edits.GetArray().setSize(variants.size());
            for(exint i = 0; i < variants.size(); ++i) {
                edits.GetArray()(i).reset(_CreateEdit(variants(i)));
            }
        } else {
            edits.Clear();
            edits.GetDefault().reset(_CreateEdit(firstVariant));
        }
    }
}


} // namespace


OP_ERROR
GusdSOP_usdimport::_CreateNewPrims(OP_Context& ctx,
                                   const GusdUSD_Traverse* traverse,
                                   UT_ErrorSeverity sev)
{
    fpreal t = ctx.getTime();

    UT_String file, primPath;
    evalString(file, "import_file", 0, t);
    evalString(primPath, "import_primpath", 0, t);
    if(!file.isstring() || !primPath.isstring()) {
        // Nothing to do.
        return UT_ERROR_NONE;
    }

    /* The prim path may be a list of prims.
       Additionally, those prim paths may include variants
       (eg., /some/model{variant=sel}/subscope ).
       Including multiple variants may mean that we need to access
       multiple stages. 

       Resolve the actual set of prims and variants first.*/

    UT_Array<SdfPath> primPaths, variants;
    if(!GusdUSD_Utils::GetPrimAndVariantPathsFromPathList(
           primPath, primPaths, variants, sev))
        return error();

    GusdDefaultArray<UT_StringHolder> filePaths;
    filePaths.SetConstant(file);

    // Get stage edits applying any of our variants.
    GusdDefaultArray<GusdStageEditPtr> edits;
    _GetStageEditsForVariants(variants, edits);
    
    // Load the root prims.
    UT_Array<UsdPrim> rootPrims;
    {
        rootPrims.setSize(primPaths.size());

        GusdStageCacheReader cache;
        if(!cache.GetPrims(filePaths, primPaths, edits,
                           rootPrims.data(),
                           GusdStageOpts::LoadAll(), sev))
            return error();
    }

    GusdDefaultArray<UsdTimeCode> times;
    times.SetConstant(evalFloat("import_time", 0, t));

    UT_String purpose;
    evalString(purpose, "purpose", 0, t);

    GusdDefaultArray<GusdPurposeSet> purposes;
    purposes.SetConstant(GusdPurposeSet(
                             GusdPurposeSetFromMask(purpose)|
                             GUSD_PURPOSE_DEFAULT));

    UT_Array<UsdPrim> prims;
    if(traverse) {
        // Before traversal, make a copy of the variants list.
        const UT_Array<SdfPath> variantsPreTraverse(variants);

        UT_Array<GusdUSD_Traverse::PrimIndexPair> primIndexPairs;

        UT_UniquePtr<GusdUSD_Traverse::Opts> opts(traverse->CreateOpts());
        if(opts) {
            if(!opts->Configure(*this, t))
                return error();
        }

        if(!traverse->FindPrims(rootPrims, times, purposes, primIndexPairs,
                                /*skip root*/ false, opts.get())) {
            return error();
        }

        // Resize the prims and variants lists to match the size of
        // primIndexPairs.
        exint size = primIndexPairs.size();
        prims.setSize(size);
        variants.setSize(size);

        // Now iterate through primIndexPairs to populate the prims
        // and variants lists.
        for (exint i = 0; i < size; i++) {
            prims(i) = primIndexPairs(i).first;
            exint index = primIndexPairs(i).second;

            variants(i) = index < variantsPreTraverse.size() ?
                          variantsPreTraverse(index) : SdfPath();
        }

    } else {
        std::swap(prims, rootPrims);
    }

    /* Have the resolved set of USD prims.
       Now create prims or points on the detail.*/

    bool packedPrims = !evalInt("import_class", 0, t);
    if(packedPrims) {
        UT_String vpLOD;
        evalString(vpLOD, "viewportlod", 0, t);


        GusdDefaultArray<UT_StringHolder> lods;
        lods.SetConstant(vpLOD);

        GusdGU_USD::AppendPackedPrims(*gdp, prims, variants,
                                      times, lods, purposes);
    } else {
        GusdGU_USD::AppendRefPoints(*gdp, prims, GUSD_PATH_ATTR,
                                    GUSD_PRIMPATH_ATTR);
    }
    return error();
}

OP_ERROR
GusdSOP_usdimport::_ExpandPrims(OP_Context& ctx,
                                const GusdUSD_Traverse* traverse,
                                UT_ErrorSeverity sev)
{
    if(!traverse)
        return UT_ERROR_NONE; // Nothing to do!

    fpreal t = ctx.getTime();

    // Construt a range and bind prims.
    bool packedPrims = !evalInt("import_class", 0, t);
    GA_AttributeOwner owner = packedPrims ?
        GA_ATTRIB_PRIMITIVE : GA_ATTRIB_POINT;
    GA_Range rng(gdp->getIndexMap(owner),
                 UTverify_cast<const GA_ElementGroup*>(_group));

    UT_Array<UsdPrim> rootPrims;
    GusdDefaultArray<UsdTimeCode> times;
    GusdDefaultArray<GusdPurposeSet> purposes;
    {
        GusdStageCacheReader cache;
        if(!GusdGU_USD::BindPrims(cache, rootPrims, *gdp, rng,
                                  /*variants*/ nullptr,
                                  &purposes, &times, sev)) {
            return error();
        }
    }
    if(!times.IsVarying())
        times.SetConstant(evalFloat("import_time", 0, t));

    // Traverse to find a new prim selection.
    UT_Array<GusdUSD_Traverse::PrimIndexPair> expandedPrims;
    {
        UT_UniquePtr<GusdUSD_Traverse::Opts> opts(traverse->CreateOpts());
        if(opts) {
            if(!opts->Configure(*this, t))
                return error();
        }

        if(!traverse->FindPrims(rootPrims, times, purposes, expandedPrims,
                                /*skip root*/ true, opts.get()))
            return error();
    }

    GA_AttributeFilter filter(GA_AttributeFilter::selectPublic());
    GusdGU_USD::AppendExpandedRefPoints(
        *gdp, *gdp, rng, expandedPrims, filter,
        GUSD_PATH_ATTR, GUSD_PRIMPATH_ATTR);

    if(evalInt("import_delold", 0, t)) {
        if(packedPrims)
            gdp->destroyPrimitives(rng, /*and points*/ true);
        else
            gdp->destroyPoints(rng); // , GA_DESTROY_DEGENERATE);
    }
    return error();
}
                               

OP_ERROR
GusdSOP_usdimport::cookInputGroups(OP_Context& ctx, int alone)
{
    if(!getInput(0))
        return UT_ERROR_NONE;

    int groupIdx = getParmList()->getParmIndex("import_group");
    int classIdx = getParmList()->getParmIndex("import_class");
    bool packedPrims = !evalInt(classIdx, 0, ctx.getTime());
    
    GA_GroupType groupType = packedPrims ?  
        GA_GROUP_PRIMITIVE : GA_GROUP_POINT;

    return cookInputAllGroups(ctx, _group, alone,
                              /* do selection*/ true,
                              groupIdx, classIdx, groupType);
}


OP_ERROR
GusdSOP_usdimport::cookMySop(OP_Context& ctx)
{
    OP_AutoLockInputs lock(this);
    if(lock.lock(ctx) >= UT_ERROR_ABORT)
        return error();

    // Local var support.
    setCurGdh(0, myGdpHandle);
    setupLocalVars();

    if(getInput(0))
        duplicateSource(0, ctx);
    else
        gdp->clearAndDestroy();

    /* Extra inputs have to be re-added on each cook.*/
    _AddTraversalParmDependencies();

    if(cookInputGroups(ctx, 0) < UT_ERROR_ABORT)
        _Cook(ctx);
        
    resetLocalVarRefs();

    return error();
}


void
GusdSOP_usdimport::finishedLoadingNetwork(bool isChildCall)
{
    SOP_Node::finishedLoadingNetwork(isChildCall);
    
    if(isChildCall) {
        /* Update our traversal parms.
           Needs to happen post-loading since loading could
           have changed the traversal mode.*/
        UpdateTraversalParms();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
