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
#include "ROP_usdoutput.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/nullPtr.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/fileUtils.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdUtils/pipeline.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/primSpec.h"

#include <CH/CH_Manager.h>
#include <GA/GA_Range.h>
#include <GT/GT_GEODetail.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GT/GT_Refine.h>
#include <GT/GT_Util.h>
#include <GT/GT_PrimInstance.h>
#include <GU/GU_Detail.h>
#include <GEO/GEO_AttributeHandle.h>
#include <OBJ/OBJ_Node.h>
#include <OP/OP_Node.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_ChoiceList.h>
#include <PRM/PRM_Conditional.h>
#include <PRM/PRM_Default.h>
#include <PRM/PRM_SpareData.h>
#include <ROP/ROP_Error.h>
#include <ROP/ROP_Templates.h>
#include <SOP/SOP_Node.h>
#include <UT/UT_Array.h>
#include <UT/UT_Set.h>
#include <VOP/VOP_Node.h>

#include "gusd/gusd.h"
#include "gusd/primWrapper.h"
#include "gusd/refiner.h"
#include "gusd/stageCache.h"
#include "gusd/shaderWrapper.h"
#include "gusd/UT_Error.h"
#include "gusd/UT_Gf.h"
#include "gusd/UT_Version.h"
#include "gusd/context.h"

#include "boost/foreach.hpp"

PXR_NAMESPACE_OPEN_SCOPE

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::map;
using std::pair;
using std::vector;

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

