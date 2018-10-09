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
#include "SOP_usdunpack.h"

#include "gusd/GU_USD.h"
#include "gusd/PRM_Shared.h"
#include "gusd/USD_Traverse.h"
#include "gusd/USD_Utils.h"
#include "gusd/UT_Assert.h"
#include "gusd/UT_StaticInit.h"

#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/fileUtils.h>

#include <GA/GA_AttributeFilter.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Director.h>
#include <OP/OP_OperatorTable.h>
#include <PI/PI_EditScriptedParms.h>
#include <PRM/PRM_AutoDeleter.h>
#include <PRM/PRM_Conditional.h>
#include <UT/UT_WorkArgs.h>
#include <UT/UT_UniquePtr.h>
#include <PY/PY_Python.h>

PXR_NAMESPACE_OPEN_SCOPE

using std::cerr;
using std::endl;
using std::string;

namespace {

#define _NOTRAVERSE_NAME "none"
#define _GPRIMTRAVERSE_NAME "std:boundables"

int _TraversalChangedCB(void* data, int idx, fpreal64 t,
                        const PRM_Template* tmpl)
{
    auto& sop = *reinterpret_cast<GusdSOP_usdunpack*>(data);
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
    for(const auto& pair : table) {
        names.append(pair.second->GetName());
    }
    
    names.stdsort(
        [](const PRM_Name& a, const PRM_Name& b)    
        { return UT_String(a.getLabel()) < UT_String(b.getLabel()); });
    names.append(PRM_Name());

    static PRM_ChoiceList menu(PRM_CHOICELIST_SINGLE, &names(0));
    return menu;
}

PRM_Template*   _CreateTemplates()
{
    //const auto& table = GusdUSD_TraverseTable::GetInstance();

    /* XXX: All names should be prefixed to ensure they don't
            collide with the templates of the traversal plugins.*/

    static PRM_Name className("unpack_class", "Class");
    static PRM_Name groupName("unpack_group", "Group");

    static PRM_Name traversalName("unpack_traversal", "Traversal");
    static PRM_Default traversalDef(0, _GPRIMTRAVERSE_NAME);

    static PRM_Name geomTypeName("unpack_geomtype", "Geometry Type");
    static PRM_Name geomTypeChoices[] =
    {
        PRM_Name("packedprims", "Packed Prims"),
        PRM_Name("polygons", "Polygons"),
        PRM_Name(0)
    };
    static PRM_ChoiceList geomTypeMenu(PRM_CHOICELIST_SINGLE, geomTypeChoices);


    static PRM_Name deloldName("unpack_delold", "Delete Old Points/Prims");

    static PRM_Name timeName("unpack_time", "Time");
    static PRM_Default timeDef(0, "$RFSTART");
    static PRM_Conditional
            disableWhenNotPoints("{ unpack_class != \"point\" }");

    static PRM_Name attrsHeadingName("attrs_heading", "Attributes");

    static PRM_Name attrsName("transfer_attrs", "Transfer Attributes");
    static const char* attrsHelp = "Specifies a list of attributes to "
        "transfer from the input prims to the result geometry.";

    static PRM_Name primvarsName("import_primvars", "Import Primvars");
    static PRM_Default primvarsCdDef(0, "Cd");
    static const char* primvarsHelp = "Specifies a list of primvars to "
        "import from the traversed USD prims.";

    static PRM_Conditional
            disableWhenNotPolygons("{ unpack_geomtype != \"polygons\" }");


    GusdPRM_Shared shared;

    static PRM_Template templates[] = {
        PRM_Template(PRM_STRING, 1, &groupName, /*default*/ 0),
        PRM_Template(PRM_ORD, 1, &className, /*default*/ 0,
                     &PRMentityMenuPointsAndPrimitives),
        PRM_Template(PRM_TOGGLE, 1, &deloldName, PRMoneDefaults),

        PRM_Template(PRM_FLT, 1, &timeName, &timeDef,
                     // choicelist, range, callback, spare, group, help
                     0, 0, 0, 0, 0, 0,
                     &disableWhenNotPoints),
        PRM_Template(PRM_ORD, 1, &traversalName,
                     &traversalDef, &_CreateTraversalMenu(),
                     0, // range
                     _TraversalChangedCB),
        PRM_Template(PRM_ORD, 1, &geomTypeName, 0, &geomTypeMenu),

        PRM_Template(PRM_HEADING, 1, &attrsHeadingName, 0),
        PRM_Template(PRM_STRING, 1, &attrsName, 0,
                     // choicelist, range, callback, spare, group, help
                     0, 0, 0, 0, 0, attrsHelp),

        PRM_Template(PRM_STRING, 1, &primvarsName, &primvarsCdDef,
                     // choicelist, range, callback, spare, group, help
                     0, 0, 0, 0, 0, primvarsHelp,
                     &disableWhenNotPolygons),
        PRM_Template()
    };

    return templates;
}


auto _mainTemplates(GusdUT_StaticVal(_CreateTemplates));


} /*namespace*/


