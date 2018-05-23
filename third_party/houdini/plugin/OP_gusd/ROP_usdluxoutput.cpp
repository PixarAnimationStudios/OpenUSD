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
#include "ROP_usdluxoutput.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/fileUtils.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdUtils/pipeline.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/usd/sdf/fileFormat.h"

#include <GT/GT_GEODetail.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GEO/GEO_AttributeHandle.h>
#include <OBJ/OBJ_Node.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_Bundle.h>
#include <OP/OP_Node.h>
#include <OP/OP_BundleList.h>
#include <PRM/PRM_ChoiceList.h>
#include <PRM/PRM_SpareData.h>
#include <ROP/ROP_Error.h>
#include <ROP/ROP_Templates.h>
#include <SOP/SOP_Node.h>
#include <VOP/VOP_Node.h>

#include "gusd/gusd.h"
#include "gusd/primWrapper.h"
#include "gusd/refiner.h"
#include "gusd/stageCache.h"
#include "gusd/shaderWrapper.h"
#include "gusd/error.h"
#include "gusd/UT_Gf.h"
#include "gusd/UT_Version.h"
#include "gusd/context.h"

#include "boost/foreach.hpp"

PXR_NAMESPACE_OPEN_SCOPE

using std::string;
using std::map;
using std::pair;
using std::vector;