namespace {

void setKind( const string &path, UsdStagePtr stage );
string getStringUniformOrDetailAttribute( 
        GT_PrimitiveHandle prim, 
        const char* attrName );
bool setCamerasAreZup(UsdStageWeakPtr const &stage, bool isZup);

template<typename ShaderT>
void addShaderToMap(const ShaderT& shader, const SdfPath& primPath,
                    UT_Map<ShaderT, std::vector<SdfPath> >& map);

OP_Node* 
creator(OP_Network* network,
        const char* name,
        OP_Operator* op) 
{
    return new GusdROP_usdoutput(network, name, op);
}

//------------------------------------------------------------------------------
// paramters
//------------------------------------------------------------------------------
OP_TemplatePair*
getTemplates()
{
    static PRM_Name sopPathName("soppath", "SOP Path");

    static PRM_Name    usdFileName("usdfile", "USD File");
    static PRM_Default usdFileDefault(0, "$HIP/out.usd");

    static PRM_Name  granularityChoiceNames[] = {
        PRM_Name( "oneFile", "One File" ),
        PRM_Name( "perFrame", "Per Frame Files" ),
        PRM_Name()
    };

    static PRM_ChoiceList granularityMenu( PRM_CHOICELIST_SINGLE, 
                                           granularityChoiceNames );

    static PRM_Name        granularityName("granularity","Granularity");

    static PRM_Name        pathsHeadingName("pathsheading", "Paths");
    static PRM_Name        pathPrefixName("pathprefix", "Prefix");
    static PRM_Default     pathPrefixDefault(0, "/FxAsset");
    static PRM_Name        enablePathAttrName("enablepathattrib", "");
    static PRM_Name        pathAttrName("pathattrib", "Path attribute");
    static PRM_Default     pathAttrDefault(0, "usdprimpath");
    static PRM_Conditional pathAttrConditional("{ enablepathattrib == 0 }");
    static PRM_Name        alwaysWriteRootName( "alwayswriteroot", "Always Write Root Prim" );

    static PRM_Name primvarHeadingName("primvarheading", "Primvars");
    static PRM_Name varyingPrimvarsName("varyingprimvars", "Varying");
    static PRM_Name faceVaryingPrimvarsName("facevaryingprimvars", "Facevarying");
    static PRM_Name uniformPrimvarsName("uniformprimvars", "Uniform");
    static PRM_Name constantPrimvarsName("constantprimvars", "Constant");
    
    static PRM_Name shaderHeadingName("shaderheading", "Shaders");
    static PRM_Name enableShadersName("enableshaders", "Output Shaders");
    static PRM_Name usdShadingFileName("usdshadingfile", "USD Shading File");
    static PRM_Name usdShaderName("usdshader", "USD Shader");
    static PRM_Name shaderOutDirName("shaderoutdir", "Shader Output Dir");
    static PRM_Conditional shaderOutConditional("{ enableshaders == 0 }");

    static PRM_Name scriptsHeadingName("scriptsheading", "Scripts");
    static PRM_Name pxhPreRenderName("pxhprerenderscript", "Pxh Pre-Render Script");

    static PRM_Name geometryHeadingName("geometryheading", "Geometry");
    static PRM_Name instanceRefsName("usdinstancing","Enable USD Instancing");
    static PRM_Name authorVariantSelName("authorvariantselection", "Author Variant Selections");

    static PRM_Name overlayHeadingName( "overlayheading", "Overlay");
    static PRM_Name overlayName("overlay", "Overlay Existing Geometry");
    static PRM_Conditional overlayConditional("{ overlay == 0 }");

    static PRM_Name referenceFileName( "referencefile", "Overlay Reference File" );
    static PRM_Name overlayAllName( "overlayall", "Overlay All" );
    static PRM_Default overlayAllDefault( 1 );
    static PRM_Name overlayXformsName( "overlayxforms", "Overlay Transforms" );
    static PRM_Name overlayPointsName( "overlaypoints", "Overlay Points" );
    static PRM_Name overlayPrimvarsName( "overlayprimvars", "Overlay Primvars" );

    static PRM_Name tinaProgressScriptName( "tinaprogressscript", "Tina Progress Script" );

    static PRM_Template templates[] = {
        PRM_Template(
            PRM_STRING_OPREF,
            PRM_TYPE_DYNAMIC_PATH,
            1,
            &sopPathName,
            0, // default
            0, // choice
            0, // range
            0, // callback
            &PRM_SpareData::sopPath,
            0, // paramgroup (leave default)
            "SOP to export", // help string
            0), // disable rules

        PRM_Template(
            PRM_FILE,
            1,
            &usdFileName,
            &usdFileDefault,
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "USD file to write to", // help string
            0), // disable rules

        PRM_Template(
            PRM_ORD,
            1,
            &granularityName,
            0,
            &granularityMenu ),

        PRM_Template(
            PRM_HEADING,
            1,
            &pathsHeadingName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            0, // help string
            0), // disable rules

        PRM_Template(
            PRM_STRING,
            1,
            &pathPrefixName,
            &pathPrefixDefault,
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "A prefix to the locations objects will be written to in the USD file. "
            "This prefix will be ignored if using a path attribute.",
            0), // disable rules

        PRM_Template(
            PRM_TOGGLE,
            PRM_TYPE_TOGGLE_JOIN,
            1,
            &enablePathAttrName,
            PRMoneDefaults, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            0, // help string
            0), // disable rules

        PRM_Template(
            PRM_STRING,
            1,
            &pathAttrName,
            &pathAttrDefault,
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Primitive attribute which specifies a path for each gprim. "
            "If this attribute exists for a prim them that prim will be written "
            "to the location in the USD file contained in the attribute. For overlays, "
            "objects are imported with a location in the attribute, modified and "
            "them written back out to the right location using that attribute. "
            "The path prefix is ignored when this attribute exists.",
            &pathAttrConditional),
  
        PRM_Template(
            PRM_TOGGLE,
            1,
            &alwaysWriteRootName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "If the file would otherwise be empty, write an empty group prim at "
            "the location specified in the the prefix parm.",
            0), // disable rules

        PRM_Template(
            PRM_HEADING,
            1,
            &geometryHeadingName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            0, // help string
            0), // disable rules

        PRM_Template(
            PRM_TOGGLE,
            1,
            &instanceRefsName,
            PRMzeroDefaults, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Make references to USD primitives instanceable.",
            0), // disable rules

        PRM_Template(
            PRM_TOGGLE,
            1,
            &authorVariantSelName,
            PRMzeroDefaults, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Write variant selections with USD packed prims if a variant was "
            "explicity set when the packed prim was created. This is useful "
            "when writing prototypes for point instancers.",
            0), // disable rules

        PRM_Template(
            PRM_HEADING,
            1,
            &overlayHeadingName,
            0,
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            0,
            0),

        PRM_Template(
            PRM_TOGGLE,
            1,
            &overlayName,
            0,
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Write USD file that modifies an existing file",
            0),

        PRM_Template(
            PRM_FILE,
            1,
            &referenceFileName,
            0,
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "USD file to be modified by overlaying changes",
            &overlayConditional),
 
        PRM_Template(
            PRM_TOGGLE,
            1,
            &overlayAllName,
            &overlayAllDefault,
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Export transforms, points, primvars and topology for each object",
            &overlayConditional),

        PRM_Template(
            PRM_TOGGLE,
            1,
            &overlayXformsName,
            0,
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Export only transforms for each object",
            &overlayConditional),
 
        PRM_Template(
            PRM_TOGGLE,
            1,
            &overlayPointsName,
            0,
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Export only points for each object",
            &overlayConditional),

        PRM_Template(
            PRM_TOGGLE,
            1,
            &overlayPrimvarsName,
            0,
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Export only primvars for each object",
            &overlayConditional),       

        PRM_Template(
            PRM_HEADING,
            1,
            &primvarHeadingName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            0, // help string
            0), // disable rules

        PRM_Template(
            PRM_STRING,
            1,
            &varyingPrimvarsName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Varying primvar exports", // help string
            0), // disable rules

        PRM_Template(
            PRM_STRING,
            1,
            &faceVaryingPrimvarsName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Facevarying primvar exports", // help string
            0), // disable rules

        PRM_Template(
            PRM_STRING,
            1,
            &uniformPrimvarsName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Uniform primvar exports", // help string
            0), // disable rules

        PRM_Template(
            PRM_STRING,
            1,
            &constantPrimvarsName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Constant primvar exports", // help string
            0), // disable rules

        PRM_Template(
            PRM_HEADING,
            1,
            &shaderHeadingName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            0, // help string
            0), // disable rules

        PRM_Template(
            PRM_STRING,
            1,
            &usdShadingFileName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Absolute path to USD shading file", // help string
            0), // disable rules

        PRM_Template(
            PRM_STRING,
            1,
            &usdShaderName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "USD Shader name", // help string
            0), // disable rules

        PRM_Template(
            PRM_TOGGLE,
            1,
            &enableShadersName,
            PRMzeroDefaults,
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Enable houdini materials to be "
            "converted into usd shaders.", // help string
            0), // disable rules

        PRM_Template(
            PRM_STRING,
            1,
            &shaderOutDirName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Directory where shaders built from "
            "houdini materials will go.", // help string
            &shaderOutConditional), // disable rules

        PRM_Template(
            PRM_HEADING,
            1,
            &scriptsHeadingName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            0, // help string
            0), // disable rules

        PRM_Template( 
            PRM_STRING,
            1,
            &pxhPreRenderName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Python script to execute before any USD file is written. "
            "Similar to prerender, but more reliable."
            ),
        
        // predefined render script templates from ROP_Templates.h
        theRopTemplates[ROP_TPRERENDER_TPLATE],
        theRopTemplates[ROP_PRERENDER_TPLATE],
        theRopTemplates[ROP_LPRERENDER_TPLATE],
        theRopTemplates[ROP_TPREFRAME_TPLATE],
        theRopTemplates[ROP_PREFRAME_TPLATE],
        theRopTemplates[ROP_LPREFRAME_TPLATE],
        theRopTemplates[ROP_TPOSTFRAME_TPLATE],
        theRopTemplates[ROP_POSTFRAME_TPLATE],
        theRopTemplates[ROP_LPOSTFRAME_TPLATE],
        theRopTemplates[ROP_TPOSTRENDER_TPLATE],
        theRopTemplates[ROP_POSTRENDER_TPLATE],
        theRopTemplates[ROP_LPOSTRENDER_TPLATE],

        PRM_Template(
            PRM_STRING | PRM_TYPE_INVISIBLE,
            1,
            &tinaProgressScriptName,
            0, // default
            0, // menu choices
            0, // range
            0, // callback
            0, // thespareptr (leave default)
            0, // paramgroup (leave default)
            "Reservered for use by tina", // help string
            0), // disable rules

        PRM_Template(),
    };

    static OP_TemplatePair usdTemplates(templates);
    static OP_TemplatePair ropTemplates(ROP_Node::getROPbaseTemplate(), &usdTemplates);
    return &ropTemplates;
}

static PRM_Name    protoFileName("protofile", "Inst Proto File");
static PRM_Default protoFileNameDefault(0, "");

static PRM_Name instanceHeadingName("instancingheading", "Instancing");
static PRM_Name instancePackedUsdName("instancepackedusd", "Instance Packed USD Prims");
static PRM_Name writeProtoIdsName("writeprotoids", "Write Instance Prototype Ids");
static PRM_Name coalesceFragmentsName("coalescefragments", "Coalesce Fragments" );

static PRM_Name objPathName("objpath", "OBJ Path");

PRM_Template obsoleteParameters[] = 
{
    PRM_Template(
        PRM_HEADING,
        1,
        &instanceHeadingName,
        0, // default
        0, // menu choices
        0, // range
        0, // callback
        0, // thespareptr (leave default)
        0, // paramgroup (leave default)
        0, // help string
        0), // disable rules

    PRM_Template(
        PRM_FILE,
        1,
        &protoFileName,
        &protoFileNameDefault,
        0, // menu choices
        0, // range
        0, // callback
        0, // thespareptr (leave default)
        0, // paramgroup (leave default)
        "Reference to add to USD file", // help string
        0), // disable rules

    PRM_Template(
        PRM_TOGGLE,
        1,
        &instancePackedUsdName,
        PRMzeroDefaults, // default
        0, // menu choices
        0, // range
        0, // callback
        0, // thespareptr (leave default)
        0, // paramgroup (leave default)
        "When enabled, any PackedUSD primitives found on export will be "
        "gathered into a pointInstancer primitive. This allows you to copy "
        "PackedUSD prims with a copy SOP and convert the output to point "
        "instances. Prototype PackedUSD prims must still be specified in "
        "the Prototype Paths parameter below.",
        0), // disable rules
    
    PRM_Template(
        PRM_TOGGLE,
        1,
        &writeProtoIdsName,
        PRMzeroDefaults, // default
        0, // menu choices
        0, // range
        0, // callback
        0, // thespareptr (leave default)
        0, // paramgroup (leave default)
        "Include an attribute that contains a unique instance id. "
        "This is required when writing prototypes for point instancers. ",
        0), // disable rules

    PRM_Template(
        PRM_STRING_OPREF,
        PRM_TYPE_DYNAMIC_PATH,
        1,
        &objPathName,
        0, // default
        0, // choice
        0, // range
        0, // callback
        &PRM_SpareData::objPath,
        0, // paramgroup (leave default)
        "OBJ network to export", // help string
        0), // disable rules

    PRM_Template(
        PRM_TOGGLE,
        1,
        &coalesceFragmentsName,
        PRMoneDefaults, // default
        0, // menu choices
        0, // range
        0, // callback
        0, // thespareptr (leave default)
        0, // paramgroup (leave default)
        "Coalesce packed fragments into a single mesh.",
        0), // disable rules

    PRM_Template(),
};

//------------------------------------------------------------------------------


OP_VariablePair*
getVariablePair()
{
    static OP_VariablePair* pair = 0;
    if (!pair) {
        pair = new OP_VariablePair(ROP_Node::myVariableList);
    }
    return pair;
}

} // namespace