void
GusdSOP_usdunpack::Register(OP_OperatorTable* table)
{
    OP_Operator* op =
        new OP_Operator("pixar::usdunpack",
                        "USD Unpack",
                        Create,
                        *_mainTemplates,
                        /* min inputs */ (unsigned int)0,
                        /* max input  */ (unsigned int)1);
    op->setIconName("pxh_gusdIcon.png");
    op->setOpTabSubMenuPath( "Pixar" );
    table->addOperator(op);
    table->setOpFirstName( "pixar::usdunpack", "usdunpack" );
}


OP_Node*
GusdSOP_usdunpack::Create(OP_Network* net, const char* name, OP_Operator* op)
{
    return new GusdSOP_usdunpack(net, name, op);
}


GusdSOP_usdunpack::GusdSOP_usdunpack(
    OP_Network* net, const char* name, OP_Operator* op)
  : SOP_Node(net, name, op), _group(NULL)
{}


void
GusdSOP_usdunpack::UpdateTraversalParms()
{
    if(getIsChangingSpareParms())
        return;

    UT_String traversal;
    evalString(traversal, "unpack_traversal", 0, 0);

    const auto& table = GusdUSD_TraverseTable::GetInstance();

    const PRM_Template* customTemplates = NULL;
    if(traversal != _NOTRAVERSE_NAME) {
        if(const auto* type = table.Find(traversal)) {
            customTemplates = type->GetTemplates();
        }
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

        static PRM_Name tabsName("unpack_tabs", "");
        
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
GusdSOP_usdunpack::_AddTraversalParmDependencies()
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

template<typename T>
void RemapArray(const UT_Array<GusdUSD_Traverse::PrimIndexPair>& pairs,
                const UT_Array<T>& srcArray,
                const T& defaultValue,
                UT_Array<T>& dstArray)
{
    const exint size = pairs.size();
    dstArray.setSize(size);
    for (exint i = 0; i < size; ++i) {
        const exint index = pairs(i).second;
        dstArray(i) = (index >= 0 && index < size) ?
                      srcArray(index) : defaultValue;
    }
}

OP_ERROR
GusdSOP_usdunpack::_Cook(OP_Context& ctx)
{
    fpreal t = ctx.getTime();

    UT_String traversal;
    evalString(traversal, "unpack_traversal", 0, t);

    UT_String geomType;
    evalString(geomType, "unpack_geomtype", 0, t);
    bool unpackToPolygons = (geomType == "polygons");

    bool packedPrims = !evalInt("unpack_class", 0, ctx.getTime());

    // If there is no traversal AND geometry type is not
    // polygons, then the output prims would be the same as the inputs,
    // so nothing left to do.
    if (traversal == _NOTRAVERSE_NAME && !unpackToPolygons) {
        return UT_ERROR_NONE;
    }

    GA_AttributeOwner owner = packedPrims ?
        GA_ATTRIB_PRIMITIVE : GA_ATTRIB_POINT;

    // Construct a range and bind prims.
    GA_Range rng(gdp->getIndexMap(owner),
                 UTverify_cast<const GA_ElementGroup*>(_group));

    UT_Array<SdfPath> variants;
    GusdDefaultArray<GusdPurposeSet> purposes;
    GusdDefaultArray<UsdTimeCode> times;
    UT_Array<UsdPrim> rootPrims;
    {
        GusdStageCacheReader cache;
        if(!GusdGU_USD::BindPrims(cache, rootPrims, *gdp, rng,
                                  &variants, &purposes, &times)) {
            return error();
        }
    }

    if(!times.IsVarying())
        times.SetConstant(evalFloat("unpack_time", 0, t));

    // Run the traversal and store the resulting prims in traversedPrims.
    // If unpacking to polygons, the traversedPrims will need to contain
    // gprim level prims, which means a second traversal may be required.

    UT_Array<GusdUSD_Traverse::PrimIndexPair> traversedPrims;
    if (traversal != _NOTRAVERSE_NAME) {
        // For all traversals except gprim level, skipRoot must be true to
        // get the correct results. For gprim level traversals, skipRoot
        // should be false so the results won't be empty.
        bool skipRoot = (traversal != _GPRIMTRAVERSE_NAME);
        if (!_Traverse(traversal, t, rootPrims, times, purposes,
                       skipRoot, traversedPrims)) {
            return error();
        }
    } else if (unpackToPolygons) {
        // There is no traversal specified, but unpackToPolygons is true.
        // A second traversal will be done upon traversedPrims to make
        // sure it contains gprim level prims, but for now, just copy the
        // original packed prims from primHnd into traversedPrims.
        const exint size = rootPrims.size();
        traversedPrims.setSize(size);
        for (exint i = 0; i < size; ++i) {
            traversedPrims(i) = std::make_pair(rootPrims(i), i);
        }
    }

    // If unpacking to polygons AND the traversal was anything other than
    // gprim level, we need to traverse again to get down to the gprim
    // level prims.
    if (unpackToPolygons && traversal != _GPRIMTRAVERSE_NAME) {
        const exint size = traversedPrims.size();

        // Split up the traversedPrims pairs into 2 arrays.
        UT_Array<UsdPrim> prims(size, size);
        UT_Array<exint> indices(size, size);
        for (exint i = 0; i < size; ++i) {
            prims(i) = traversedPrims(i).first;
            indices(i) = traversedPrims(i).second;
        }

        GusdDefaultArray<GusdPurposeSet>    
            traversedPurposes(purposes.GetDefault());
        if(purposes.IsVarying()) {
            // Purposes must be remapped to align with traversedPrims.
            RemapArray(traversedPrims, purposes.GetArray(),
                       GUSD_PURPOSE_DEFAULT, traversedPurposes.GetArray());
        }

        GusdDefaultArray<UsdTimeCode> traversedTimes(times.GetDefault());
        if(times.IsVarying()) {
            // Times must be remapped to align with traversedPrims.
            RemapArray(traversedPrims, times.GetArray(),
                       times.GetDefault(), traversedTimes.GetArray());
        }

        // Clear out traversedPrims so it can be re-populated
        // during the new traversal.
        traversedPrims.clear();

        // skipRoot should be false so the result won't be empty.
        bool skipRoot = false;
        if (!_Traverse(_GPRIMTRAVERSE_NAME, t, prims,
                       traversedTimes, traversedPurposes,
                       skipRoot, traversedPrims)) {
            return error();
        }

        // Each index in the traversedPrims pairs needs
        // to be remapped back to a prim in primHnd.
        for (exint i = 0; i < traversedPrims.size(); ++i) {
            const exint primsIndex = traversedPrims(i).second;
            traversedPrims(i).second = indices(primsIndex);
        }
    }

    // Build an attribute filter using the transfer_attrs parameter.
    UT_String transferAttrs;
    evalString(transferAttrs, "transfer_attrs", 0, t);

    GA_AttributeFilter filter(
        GA_AttributeFilter::selectAnd(
            GA_AttributeFilter::selectByPattern(transferAttrs.c_str()),
            GA_AttributeFilter::selectPublic()));

    if (!packedPrims) {
        GusdGU_USD::AppendExpandedRefPoints(
            *gdp, *gdp, rng, traversedPrims, filter,
            GUSD_PATH_ATTR, GUSD_PRIMPATH_ATTR);

    } else {
        // The variants array needs to be expanded to
        // align with traversedPrims.
        UT_Array<SdfPath> expandedVariants;
        RemapArray(traversedPrims, variants,
                   SdfPath::EmptyPath(), expandedVariants);

        GusdDefaultArray<UsdTimeCode> traversedTimes(times.GetDefault());
        if(times.IsVarying()) {
            // Times must be remapped to align with traversedPrims.
            RemapArray(traversedPrims, times.GetArray(),
                       times.GetDefault(), traversedTimes.GetArray());
        }

        UT_String importPrimvars;
        evalString(importPrimvars, "import_primvars", 0, t);

        GusdGU_USD::AppendExpandedPackedPrims(
            *gdp, *gdp, rng, traversedPrims, expandedVariants, traversedTimes,
            filter, unpackToPolygons, importPrimvars);
    }

    if(evalInt("unpack_delold", 0, t)) {

        // Only delete prims or points that were successfully
        // binded to prims in primHnd.
        GA_OffsetList delOffsets;
        delOffsets.reserve(rootPrims.size());
        exint i = 0;
        for (GA_Iterator it(rng); !it.atEnd(); ++it, ++i) {
            if (rootPrims(i).IsValid()) {
                delOffsets.append(*it);
            }
        }
        GA_Range delRng(gdp->getIndexMap(owner), delOffsets);

        if(packedPrims)
            gdp->destroyPrimitives(delRng, /*and points*/ true);
        else
            gdp->destroyPoints(delRng); // , GA_DESTROY_DEGENERATE);
    }

    return error();
}

bool
GusdSOP_usdunpack::_Traverse(const UT_String& traversal,
                             const fpreal time,
                             const UT_Array<UsdPrim>& prims,
                             const GusdDefaultArray<UsdTimeCode>& times,
                             const GusdDefaultArray<GusdPurposeSet>& purposes,
                             bool skipRoot,
                             UT_Array<GusdUSD_Traverse::PrimIndexPair>& traversed)
{
    const auto& table = GusdUSD_TraverseTable::GetInstance();
    
    const GusdUSD_Traverse* traverse = table.FindTraversal(traversal);
    if (!traverse) {
        GUSD_ERR().Msg("Failed locating traversal '%s'", traversal.c_str());
        return false;
    }

    UT_UniquePtr<GusdUSD_Traverse::Opts> opts(traverse->CreateOpts());
    if (opts) {
        if (!opts->Configure(*this, time)) {
            return false;
        }
    }

    if (!traverse->FindPrims(prims, times, purposes, traversed,
                             skipRoot, opts.get())) {
        return false;
    }

    return true;
}
                               

OP_ERROR
GusdSOP_usdunpack::cookInputGroups(OP_Context& ctx, int alone)
{
    if(!getInput(0))
        return UT_ERROR_NONE;

    int groupIdx = getParmList()->getParmIndex("unpack_group");
    int classIdx = getParmList()->getParmIndex("unpack_class");
    bool packedPrims = !evalInt(classIdx, 0, ctx.getTime());
    
    GA_GroupType groupType = packedPrims ?  
        GA_GROUP_PRIMITIVE : GA_GROUP_POINT;

    return cookInputAllGroups(ctx, _group, alone,
                              /* do selection*/ true,
                              groupIdx, classIdx, groupType);
}


OP_ERROR
GusdSOP_usdunpack::cookMySop(OP_Context& ctx)
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
GusdSOP_usdunpack::finishedLoadingNetwork(bool isChildCall)
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