namespace {

void setKind( const string &path, UsdStagePtr stage );
string getStringUniformOrDetailAttribute(
        GT_PrimitiveHandle prim,
        const char* attrName );
bool setCamerasAreZup(UsdStageWeakPtr const &stage, bool isZup);

OP_Node*
creator(OP_Network* network,
        const char* name,
        OP_Operator* op)
{
    return new GusdROP_usdluxoutput(network, name, op);
}

//------------------------------------------------------------------------------
// parameters
//------------------------------------------------------------------------------
OP_TemplatePair*
getTemplates()
{
    static PRM_Name sopPathsName("soppaths", "SOP Paths");

    static PRM_SpareData sopPathsData(PRM_SpareArgs()
          << PRM_SpareToken("opfilter", "!!OBJ!!")
          << PRM_SpareToken("oprelative", "/obj")
    );

    static PRM_Name    usdFileName("usdfile", "USD File");
    static PRM_Default usdFileDefault(0, "$HIP/out.usda");

    static PRM_Name  granularityChoiceNames[] = {
            PRM_Name( "oneFile", "One File" ),
            PRM_Name( "perFrame", "Per Frame Files" ),
            PRM_Name()
    };

    static PRM_ChoiceList granularityMenu( PRM_CHOICELIST_SINGLE,
                                           granularityChoiceNames );

    static PRM_Name        granularityName("granularity","Granularity");

    static PRM_Name scriptsHeadingName("scriptsheading", "Scripts");
    static PRM_Name pxhPreRenderName("pxhprerenderscript", "Pxh Pre-Render Script");

    static PRM_Name tinaProgressScriptName( "tinaprogressscript", "Tina Progress Script" );

    static PRM_Template templates[] = {
            PRM_Template(
                    PRM_STRING_OPLIST,
                    PRM_TYPE_DYNAMIC_PATH_LIST,
                    1,
                    &sopPathsName,
                    0,
                    0,
                    0,
                    0,
                    &sopPathsData),

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

//------------------------------------------------------------------------------


OP_VariablePair*
getVariablePair()
{
    static OP_VariablePair* pair = nullptr;
    if (!pair) {
        pair = new OP_VariablePair(ROP_Node::myVariableList);
    }
    return pair;
}

} // namespace

void GusdROP_usdluxoutput::
Register(OP_OperatorTable* table)
{
    auto* usdOutROP = new OP_Operator(
            "pixar::usdluxoutput",
            "USD Lux Output",
            creator,
            getTemplates(),
            (unsigned int)0,
            (unsigned int)1024,
            getVariablePair(),
            OP_FLAG_GENERATOR );
    usdOutROP->setIconName("pxh_gusdIcon.png");
    usdOutROP->setOpTabSubMenuPath( "Pixar" );
    table->addOperator(usdOutROP);
    table->setOpFirstName( "pixar::usdluxoutput", "usdluxoutput" );

    // We can use this ROP in a sop context.
    auto* usdOutSOP = new OP_Operator(
            "pixar::usdluxrop",
            "ROP USD Lux Output",
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
    sopTable->setOpFirstName( "pixar::usdluxrop", "usdluxrop" );
}


GusdROP_usdluxoutput::
GusdROP_usdluxoutput(OP_Network* network,
                  const char* name,
                  OP_Operator* entry)
        : ROP_Node(network, name, entry)
{}

bool
GusdROP_usdluxoutput::updateParmsFlags()
{
    bool inSopContext = CAST_SOPNODE(getInput(0)) != nullptr;
    bool changed = ROP_Node::updateParmsFlags();
    changed |= enableParm("soppaths", !inSopContext);
    return changed;
}

bool
GusdROP_usdluxoutput::filterNode(OP_Node *node)
{
    if (!node)
    {
        return false;
    }

    OBJ_Node	*obj = node->castToOBJNode();
    return obj != nullptr;
}

ROP_RENDER_CODE GusdROP_usdluxoutput::
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

    // Each task on the farm should write to a separate file. However, several
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

    m_usdStage->SetStartTimeCode(startTimeCode);
    m_usdStage->SetEndTimeCode(endTimeCode);

    return ROP_CONTINUE_RENDER;
}

ROP_RENDER_CODE GusdROP_usdluxoutput::
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

    // traverse stage and define any typeless prims as xforms
    // XXX should there be a user option for xforms, overs, possibly others?
    for(UsdPrim prim: UsdPrimRange::Stage(m_usdStage)) {
        if(!prim.HasAuthoredTypeName()) {
            prim.SetTypeName(TfToken("Xform"));
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

    return ROP_CONTINUE_RENDER;
}

int GusdROP_usdluxoutput::
startRender(int frameCount,
            fpreal tstart,
            fpreal tend)
{
    resetState();

    // Validate inputs as much as possible before we start doing any real work
    m_renderNodes.clear();

    // Check to see it the ROP is being used is in a SOP context. If so,
    // output the SOP connected to our input.
    if(SOP_Node* sopNode = CAST_SOPNODE( getInput( 0 ) )) {
        m_renderNodes.append(sopNode->castToOBJNode());
    }
    else
    {
        UT_String sopPathsValue;
        evalString(sopPathsValue, "soppaths", 0, tstart);

        UT_StringList sopPaths;
        sopPathsValue.tokenize(sopPaths);

        OP_Network* objNetwork = OPgetDirector()->getManager("obj");

        if (sopPaths.entries() > 0)
        {
            for (auto i = 0; i < sopPaths.entries(); ++i)
            {
                auto sopPath = sopPaths[i];
                OP_Node* node = objNetwork->findNode(sopPath);
                if (filterNode(node))
                {
                    m_renderNodes.append(node);
                }
            }
        }
        else
        {
            m_renderNodes.append(objNetwork->castToOPNode());
        }
    }

    UT_String fn;
    evalString(fn, "usdfile", 0, tstart);
    if( !fn.isstring() ) {
        return abort( "USD File is not set to a valid value." );
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

    // This was copied from a SSI example. I strongly suspect this is a no-op.
    executePreRenderScript(tstart);

    return ROP_CONTINUE_RENDER;
}

namespace {

void
setKind( const string &path, UsdStagePtr stage )
{
    // When we are creating new geometry, the path prefix
    // parm specifies the root of our asset. This prim needs to be marked as a
    // component (model) and all its ancestors need to be marked group.
    //
    // Unless we are writing a group of references to other assets. This is the
    // case if our children are models.

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
    for( UsdPrim parent = model.GetPrim().GetParent(); parent.IsValid(); parent = parent.GetParent() ) {
        UsdModelAPI m( parent );
        if( m && !m.GetKind( &kind ))
            m.SetKind( KindTokens->group );
    }
}

} // close namespace

exint GusdROP_usdluxoutput::
collectLightNodes(OP_NodeList& lightNodeList, OP_NodeList& netNodeList, OP_Node* root)
{
    exint numAddedLights = 0;
    if (root->isSubNetwork(false) || root->isManager())
    {
        OP_NodeList localNodeList;
        root->getAllChildren(localNodeList);

        for (auto &node: localNodeList)
        {
            std::string nodeType = node->getOperator()->getName().c_str();
            numAddedLights += collectLightNodes(lightNodeList, netNodeList, node);
        }

        if (numAddedLights > 0)
        {
            netNodeList.insertAt(root, 0);
        }
    }
    else if (UsdLightWrapper::canBeWritten(root))
    {
        lightNodeList.append(root);
        ++numAddedLights;
    }

    return numAddedLights;
}

ROP_RENDER_CODE GusdROP_usdluxoutput::
renderFrame(fpreal time,
            UT_Interrupt* interupt)
{
    executePreFrameScript(time);

    const double frame = CHgetSampleFromTime(time);
    UsdTimeCode timeCode(frame);

    if( m_granularity == PER_FRAME ) {
        ROP_RENDER_CODE rv = openStage( time, (int)frame, (int)frame );
        if( rv != ROP_CONTINUE_RENDER )
            return rv;
    }

    OP_NodeList lightNodeList;
    OP_NodeList netNodeList;

    for (auto localRootNode: m_renderNodes)
    {
        collectLightNodes(lightNodeList, netNodeList, localRootNode);
    }

    for (auto& netNode: netNodeList)
    {
        UsdLightWrapper::write(m_usdStage, netNode, time, timeCode);
    }

    for (auto& lightNode: lightNodeList)
    {
        UsdLightWrapper::write(m_usdStage, lightNode, time, timeCode);
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


void GusdROP_usdluxoutput::resetState()
{
    m_usdStage = NULL;

    m_startFrame = 0;
    m_endFrame = 0;
    m_renderNodes.clear();

    m_fdTmpFile = -1;
}


ROP_RENDER_CODE GusdROP_usdluxoutput::
endRender()
{
    double endTimeCode = CHgetTimeFromFrame( m_endFrame );

    // Set the default prim path (to default or m_defaultPrimPath if set).
    if( m_granularity == ONE_FILE ) {
        if( !m_defaultPrimPath.empty() ) {
            setKind( m_defaultPrimPath, m_usdStage );

            SdfLayerHandle layer = m_usdStage->GetRootLayer();
            if( m_defaultPrimPath[0] == '/' && m_defaultPrimPath.find( '/', 1 ) == string::npos ) {
                SdfPrimSpecHandle defPrim = layer->GetPrimAtPath( SdfPath(m_defaultPrimPath) );
                if( defPrim ) {
                    layer->SetDefaultPrim( TfToken(m_defaultPrimPath.substr(1) ) );
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


ROP_RENDER_CODE GusdROP_usdluxoutput::
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

    for (const auto& prim : stage->GetPseudoRoot().
            GetFilteredChildren(UsdPrimIsDefined &&
                                !UsdPrimIsAbstract)) {
        prim.SetCustomDataByKey(TfToken("zUp"), VtValue(isZup));
        anySet = true;
    }
    return anySet;
}

} // close namespace

PXR_NAMESPACE_CLOSE_SCOPE