void GusdROP_usdoutput::
Register(OP_OperatorTable* table)
{
    OP_Operator* usdOutROP = new OP_Operator(
            "pixar::usdoutput",
            "USD Output",
            creator,
            getTemplates(),
            (unsigned int)0, 
            (unsigned int)1024,
            getVariablePair(),
            OP_FLAG_GENERATOR );
    usdOutROP->setIconName("pxh_gusdIcon.png");
    usdOutROP->setObsoleteTemplates(obsoleteParameters);
    usdOutROP->setOpTabSubMenuPath( "Pixar" );
    table->addOperator(usdOutROP);
    table->setOpFirstName( "pixar::usdoutput", "usdoutput" );

    // We can use this ROP in a sop context.
    OP_Operator* usdOutSOP = new OP_Operator(
            "pixar::usdrop",
            "ROP USD Output",
            creator,
            getTemplates(),
            (unsigned int)0, 
            (unsigned int)1,
            getVariablePair(),
            OP_FLAG_GENERATOR|OP_FLAG_MANAGER);
    usdOutSOP->setIconName("pxh_gusdIcon.png");
    usdOutSOP->setOpTabSubMenuPath( "Pixar" );


    // Note:  This is reliant on the order of operator table construction and
    // may not be safe to do in all cases.
    OP_OperatorTable* sopTable
        = OP_Network::getOperatorTable(SOP_TABLE_NAME, SOP_SCRIPT_NAME);
    sopTable->addOperator(usdOutSOP);
    sopTable->setOpFirstName( "pixar::usdrop", "usdrop" );
}


GusdROP_usdoutput::
GusdROP_usdoutput(OP_Network* network,
           const char* name,
           OP_Operator* entry)
    : ROP_Node(network, name, entry)
{}


GusdROP_usdoutput::
~GusdROP_usdoutput() 
{}

bool 
GusdROP_usdoutput::updateParmsFlags()
{
    bool inSopContext = CAST_SOPNODE(getInput(0)) != NULL;
    bool changed = ROP_Node::updateParmsFlags();
    changed |= enableParm("soppath", !inSopContext);
    return changed;
}

ROP_RENDER_CODE GusdROP_usdoutput::
openStage(fpreal tstart, int startTimeCode, int endTimeCode)
{
    // Always reset the temporary file descriptor to be invalid.
    m_fdTmpFile = -1;

    UT_String fn;
    evalString(fn, "usdfile", 0, tstart);
    std::string fileName = fn.toStdString();

    if (fileName == "") {
        return abort("Unable to create new usd file, no usdfile path given.");
    }

    // Each task on the farm shold write to a seperate file. However, several
    // tasks my try to create the directory at the same time. Try and avoid
    // erroring when this happens. Note we may have to try multiple times 
    // if we need to create multiple directories in the hierarchy.
    string dir = TfGetPathName(fileName);
    dir = TfStringTrimRight( dir, "/" );
    if( dir.empty() ) {
        dir = ".";
    }

    if( !TfIsDir(dir, true) ) {
        size_t maxRetries = 5;
        do {
            TfMakeDirs(dir);
        } while( --maxRetries > 0 && !TfIsDir(dir, true) );

        if (!TfIsDir(dir, true)) {

            const std::string errorMessage = "Unable to create directory: " + dir;
            return abort(errorMessage);
        }
    }

    if( access( fileName.c_str(), F_OK ) == 0 &&
        access( fileName.c_str(), W_OK ) != 0 ) {
        return abort( "Don't have permissions to write file: " + fileName );
    }

    bool overlay = evalInt( "overlay", 0, tstart );
    if (overlay) {
        UT_String refFile;
        evalString( refFile, "referencefile", 0, tstart );

        // To apply an overlay, the usd stage will be edited in
        // place, via its SessionLayer. This SessionLayer will
        // later be saved to disk (writing out all overlay edits)
        // and once saved, will then be cleared back out.

        std::string err;
        {
            GusdUT_StrErrorScope scope(&err);
            GusdUT_ErrorContext errCtx(scope);

            GusdStageCacheReader cache;
            m_usdStage = cache.FindOrOpen(refFile, GusdStageOpts::LoadAll(),
                                          GusdStageEditPtr(), &errCtx);
        }
        if (!m_usdStage) {
            return abort(err);
        }

        // BUG: Mutating stages returned from the cache is not safe!
        // Crashes, non-deterministic cooks, cats and dogs living together...
        // The only safe way to mutate a stage is to make a new stage,
        // and put locks around it if there's any possibility of other
        // threads trying to access it at the same time.
        if (m_usdStage->GetSessionLayer()) {
            m_usdStage->GetSessionLayer()->Clear();
        } else {
            m_usdStage = UsdStage::Open(m_usdStage->GetRootLayer(),
                                        SdfLayer::CreateAnonymous());
            if (!m_usdStage) {
                const std::string errorMessage = "Unable to open: "
                                                 + refFile.toStdString();
                return abort(errorMessage);
            }
        }
        // Set m_usdStage's EditTarget to be its SessionLayer.
        m_usdStage->SetEditTarget(m_usdStage->GetSessionLayer());

        // If given model path and asset name detail attributes, we set up an
        // edit target to remap the output of the overlay to the specfied
        // model's scope. For exmaple, uutput would be /model/geom/... instead
        // of /World/sets/model/geom...

        // Cook the node to get detail attributes.
        OP_Context houdiniContext(startTimeCode);
        GU_DetailHandle cookedGeoHdl = m_renderNode->getCookedGeoHandle(houdiniContext);

        // Get the model path and asset name
        UT_String modelPath;
        if (cookedGeoHdl.isValid()) {
            GU_DetailHandleAutoReadLock detailLock( cookedGeoHdl );
            const GEO_AttributeHandle modelPathHandle = detailLock->getDetailAttribute("usdmodelpath");
            const GEO_AttributeHandle assetNameHandle = detailLock->getDetailAttribute("usdassetname");
            const GEO_AttributeHandle defaultPrimPathHandle = detailLock->getDetailAttribute("usddefaultprimpath");
            modelPathHandle.getString(modelPath);
            UT_String assetName;
            if(assetNameHandle.getString(assetName)) {
                m_assetName = assetName.toStdString();
            }
            UT_String defaultPrimPath;
            if(defaultPrimPathHandle.getString(defaultPrimPath)) {
                m_defaultPrimPath = defaultPrimPath.toStdString();
            }
        }

        // If we have both, proceed to remapping through edit target.
        if (!m_assetName.empty() && m_assetName != "None" &&
            modelPath.isstring() && modelPath.toStdString() != "None") {

            // Add a reference from the model path to the asset name. This
            // allows us to create a pcp mapping function through that reference
            // arc. The result is we can write to the normal shot convert
            // usdprimpath, but it will automatically map to a prim path with
            // the model as the root. Use the root later for temporary edits we
            // don't want to save.
            m_usdStage->GetRootLayer()->SetPermissionToSave(false);
            m_usdStage->SetEditTarget(m_usdStage->GetRootLayer());

            // Get the prim whose scope we are mapping to.
            m_modelPrim = m_usdStage->GetPrimAtPath(SdfPath(modelPath.toStdString() ));

            // Make sure model prim exists on stage.
            if( !m_modelPrim.IsValid() ){
                const std::string errorMessage = "Unable to find model at: "
                                                 + modelPath.toStdString();
                return abort(errorMessage);
            }

            // Create an overlay of the asset name as a root scope.
            UsdPrim refPrim = m_usdStage->OverridePrim(SdfPath("/" + m_assetName ));

            // Reference that new root scope.
            SdfReference ref = SdfReference();
            ref.SetPrimPath(SdfPath("/" + m_assetName ));
            UsdReferences refs = m_modelPrim.GetReferences();
            refs.AddReference( ref );

            // Get the model's prim index (contains all opinions on this node)
            const PcpPrimIndex idx = m_modelPrim.ComputeExpandedPrimIndex();

            // Find the node that referenced in the model.
            PcpNodeRef node;
            for (const PcpNodeRef& child : idx.GetNodeRange()) {
                if (child.GetArcType() == PcpArcTypeReference &&
                    child.GetDepthBelowIntroduction() == 0 &&
                    child.GetPath() == SdfPath("/" + m_assetName ) ) {
                    node = child;
                    break;
                }
            }

            // Can't remap if the node is invalid.
            if (!node) {
                const std::string errorMessage = "Unable to find valid node "
                                                "for remapping with asset "
                                                "name:" + m_assetName;
                return abort(errorMessage);
            }

            // Create the edit target with the node (and its mapping). 
            UsdEditTarget editTarget = UsdEditTarget(m_usdStage->GetSessionLayer(), node);
            m_usdStage->SetEditTarget(editTarget);

            // Remove the temp reference.
            refs.ClearReferences();
        }
    } else {
        
        // Find out if a layer with this fileName already exists.
        if (SdfLayer::Find(fileName)) {
            // Get the SdfFileFormat from fileName.
            SdfFileFormatConstPtr format =
                SdfFileFormat::FindByExtension(fileName);
            if (!format) {
                return abort("Unable to determine USD format of: " + fileName);
            }

            // Create a temporary file in the same dir as fileName.
            std::string tmpFileName;
            m_fdTmpFile = ArchMakeTmpFile(dir, TfGetBaseName(fileName),
                                          &tmpFileName);
            if (m_fdTmpFile == -1) {
                return abort("Unable to create temporary file in: " + dir);
            }
            // Copy file permissions from fileName to tmpFileName.
            int mode;
            if (!ArchGetStatMode(fileName.c_str(), &mode)) {
                mode = 0664; // Use 0664 (-rw-rw-r--) if stat of fileName fails.
            }
            ArchChmod(tmpFileName.c_str(), mode);

            // Create a rootLayer and stage with tmpFileName.
            SdfLayerRefPtr tmpLayer = SdfLayer::CreateNew(format, tmpFileName);
            m_usdStage = UsdStage::Open(tmpLayer);

            if (!m_usdStage) {
                ArchUnlinkFile(tmpFileName.c_str());
                return abort("Unable to create new stage: " + tmpFileName);
            }

        } else {
            m_usdStage = UsdStage::CreateNew(fileName);

            if (!m_usdStage) {
                return abort("Unable to create new stage: " + fileName);
            }
        }
    }

    m_usdStage->SetStartTimeCode(startTimeCode);
    m_usdStage->SetEndTimeCode(endTimeCode);

    return ROP_CONTINUE_RENDER;
}

namespace {
void 
copyKindMetaDataForOverlays( UsdStageRefPtr stage, SdfPrimSpecHandle p )
{
    UsdPrim usdPrim = stage->GetPrimAtPath( p->GetPath() );
    if( !usdPrim )
        return;

    TfToken kind;
    UsdModelAPI( usdPrim ).GetKind(&kind);
 
    if( !kind.IsEmpty() ) {
        p->SetKind( kind );
    }

    // Recurse until we find a model
    if( usdPrim.IsGroup() ) {
        TF_FOR_ALL( it, p->GetNameChildren() ) {
            copyKindMetaDataForOverlays( stage, *it );
        }
    }
}
} // close namespace 

ROP_RENDER_CODE GusdROP_usdoutput::
closeStage(fpreal tend)
{
    // m_usdStage will be invalid if something failed.
    if( !m_usdStage )
        return ROP_CONTINUE_RENDER;
    
    TfToken upAxis = UsdGeomGetFallbackUpAxis();
    UsdGeomSetStageUpAxis(m_usdStage, upAxis);

    setCamerasAreZup(m_usdStage, /* zUp= */ false);

    UT_String usdFile;
    evalString(usdFile, "usdfile", 0, tend);

    bool overlay = evalInt("overlay", 0, tend);
    if (overlay) {
        m_usdStage->GetSessionLayer()->Export(usdFile.toStdString());

        // Now that the SessionLayer has been exported into a file,
        // clear out all the changes in the SessionLayer to restore
        // it to the way it was before any overlay edits were applied.
        m_usdStage->GetSessionLayer()->Clear();
    } else {
        // traverse stage and define any typeless prims as xforms
        // XXX should there be a user option for xforms, overs, possibly others?
        bool hasPrims = false;
        for(UsdPrim prim: UsdPrimRange::Stage(m_usdStage)) {
            if(!prim.HasAuthoredTypeName()) {
                prim.SetTypeName(TfToken("Xform"));
            }
            hasPrims = true;
        }
    
        if( !hasPrims && evalInt("alwayswriteroot", 0, tend) ) {

            // If we are writing per frame files and an a prim does not have
            // geometry on a frame, the USD file will be empty. Reading a packed
            // USD prim from an empty is funky. So we add the option of always 
            // writing an empty group. 
            if( SdfPath::IsValidPathString( m_pathPrefix ) ) {
                UsdGeomXform prim = UsdGeomXform::Define( m_usdStage, SdfPath( m_pathPrefix ) );
                setKind( m_pathPrefix, m_usdStage );
            }
        }

        m_usdStage->GetRootLayer()->Save();

        // If m_fdTmpFile is valid, then the rootLayer of m_usdStage is just
        // a temporary file. It was just saved to disk, and now it needs to
        // be renamed to replace usdFile.
        if (m_fdTmpFile != -1) {
            // Release the file descriptor.
            close(m_fdTmpFile);
            m_fdTmpFile = -1;

            const char* tmpFilePath =
                m_usdStage->GetRootLayer()->GetRealPath().c_str();

            const char* targetPath = usdFile.c_str();
            if (unlink(targetPath) != 0 || 
                std::rename(tmpFilePath, targetPath) != 0) {

                unlink(tmpFilePath);
                return abort("Failed to replace file: " + usdFile.toStdString());
            }

            // Reload any stages on the cache matching this path.
            // Note that this is deferred til the main event queue
            GusdStageCacheWriter cache;
            UT_StringSet paths;
            paths.insert(targetPath);
            cache.ReloadStages(paths);
        }
    }

    return ROP_CONTINUE_RENDER;
}

int GusdROP_usdoutput::
startRender(int frameCount,
            fpreal tstart,
            fpreal tend) 
{
    resetState();

    DBG(cerr << "GusdROP_usdoutput::startRender " << CHgetSampleFromTime(tstart) << ", " << CHgetSampleFromTime(tend) <<  endl);

    // Validate inputs as much as possible before we start doing any real work
    m_renderNode = NULL;

    // Check to see it the ROP is being used is in a SOP context. If so,
    // output the SOP connected to our input.
    if(SOP_Node* sopNode = CAST_SOPNODE( getInput( 0 ) )) {
        m_renderNode = sopNode;
    } else {

        UT_String sopPath;
        evalString(sopPath, "soppath", 0, tstart);
        if( !sopPath.isstring() ) {
            return abort( "SOP Path not set to a valid value." );
        }
        else {
    
            sopNode = findSOPNode(sopPath);
            if(!sopNode) {
                const std::string errorMessage = "Unable to find sop: "
                    + std::string(sopPath);
                return abort(errorMessage);
            }
            m_renderNode = sopNode;
        }
    }

    UT_String fn;
    evalString(fn, "usdfile", 0, tstart);
    if( !fn.isstring() ) {
        return abort( "USD File is not set to a valid value." );
    }

    if( evalInt( "overlay", 0, 0 ) ) {

        UT_String refFile;
        evalString( refFile, "referencefile", 0, tstart );
        if( !refFile.isstring() ) {
            return abort( "Overlay reference file is not set to a valid value." );
        }
    }


    // The ROP_Node built in preRenderScript does not always run when you expect it to.
    // It seems to be unreliable when chaining networks.
    // Add a new property and run the script ourselves so we can be sure it runs
    // at the right time.
    UT_String preRenderScript;
    evalString( preRenderScript, "pxhprerenderscript", 0, tstart );
    if( preRenderScript.isstring() ) {
        OP_ERROR err = executeScript( preRenderScript, CH_PYTHON_SCRIPT, tstart );
        if( err != UT_ERROR_NONE ) {
            return abort("Pre render script failed.");
        }
    }    

    m_startFrame = CHgetSampleFromTime(tstart);
    m_endFrame   = CHgetSampleFromTime(tend);

    m_granularity = 
        static_cast<Granularity>(evalInt( "granularity", 0, tstart ));

    if( m_granularity == ONE_FILE ) {
        int rv = openStage( tstart, m_startFrame, m_endFrame );
        if( rv != ROP_CONTINUE_RENDER )
            return rv;
    }

    // Path prefix
    UT_String pathPrefix;
    evalString(pathPrefix, "pathprefix", 0, tstart);
    pathPrefix.trimBoundingSpace();
    pathPrefix.harden();
    if(pathPrefix.isstring()) {
        string s = pathPrefix.toStdString();

        if( s.front() != '/' ) {
            s = string("/") + s;
        }
        // remove trailing slashes just to be consistant
        if( s.back() == '/' )
            s.pop_back();
        m_pathPrefix = s;
    }
   
    // Partition by attribute
    if(evalInt("enablepathattrib", 0, tstart)) {
        UT_String partitionAttr;
        evalString(partitionAttr, "pathattrib", 0, tstart);
        partitionAttr.trimBoundingSpace();
        if(partitionAttr.isstring()) {
            m_hasPartitionAttr = true;
            m_partitionAttrName = partitionAttr.toStdString();
        }
    }

    // Fill primvar filter
    UT_String primvars;
    evalString(primvars, "varyingprimvars", 0, tstart);
    m_primvarFilter.setPattern(GT_OWNER_POINT, primvars.toStdString());
    evalString(primvars, "facevaryingprimvars", 0, tstart);
    m_primvarFilter.setPattern(GT_OWNER_VERTEX, primvars.toStdString());
    evalString(primvars, "uniformprimvars", 0, tstart);
    m_primvarFilter.setPattern(GT_OWNER_UNIFORM, primvars.toStdString());
    evalString(primvars, "constantprimvars", 0, tstart);
    m_primvarFilter.setPattern(GT_OWNER_CONSTANT, primvars.toStdString());

    // This was copied from a SSI example. I strongly suspect this is a no-op.
    executePreRenderScript(tstart);

    return ROP_CONTINUE_RENDER;
}
namespace {

void
setKind( const string &path, UsdStagePtr stage )
{
    // When we are creating new geometry (not doing overlays), the path prefix 
    // parm specifies the root of our asset. This prim needs to be marked as a
    // component (model) and all its ancestors need to be marked group.
    //
    // Unless we are writing a group of references to other assets. This is the 
    // case if our chidren are models.

    if( path.empty() )
        return;

    UsdPrim p = stage->GetPrimAtPath( SdfPath( path ) );
    if( !p.IsValid() )
        return;
    UsdModelAPI model( p ); 
    TfToken kind;
    if( model && !model.GetKind( &kind )) {

        bool hasModelChildren = false;
        BOOST_FOREACH( UsdPrim child, p.GetChildren()) {

            TfToken childKind;
            UsdModelAPI( child ).GetKind( &childKind );
            if( KindRegistry::IsA( childKind, KindTokens->model )) {
                hasModelChildren = true;
                break;
            }
        }
        if( hasModelChildren )
            model.SetKind( KindTokens->group );
        else
            model.SetKind( GusdGetAssetKind() );
    }
    for( UsdPrim p = model.GetPrim().GetParent(); p.IsValid(); p = p.GetParent() ) {
        UsdModelAPI m( p );
        if( m && !m.GetKind( &kind ))
            m.SetKind( KindTokens->group );
    }
}

} // close namespace

ROP_RENDER_CODE GusdROP_usdoutput::
renderFrame(fpreal time,
            UT_Interrupt* interupt) 
{
    executePreFrameScript(time);

    const double frame = CHgetSampleFromTime(time);

    DBG(cerr << "GusdROP_usdoutput::renderFrame " << frame << endl);

    if( m_granularity == PER_FRAME ) {
        ROP_RENDER_CODE rv = openStage( time, frame, frame );
        if( rv != ROP_CONTINUE_RENDER )
            return rv;
    }

    GT_RefineParms refineParms;

    // Tell the collectors (in particular the f3d stuff) that we are 
    // writing a USD file rather than doing interactive visualization.
    // an interactive visualization
    refineParms.set( "refineToUSD", true );

    const bool overlayGeo = evalInt( "overlay", 0, 0 );
    const bool overlayAll = evalInt( "overlayall", 0, 0 );
    const bool overlayPoints = evalInt( "overlaypoints", 0, 0 );
    const bool overlayXforms = evalInt( "overlayxforms", 0, 0 );
    const bool overlayPrimvars = evalInt( "overlayprimvars", 0, 0 );

    // Find the obj node that contains to SOP we are exporting
    OBJ_Node *objNode = CAST_OBJNODE(m_renderNode->getCreator());

    // If parms have been added to the obj node that will cause the meshs 
    // to be rendered in houdini as a subdivs, output the USD to render as a subdiv.
    int polysAsSubd = 0;
    if ((objNode->evalParameterOrProperty("ri_rendersubd", 0, time, polysAsSubd)
        && polysAsSubd != 0)
      ||(objNode->evalParameterOrProperty("ri_renderhsubd", 0, time, polysAsSubd)
        && polysAsSubd != 0)
      ||(objNode->evalParameterOrProperty("vm_rendersubd", 0, time, polysAsSubd)
        && polysAsSubd != 0)) {
        
        refineParms.setPolysAsSubdivision( true );
    } 

    OP_Context houdiniContext(time);

    // Get the OBJ node transform
    UT_Matrix4D localToWorldMatrix;
    objNode->getLocalToWorldTransform( houdiniContext, localToWorldMatrix);

    // Cook our input
    GU_DetailHandle cookedGeoHdl = m_renderNode->getCookedGeoHandle(houdiniContext);
    if (!cookedGeoHdl.isValid()) {
        const std::string errorMessage = "invalid cooked geometry from sop: "
            + std::string(m_renderNode->getName());
        return abort(errorMessage);
    }

    GusdRefinerCollector refinerCollector;
    GusdRefiner refiner( 
        refinerCollector,
        m_pathPrefix.empty() ? SdfPath() : SdfPath(m_pathPrefix),
        m_partitionAttrName, 
        localToWorldMatrix );

    // If we ae only overlaying transforms and encounter a packed prim,
    // just write the transform and don't refine further.
    refiner.m_refinePackedPrims = !overlayGeo || !( overlayXforms && !(overlayAll || overlayPoints ));

    // If writing an overlay and a prim has an instinsic path, write the prim to that path
    refiner.m_useUSDIntrinsicNames = overlayGeo;

    // Check for a (usd)instancepath paramter/property to set as the default
    // value. This tells us to build a point instancer.
    UT_String usdInstancePath;
    if(!evalParameterOrProperty("usdinstancepath", 0, 0, usdInstancePath)) {
        if(!objNode->evalParameterOrProperty("usdinstancepath", 0, 0, usdInstancePath)) {
            if(!evalParameterOrProperty("instancepath", 0, 0, usdInstancePath)) {
                objNode->evalParameterOrProperty("instancepath", 0, 0, usdInstancePath);
            }
        }
    }
    if(usdInstancePath.isstring()) {
        refiner.m_buildPointInstancer = true;
    }

    refiner.m_writeCtrlFlags.overAll = overlayAll;
    refiner.m_writeCtrlFlags.overPoints = overlayPoints;
    refiner.m_writeCtrlFlags.overTransforms = overlayXforms;
    refiner.m_writeCtrlFlags.overPrimvars = overlayPrimvars;

    refiner.refineDetail( cookedGeoHdl, refineParms );

    // If we are building a point instancer, the refiner will have accumulated all the 
    // instances. Now we can build the instancer prims.
    const GusdRefiner::GprimArray& gprimArray = refiner.finish();

    DBG( cerr << "Num of refined gt prims = " << gprimArray.size() << endl );

    // Build a structure to hold the data that the wrapper prims need to 
    // write to USD.
    GusdContext ctxt( UsdTimeCode(frame), 
                      GusdContext::Granularity(m_granularity), 
                      m_primvarFilter );

    if(usdInstancePath.isstring()) {
        ctxt.usdInstancePath = usdInstancePath;
    }

    ctxt.writeOverlay = overlayGeo;

    // Check for a usdprototypespath paramter/property to set as the default
    // for point instancing.
    UT_String usdPrototypesPath;
    if(!evalParameterOrProperty("usdprototypespath", 0, 0, usdPrototypesPath)) {
        objNode->evalParameterOrProperty("usdprototypespath", 0, 0, usdPrototypesPath);
    }
    if(usdPrototypesPath.isstring()) {
        ctxt.usdPrototypesPath = usdPrototypesPath.toStdString();
    }

    // Check for usd Layer Offset attributes (offset and scale).
    fpreal usdTimeOffset = 0;
    if(!evalParameterOrProperty("usdtimeoffset", 0, 0, usdTimeOffset)) {
        objNode->evalParameterOrProperty("usdtimeoffset", 0, 0, usdTimeOffset);
    }
    ctxt.usdTimeOffset = usdTimeOffset;

    fpreal usdTimeScale = 1.0;
    if(!evalParameterOrProperty("usdtimescale", 0, 0, usdTimeScale)) {
        objNode->evalParameterOrProperty("usdtimescale", 0, 0, usdTimeScale);
    }
    ctxt.usdTimeScale = usdTimeScale;

    if( m_hasPartitionAttr )
        ctxt.primPathAttribute = m_partitionAttrName;

    ctxt.authorVariantSelections = evalInt( "authorvariantselection", 0, 0 );

    ctxt.makeRefsInstanceable = 
        static_cast<bool>(evalInt( "usdinstancing", 0, 0 ));

    // This ROP supports binding shaders from 2 different sources:
    // 1. A shader that is already defined in a usd file somewhere can be
    //    referenced into this stage.
    // 2. A material (shop network) inside houdini can be converted into
    //    a usd shader and authored into this stage.
    //
    // Store maps of per-prim assignments for both shader types.
    UsdRefShaderMap usdRefShaderMap;
    HouMaterialMap houMaterialMap;

    // Sort the refined prim array by primitive paths. This ensures parents
    // will be written before their children.
    GusdRefinerCollector::GprimArray gPrims = gprimArray;
    std::sort( gPrims.begin(), gPrims.end(),
            []( const GusdRefinerCollector::GprimArrayEntry& a,
                const GusdRefinerCollector::GprimArrayEntry& b ) -> bool
            { return a.path < b.path; } );

    UT_Set<SdfPath> gprimsProcessedThisFrame;
    GusdSimpleXformCache xformCache;
    bool needToUpdateModelExtents = false;

    // Iterate over the refined prims and write
    for( auto& gtPrim : gPrims ) {

        const SdfPath& primPath = gtPrim.path;

        DBG(cerr << "Write prim: " << primPath << ", type = " << gtPrim.prim->className() << endl);

        // Copy properties that were accumulated in the refiner and stored with 
        // the refined prim to the context.
        ctxt.purpose = gtPrim.purpose;
        const GusdWriteCtrlFlags& flags = gtPrim.writeCtrlFlags;
        ctxt.overlayPoints =     overlayGeo && (flags.overPoints || flags.overAll);
        ctxt.overlayTransforms = overlayGeo && (flags.overTransforms || flags.overAll);
        ctxt.overlayPrimvars =   overlayGeo && (flags.overPrimvars || flags.overAll);
        ctxt.overlayAll =        overlayGeo && flags.overAll;

        ctxt.writeStaticGeo = flags.writeStaticGeo;
        ctxt.writeStaticTopology = flags.writeStaticTopology;
        ctxt.writeStaticPrimvars = flags.writeStaticPrimvars;

        if( ctxt.overlayPoints || ctxt.overlayTransforms ) {
            needToUpdateModelExtents = true;
        }

        gprimsProcessedThisFrame.insert(primPath);

        GT_PrimitiveHandle usdPrim;

        // Have we seen this prim on a previous frame?
        GprimMap::iterator gpit = m_gprimMap.find(primPath);
        if( gpit == m_gprimMap.end() ) {

            // Create a new USD prim
            usdPrim = GusdPrimWrapper::defineForWrite(
                        gtPrim.prim, m_usdStage, primPath, ctxt );

            if( !usdPrim ) {
                TF_WARN( "prim did not convert. %s", gtPrim.prim->className() );
            }
            else {
                m_gprimMap[primPath] = usdPrim;

                GusdPrimWrapper* primPtr
                        = UTverify_cast<GusdPrimWrapper*>(usdPrim.get());

                // If we're writing many frames to a single file, write 
                // "bookend" visibility samples if the prim appears after 
                // the start frame.
                if ( m_granularity == ONE_FILE ) {
                    if ( frame != m_startFrame ) {
                        primPtr->addLeadingBookend( frame, m_startFrame );
                    }
                }
                primPtr->markVisible( true );
            }
        }
        else {

            // Add samples to a existing prim
            usdPrim = gpit->second;

            SdfLayerHandle layer = overlayGeo ? 
                    m_usdStage->GetSessionLayer() : m_usdStage->GetRootLayer();

            // If a USD version of this prim doesn't exist on the current edit
            // target's layer, create a new USD prim. This happens when we are
            // writing per frame files.
            SdfPrimSpecHandle ph = m_usdStage->GetEditTarget().GetPrimSpecForScenePath( primPath );
            if( !ph ) {
                dynamic_cast<GusdPrimWrapper*>(usdPrim.get())->
                    redefine( m_usdStage, SdfPath( primPath ), ctxt, gtPrim.prim );
            }

            GusdPrimWrapper* primPtr
                    = UTverify_cast<GusdPrimWrapper*>(usdPrim.get());
            if( !primPtr->isVisible() ) {
                primPtr->markVisible( true );
            }
        }

        if(usdPrim) {

            GusdPrimWrapper* primPtr
                    = UTverify_cast<GusdPrimWrapper*>(usdPrim.get());

            // Copy attributes from gt prim to USD prim.
            primPtr->updateFromGTPrim(gtPrim.prim,
                                      gtPrim.xform,
                                      ctxt,
                                      xformCache );

            // Get prim-level usdShadingFile and usdShader if they exist
            string primUsdShadingFile = 
                getStringUniformOrDetailAttribute(gtPrim.prim, "usdShadingFile");
            string primUsdShader = 
                getStringUniformOrDetailAttribute(gtPrim.prim, "usdShader");

            if( !primUsdShadingFile.empty() && !primUsdShader.empty() ) {
                UsdRefShader usdRefShader = std::make_pair(primUsdShadingFile,
                                                           primUsdShader);
                addShaderToMap(usdRefShader, SdfPath(gtPrim.path.GetString()),
                               usdRefShaderMap);
            }

            // Get prim-level shop_materialpath attribute if it exists.
            string primMaterialPath =
                getStringUniformOrDetailAttribute(gtPrim.prim,
                                                  "shop_materialpath");
            if (!primMaterialPath.empty()) {
                addShaderToMap(primMaterialPath,
                               SdfPath(gtPrim.path.GetString()),
                               houMaterialMap);
            }
            // If we're attempting to overlay instanced geometry, set the root
            // of the instance to 'instanceable = false'. Recurse on the parent
            // in case it itself is an instance.
            SdfPath currPath = primPath;
            UsdPrim currPrim = m_usdStage->GetPrimAtPath( currPath );
            while ( currPrim.IsInstanceProxy() ) {
                // Get the master prim which corresponds to each instance
                UsdPrim masterPrim = currPrim.GetPrimInMaster(); 
                const SdfPath& masterPath = masterPrim.GetPath();
                // Removing common suffices results in just the path that was
                // instance for our prim (and /__master_* for the master path)
                const pair<SdfPath, SdfPath> pathsPair = currPath.RemoveCommonSuffix(masterPath);
                currPath = pathsPair.first;
                if ( currPath.IsEmpty() ) {
                    // We shouldn't get here
                    break;
                }
                // Get the prim on the stage (not on the master)
                UsdPrim instancePrim = m_usdStage->GetPrimAtPath( currPath );
                // Check to make sure we're deinstancing an instance
                if ( instancePrim.IsInstance() ) {
                    DBG(cerr << "Deinstanced prim at: " << currPath.GetText() << endl);
                    instancePrim.SetInstanceable(false);
                }
                // Recurse on the parent prim in case it's nested as another instance
                currPrim = instancePrim;

            }

            // Check for a hero prim to operate on.
            GT_Owner owner = GT_OWNER_UNIFORM;
            GT_DataArrayHandle heroAttr = gtPrim.prim->findAttribute( "usdheroprim", owner, 0);
            if(heroAttr != NULL && heroAttr->getI32(0) > 0) {

                // Get the hero prim from the stage.
                UsdPrim heroPrim = m_usdStage->GetPrimAtPath( primPath );

                // Call the registered operate on usd prim function on our hero.
                if ( heroPrim && m_modelPrim.IsValid() ) {
                    std::string modelPath = m_modelPrim.GetName().GetString();
                    do {
                        GusdOperateOnUsdPrim( heroPrim );
                        if (heroPrim.GetName().GetString() == modelPath) {
                            break;
                        }
                        heroPrim = heroPrim.GetParent();
                    } while ( heroPrim.IsValid());
                }
            }
        }
    }
    
    // If we're holding prims which weren't processed on this frame, they should
    // become invisible on this frame
    for(GprimMap::iterator gprimIt=m_gprimMap.begin();
            gprimIt!=m_gprimMap.end();++gprimIt) {

        if(gprimsProcessedThisFrame.find(gprimIt->first)
                == gprimsProcessedThisFrame.end()) {

            const SdfPath& primPath = gprimIt->first;

            GusdPrimWrapper* primPtr
                = UTverify_cast<GusdPrimWrapper*>(gprimIt->second.get());

            SdfPrimSpecHandle ph = m_usdStage->GetRootLayer()->GetPrimAtPath( SdfPath( primPath ) );
            if( !ph ) {
                primPtr->redefine( m_usdStage, primPath, ctxt, GT_PrimitiveHandle() );
            }

            if ( m_granularity == ONE_FILE ) {
                primPtr->addTrailingBookend( frame );
                // Remove prim from the persistent gprim map.
                m_gprimMap.erase( gprimIt );
            }
            else {
                primPtr->setVisibility( UsdGeomTokens->invisible, ctxt.time );
            }
        }
    }

    // If we are not doing an overlay, assume that all the geometry is created
    // under the node named by m_pathPrefix. User can thwart this using the usdprimprim attrribute, 
    // but in practice is works reasonably well.

    if( !overlayGeo && !m_pathPrefix.empty() ) {

        const SdfPath assetPrimPath(m_pathPrefix);
        UsdPrim assetPrim = m_usdStage->GetPrimAtPath(assetPrimPath);

        if (assetPrim) {

            // Look for obj node USD shader assignment
            UT_String usdShadingFile, usdShader;
            evalString(usdShadingFile, "usdshadingfile", 0, 0);
            evalString(usdShader, "usdshader", 0, 0);

            if( usdShadingFile.isstring() && usdShader.isstring() &&
                    usdShader.toStdString() != "None")
            {
                UsdRefShader usdRefShader =
                    std::make_pair(usdShadingFile.toStdString(),
                                   usdShader.toStdString());
                addShaderToMap(usdRefShader, assetPrimPath, usdRefShaderMap);
            }

            UT_String materialPath;
            if (objNode->evalParameterOrProperty("shop_materialpath",
                                                 0, 0, materialPath)
                    && materialPath.isstring()) {
                addShaderToMap(materialPath.toStdString(), assetPrimPath,
                               houMaterialMap);
            }
        }

        bindAndWriteShaders(usdRefShaderMap, houMaterialMap);
    }
    
    if (overlayGeo) {
        // If doing an overlay of xforms or points (basically any overlay type
        // except primvars) then bounds have likely changed due to prims being
        // moved or deformed. Now the "extentsHint" attribute will need to be
        // updated for ancestors of the prims that have been overlayed.
        if (needToUpdateModelExtents) {
            // Create a UsdGeomBBoxCache for computing extents.
            TfTokenVector includedPurposes = {UsdGeomTokens->default_ , UsdGeomTokens->render};
            UsdGeomBBoxCache cache(ctxt.time, includedPurposes,
                                   /*useExtentsHint*/ false);

            // Maintain a set of paths of ancestors visited during the following
            // loop. This is an optimization to avoid computing/setting the
            // extentsHint multiple times for the same prim.
            std::set<SdfPath> visitedPaths;
            const SdfPath rootPath("/");

            for (GprimMap::const_iterator it = m_gprimMap.begin();
                it != m_gprimMap.end(); ++it) {

                SdfPath path = SdfPath(it->first).GetParentPath();

                while (path != rootPath && path != SdfPath::EmptyPath()) {
                    if (UsdGeomModelAPI model =
                        UsdGeomModelAPI(m_usdStage->GetPrimAtPath(path))) {

                        if (model.GetExtentsHintAttr() &&
                            visitedPaths.find(path) == visitedPaths.end()) {

                            const VtVec3fArray extentsHint =
                                model.ComputeExtentsHint(cache);
                            model.SetExtentsHint(extentsHint, ctxt.time);
                        }
                    }
                    visitedPaths.insert(path);
                    path = path.GetParentPath();
                }
            }
        }

        // Turn off pruning for all prims that have been overlayed.
        for (GprimMap::const_iterator it = m_gprimMap.begin();
             it != m_gprimMap.end(); ++it) {

            const SdfPath path(it->first);
            // Check if there is anything authored at this path on m_usdStage's
            // current EditTarget. If so, also author an attribute to disable
            // pruning.
            if (m_usdStage->GetEditTarget().GetPrimSpecForScenePath(path)) {
                if (UsdPrim prim = m_usdStage->GetPrimAtPath(path)) {
                    if (UsdAttribute pruneAttr =
                        prim.CreateAttribute(TfToken("pruning:prunable"),
                                             SdfValueTypeNames->Bool, false,
                                             SdfVariabilityUniform)) {
                        pruneAttr.Set(false);
                    }
                }
            }
        }
    }

    // Set the default prim path (to default or m_defaultPrimPath if set).
    if( m_granularity == PER_FRAME ) {
        if( !m_defaultPrimPath.empty() ) {
            setKind( m_defaultPrimPath, m_usdStage );

            SdfLayerHandle layer = overlayGeo ? 
                    m_usdStage->GetSessionLayer() : m_usdStage->GetRootLayer();
            if( m_defaultPrimPath[0] == '/' && m_defaultPrimPath.find( '/', 1 ) == string::npos ) {
                SdfPrimSpecHandle defPrim = layer->GetPrimAtPath( SdfPath(m_defaultPrimPath) );
                if( defPrim ) {
                    layer->SetDefaultPrim( TfToken(m_defaultPrimPath.substr(1) ) );
                }
            }
        } else if( !overlayGeo ) {
            setKind( m_pathPrefix, m_usdStage );

            if( m_pathPrefix[0] == '/' && m_pathPrefix.find( '/', 1 ) == string::npos ) {
                UsdPrim defPrim = m_usdStage->GetPrimAtPath( SdfPath(m_pathPrefix) );
                if( defPrim ) {
                    m_usdStage->SetDefaultPrim( defPrim );
                }
            }
        }

        ROP_RENDER_CODE rv = closeStage(time);
        if( rv != ROP_CONTINUE_RENDER )
            return rv;

        m_usdStage = NULL;
    }

    executePostFrameScript(time);

    // Tina needs to output progress messages and trigger TINA_DO on every
    // frame. 
    UT_String script;
    evalString( script, "tinaprogressscript", 0, time );
    if( script.isstring() ) {
        executeScript( script, CH_PYTHON_SCRIPT, time );
    }
    return ROP_CONTINUE_RENDER;
}


ROP_RENDER_CODE GusdROP_usdoutput::
bindAndWriteShaders(UsdRefShaderMap& usdRefShaderMap,
                    HouMaterialMap& houMaterialMap)
{
    //
    // This ROP supports binding shaders from 2 different sources:
    // 1. A shader that is already defined in a usd file somewhere can be
    //    referenced into this stage.
    // 2. A material (shop network) inside houdini can be converted into
    //    a usd shader and authored into this stage.
    //
    // In the unlikely case that a prim maps to both a referenced usd shader
    // and a houdini material, the houdini material will win. Here, this is
    // accomplished by binding all referenced usd shaders first, and binding
    // all houdini materials last.
    //

    // TODO: For now, only support houdini materials if the "enableshaders"
    // parameter is turned on. This toggle is our temporary way for enabling
    // houdini materials for exported assests, but disabling them for items
    // written from a cacher SOP. Turning this feature on inside a cacher SOP
    // (thus attempting to build the same houdini material in multiple tasks
    // at the same time) is currently unsupported/undefined behavior.
    bool enableHouShaders = evalInt("enableshaders", 0, 0);
    if (!enableHouShaders) {
        houMaterialMap.clear();
    }

    // If there are no shaders, exit now before defining a "Looks" scope.
    if (usdRefShaderMap.empty() && houMaterialMap.empty()) {
        return ROP_CONTINUE_RENDER;
    }

    SdfPath looksPath = SdfPath(m_pathPrefix).AppendChild(TfToken("Looks"));
    UsdGeomScope looksScope = UsdGeomScope::Define(m_usdStage, looksPath);

    //
    // Handle all referenced usd shaders first.
    //
    for (auto assignmentIt = usdRefShaderMap.begin();
         assignmentIt != usdRefShaderMap.end(); ++assignmentIt) {

        UsdRefShader usdRefShader = assignmentIt->first;
        vector<SdfPath>& primPaths = assignmentIt->second;

        string shaderFile = usdRefShader.first;
        string shaderName = usdRefShader.second;
        if (shaderName.front() != '/') {
            shaderName = string("/") + shaderName;
        }

        UsdShadeMaterial usdMaterial = UsdShadeMaterial::Define(m_usdStage,
            looksPath.AppendChild(TfToken(shaderName.substr(1))));
        
        UsdReferences refs = usdMaterial.GetPrim().GetReferences();
        shaderFile = GusdComputeRelativeSearchPath(shaderFile);

        UsdPrim shaderPrim;
        UsdStageRefPtr shaderStage = UsdStage::Open(shaderFile);
        if (!shaderStage) {
            TF_WARN("Could not open shader file '%s'", shaderFile.c_str());
        } else {
            shaderPrim = shaderStage->GetPrimAtPath(SdfPath(shaderName));
            if (!shaderPrim) {
                TF_WARN("Could not find shader '%s' in file '%s'", 
                        shaderName.c_str(), shaderFile.c_str());
            }
            else {
                SdfPathVector prefixes = shaderPrim.GetPath().GetPrefixes();
                refs.AddReference(shaderFile, prefixes[0]);
            }
        }
        if (shaderPrim) {
            for (auto primPathIt = primPaths.begin();
                 primPathIt != primPaths.end(); ++primPathIt) {
                UsdPrim prim = m_usdStage->GetPrimAtPath(*primPathIt);
                usdMaterial.Bind(prim);
            }
        }
    }

    UT_String shaderOutDir;
    evalString(shaderOutDir, "shaderoutdir", 0, 0);

    //
    // Handle all houdini material shaders last.
    //
    for (auto assignmentIt = houMaterialMap.begin();
         assignmentIt != houMaterialMap.end(); ++assignmentIt) {

        VOP_Node* materialVop = findVOPNode(assignmentIt->first.c_str());
        if (materialVop == NULL ||
            strcmp(materialVop->getRenderMask(),"RIB") != 0) {
            continue;
        }

        UT_String vopPath(materialVop->getFullPath());
        vopPath.forceAlphaNumeric();
        SdfPath path = looksPath.AppendPath(SdfPath(vopPath.toStdString()));

        GusdShaderWrapper shader(materialVop, m_usdStage, path.GetString(),
                                 shaderOutDir.toStdString());

        vector<SdfPath>& primPaths = assignmentIt->second;
        for (auto primPathIt = primPaths.begin();
             primPathIt != primPaths.end(); ++primPathIt) {
            UsdPrim prim = m_usdStage->GetPrimAtPath(*primPathIt);
            shader.bind(prim);
        }
    }

    return ROP_CONTINUE_RENDER;
}


void GusdROP_usdoutput::resetState()
{
    m_usdStage = NULL;

    m_startFrame = 0;
    m_endFrame = 0;
    m_houdiniContext = OP_Context();
    m_pathPrefix = "";
    m_hasPartitionAttr = false;
    m_partitionAttrName = "";
    m_renderNode = NULL;

    m_fdTmpFile = -1;

    m_gprimMap.clear();
}


ROP_RENDER_CODE GusdROP_usdoutput::
endRender() 
{
    double endTimeCode = CHgetTimeFromFrame( m_endFrame );

    // Set the default prim path (to default or m_defaultPrimPath if set).
    if( m_granularity == ONE_FILE ) {
        bool overlayGeo = evalInt( "overlay", 0, endTimeCode );
        if( !m_defaultPrimPath.empty() ) {
            setKind( m_defaultPrimPath, m_usdStage );

            SdfLayerHandle layer = overlayGeo ? 
                    m_usdStage->GetSessionLayer() : m_usdStage->GetRootLayer();
            if( m_defaultPrimPath[0] == '/' && m_defaultPrimPath.find( '/', 1 ) == string::npos ) {
                SdfPrimSpecHandle defPrim = layer->GetPrimAtPath( SdfPath(m_defaultPrimPath) );
                if( defPrim ) {
                    layer->SetDefaultPrim( TfToken(m_defaultPrimPath.substr(1) ) );
                }
            }
        } else if( !overlayGeo && m_usdStage ) {
            setKind( m_pathPrefix, m_usdStage );

            if( m_pathPrefix[0] == '/' && m_pathPrefix.find( '/', 1 ) == string::npos ) {
                UsdPrim defPrim = m_usdStage->GetPrimAtPath( SdfPath(m_pathPrefix) );
                if( defPrim ) {
                    m_usdStage->SetDefaultPrim( defPrim );
                }
            }
        }

        ROP_RENDER_CODE rv = closeStage(endTimeCode);
        if( rv != ROP_CONTINUE_RENDER )
            return rv;
    }


    resetState();

	executePostRenderScript(endTimeCode);

    return ROP_CONTINUE_RENDER;
}


ROP_RENDER_CODE GusdROP_usdoutput::
abort(const std::string& errorMessage)
{
    resetState();
    addError(ROP_MESSAGE, errorMessage.c_str());
    return ROP_ABORT_RENDER;
}

namespace {

string
getStringUniformOrDetailAttribute( 
    GT_PrimitiveHandle prim, 
    const char* attrName )
{
    // If a uniform attribute exists with the give name, return it. Otherwise
    // fallback to a detail attribute.
    if( auto uniformAttrs = prim->getUniformAttributes() ) {
        if( auto attr = uniformAttrs->get( attrName ) ) { 
            GT_String v = attr->getS(0);
            if( v != NULL ) {
                return v;
            }
        }
    }
    if( auto detailAttrs = prim->getDetailAttributes() ) {
        if( auto attr = detailAttrs->get( attrName )) {
            GT_String v = attr->getS(0);
            if( v != NULL ) {
                return v;
            }
        }
    }    
    return string();
}

bool 
setCamerasAreZup(UsdStageWeakPtr const &stage, bool isZup)
{
    if (!stage){
        return false;
    }
    bool anySet = false;
    
    TF_FOR_ALL(prim, stage->GetPseudoRoot().
                            GetFilteredChildren(UsdPrimIsDefined && 
                                                !UsdPrimIsAbstract)){
        prim->SetCustomDataByKey(TfToken("zUp"), VtValue(isZup));
        anySet = true;
    }
    return anySet;
}

template<typename ShaderT>
void addShaderToMap(const ShaderT& shader, const SdfPath& primPath,
                    UT_Map<ShaderT, std::vector<SdfPath> >& map)
{
    auto assignmentIt = map.find(shader);
    if (assignmentIt == map.end()) {
        map[shader] = vector<SdfPath>(1, primPath);
    } else {
        assignmentIt->second.push_back(primPath);
    }
}


} // close namespace

PXR_NAMESPACE_CLOSE_SCOPE
