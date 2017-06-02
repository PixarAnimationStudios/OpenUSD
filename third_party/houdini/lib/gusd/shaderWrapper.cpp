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
#include "shaderWrapper.h"

#include "Context.h"

#include <pxr/usd/usdShade/connectableAPI.h>

#include <pxr/usd/usdRi/materialAPI.h>
#include <pxr/usd/usdRi/risBxdf.h>
#include <pxr/usd/usdRi/risPattern.h>
#include <pxr/usd/usdRi/risOslPattern.h>

#include <DEP/DEP_MicroNode.h>
#include <PRM/PRM_ParmList.h>
#include <UT/UT_Map.h>
#include <UT/UT_Set.h>
#include <VOP/VOP_Node.h>
#include <VOP/VOP_Types.h>
#include <VOP/VOP_CodeGenerator.h>
#include <UT/UT_Version.h>
#include <UT/UT_EnvControl.h>

#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>

PXR_NAMESPACE_OPEN_SCOPE

using std::cout;
using std::cerr;
using std::endl;
using std::string;

namespace {

typedef UT_Set<string> VopSet;
typedef UT_Map<const PRM_Parm*, UsdShadeInput> ParmDepMap;


bool
_buildCustomOslNode(const VOP_Node* vopNode,
                    UT_String& shaderName,
                    const std::string& shaderOutDir)
{
    // If shaderName doesn't begin with "op:", it's not
    // a custom osl node. Just exit this function.
    if (!shaderName.startsWith("op:")) {
        return false;
    }

    shaderName.replacePrefix("op:", "");
    shaderName.forceAlphaNumeric(); // replaces each '/' with '_'
    shaderName.prepend(shaderOutDir.c_str());

    // Create shaderOutDir path directories, if they don't exist.
    try {
        boost::filesystem::create_directories(shaderOutDir);
    } catch (boost::filesystem::filesystem_error &e) {
        TF_CODING_ERROR("Failed to create dir '%s': %s", shaderOutDir.c_str(),
                        e.what());
        return false;
    }

    const string shaderNameOsl = shaderName.toStdString() + ".osl";
    const string shaderNameOso = shaderName.toStdString() + ".oso";

    std::ofstream oslFile;
    oslFile.open(shaderNameOsl, std::ios::out);

    VOP_CodeGenerator *cg =
        const_cast<VOP_Node*>(vopNode)->getVopCodeGenerator();
    cg->outputVflCode(oslFile, NULL, VEX_CG_DEFAULT,
                      vopNode->getOslContextType());
    oslFile.close();

    string houRoot(UT_EnvControl::getString(ENV_HFS));
    string houErr = shaderOutDir + "err.log";
    string houInclude = houRoot + "/houdini/osl/include";

    string oslcCmd = houRoot + "/bin/hrmanshader -e " + houErr +
                     " -cc oslc -I" + houInclude + " -o " +
                     shaderNameOso + " " + shaderNameOsl;
    std::shared_ptr<FILE> pipe(popen(oslcCmd.c_str(), "r"), pclose);
    if (!pipe) {
        TF_CODING_ERROR("Failed to run command '%s'", oslcCmd.c_str());
        return false;
    }

    return true;
}

UsdRiRisObject
_vopGraphToUsdTraversal(const VOP_Node* vopNode,
                        UsdStagePtr& stage,
                        const SdfPath& lookPath,
                        VopSet& visitedVops,
                        ParmDepMap& parmDeps,
                        const string& shaderOutDir)
{
    const string nodeName = vopNode->getName().toStdString();
    const VOP_Type nodeType = vopNode->getShaderType();

    UT_String shaderName(vopNode->getShaderName(false, nodeType));

    bool isCustomOsl =
        _buildCustomOslNode(vopNode, shaderName, shaderOutDir);

    const VtValue pathAttr(SdfAssetPath(shaderName.toStdString()));

    const SdfPath risObjectPath = lookPath.AppendPath(SdfPath(nodeName));
    UsdRiRisObject risObject;

    // Check if this node has already been visited. If so, retrieve
    // its corresponding UsdRiRisObject from the usd stage.
    if(visitedVops.find(nodeName) != visitedVops.end()) {
        risObject = UsdRiRisObject(stage->GetPrimAtPath(risObjectPath));
    } else {
        // The node hasn't been visited yet.
        if (isCustomOsl) {
            UsdRiRisOslPattern oslPattern =
                UsdRiRisOslPattern::Define(stage, risObjectPath);
            oslPattern.CreateOslPathAttr(pathAttr);
            risObject = oslPattern;
        } else {
            switch (nodeType) {
                case VOP_BSDF_SHADER:
                    risObject = UsdRiRisBxdf::Define(stage, risObjectPath);
                    break;
                case VOP_GENERIC_SHADER:
                    risObject = UsdRiRisPattern::Define(stage, risObjectPath);
                    break;
                default:
                    break;
            }
            risObject.CreateFilePathAttr(pathAttr);
        }
        visitedVops.insert(nodeName);
    }

    if(!risObject.GetPrim().IsValid()) {
        TF_CODING_ERROR("Error creating or retrieving USD prim '%s'.",
                         nodeName.c_str());
        return UsdRiRisObject();
    }

    //
    // Create UsdShadeInputs for input connections, inputs which are set
    // to a non-default value, and inputs which have channel dependencies.
    //
    const PRM_ParmList* nodeParams = vopNode->getParmList();         
    for(int i=0; i<nodeParams->getEntries(); ++i) {

        const PRM_Parm* parm = nodeParams->getParmPtr(i);

        int inIdx = vopNode->getInputFromName(parm->getToken());
        bool isConnected = inIdx >= 0
                           && const_cast<VOP_Node*>(vopNode)->isConnected(inIdx, true);

        //bool isDependent = parm->hasMicroNodes();

        if(isConnected || !parm->isTrueFactoryDefault()) {
            const PRM_Type& parmType = parm->getType();
            if(parmType.isStringType()) {

                UsdShadeInput shadeInput; 

                // If the parameter is a menu, it may be a string which represents
                // a numeric value.
                // TODO Add metadata to the parameter templates to distinguish this
                //     case.
                UT_String str;
                parm->getValue(0, str, 0, true, SYSgetSTID());
                bool isInteger = parm->getChoiceListPtr() && str.isInteger(); 

                if(isInteger) {
                    shadeInput = risObject.CreateInput(TfToken(parm->getToken()),
                                                    SdfValueTypeNames->Int);

                }
                else {
                    shadeInput = risObject.CreateInput(TfToken(parm->getToken()),
                                                    SdfValueTypeNames->String);
                }

                //if(isDependent) {
                    parmDeps[parm] = shadeInput;
                //}

                if(!isConnected) {
                    if(isInteger) {
                        shadeInput.Set(str.toInt());
                    }
                    else {
                        shadeInput.Set(str.toStdString());
                    }
                }
            }
            else if(parmType.isFloatType() &&
                    parmType.getFloatType() == PRM_Type::PRM_FLOAT_NONE) {

                UsdShadeInput shadeInput 
                    = risObject.CreateInput(TfToken(parm->getToken()),
                                            SdfValueTypeNames->Float);
                //if(isDependent) {
                    parmDeps[parm] = shadeInput;
                //}

                if(!isConnected) {
                    double val;   
                    parm->getValue(0, val, 0, SYSgetSTID());
                    shadeInput.Set((float)val);
                }
            }
            else if(parmType.hasFloatType(PRM_Type::PRM_FLOAT_INTEGER) ||
                    parmType.hasOrdinalType(PRM_Type::PRM_ORD_TOGGLE)) {

                UsdShadeInput shadeInput 
                    = risObject.CreateInput(TfToken(parm->getToken()),
                                            SdfValueTypeNames->Int);
                //if(isDependent) {
                    parmDeps[parm] = shadeInput;
                //}

                if(!isConnected) {
                    int val;   
                    parm->getValue(0, val, 0, SYSgetSTID());
                    shadeInput.Set(val);
                }
            }
            else if(parmType.hasFloatType(PRM_Type::PRM_FLOAT_RGBA)) {

                UsdShadeInput shadeInput 
                    = risObject.CreateInput(TfToken(parm->getToken()),
                                            SdfValueTypeNames->Color3d);
                //if(isDependent) {
                    parmDeps[parm] = shadeInput;
                //}

                if(!isConnected) {
                    double color[3];   
                    parm->getValues(0, color, SYSgetSTID());
                    shadeInput.Set(GfVec3d(color));
                }
            }
            else if(PRM_XYZ == parmType) {
                // TODO Does parm need additional metadata to distinguish
                //      vector/point/normal ?
            }
        }
    }

    //
    // Add connected nodes in depth-first order
    //
    for(int inIdx=0; inIdx<vopNode->nInputs(); ++inIdx) {
        // XXX Assuming the need for const_casts below is oversight in
        //     VOP API.
        bool hasInput = const_cast<VOP_Node*>(vopNode)->isConnected(inIdx, true);
        if(hasInput) {
            VOP_Node* inputVop
                = const_cast<VOP_Node*>(vopNode)->findSimpleInput(inIdx);
            int outIdx
                = inputVop->whichOutputIs(const_cast<VOP_Node*>(vopNode), inIdx);

            UsdRiRisObject inputPrim = _vopGraphToUsdTraversal(inputVop,
                                                               stage,
                                                               lookPath,
                                                               visitedVops,
                                                               parmDeps,
                                                               shaderOutDir);

            UT_String inputName, outputName;
            vopNode->getInputName(inputName, inIdx);
            inputVop->getOutputName(outputName,outIdx);

            UsdShadeInput shadeInput
                = risObject.GetInput(TfToken(inputName.toStdString()));
            UsdShadeConnectableAPI::ConnectToSource(shadeInput, 
                inputPrim, TfToken(outputName.toStdString()));

        }
    }

    return risObject;
}


void
_vopGraphToUsd(const VOP_Node* vopNode,
               UsdShadeMaterial& material,
               const ParmDepMap& parmDeps)
{
    // Look for interface inputs.
    // We iterate through the vop creator's paramters and see if any of our
    // vop graph's paramters are dependents.
    const OP_Node* creatorNode = vopNode->getCreator();
    const PRM_ParmList* creatorParams = creatorNode->getParmList();  

    UsdRiMaterialAPI materialAPI(material);

    for(int i=0; i<creatorParams->getEntries(); ++i) {

        const PRM_Parm* parm = creatorParams->getParmPtr(i);

        UT_ValArray<PRM_Parm*> curDeps;
        UT_IntArray curDepSubIdxs;
        const_cast<OP_Node*>(creatorNode)->getParmsThatReference(
                parm->getToken(), curDeps, curDepSubIdxs);

        UsdShadeInput interfaceInput;

        for(UT_ValArray<PRM_Parm*>::const_iterator curDepIt=curDeps.begin();
                curDepIt != curDeps.end(); ++curDepIt) {

            auto parmDepIt = parmDeps.find(*curDepIt);
            if(parmDepIt != parmDeps.end()) {
                const UsdShadeInput& shadeInput = parmDepIt->second;

                if(!interfaceInput.IsDefined()) {
                    interfaceInput = material.CreateInput(
                            TfToken(parm->getToken()),
                            shadeInput.GetAttr().GetTypeName());
                }
                
                UsdShadeConnectableAPI::ConnectToSource(shadeInput, 
                    interfaceInput);
            }
        }
    }
}

} // anon namespace


GusdShaderWrapper::
GusdShaderWrapper(const VOP_Node* terminalNode,
                  const UsdStagePtr& stage,
                  const std::string& path,
                  const std::string& shaderOutDir)
{
    m_usdMaterial = UsdShadeMaterial::Define(stage, SdfPath(path));
    m_shaderOutDir = shaderOutDir;

    // Make sure m_shaderOutDir ends with "/".
    if (!m_shaderOutDir.empty() && m_shaderOutDir.back() != '/') {
        m_shaderOutDir += "/";
    }

    buildLook(terminalNode);
}


void GusdShaderWrapper::
buildLook(const VOP_Node* terminalNode)
{
    if(!isValid()) {
        TF_CODING_ERROR("Usd look prim isn't valid. Can't create shader '%s'.",
                terminalNode->getFullPath().buffer());
        return;
    }

    if(terminalNode->getShaderType() != VOP_BSDF_SHADER) {
        TF_CODING_ERROR("Assigned shader node must be a bxdf. Can't create shader '%s'.",
                terminalNode->getFullPath().buffer());
        return;
    }

    SdfPath bxdfPath = m_usdMaterial.GetPath().AppendChild(TfToken(
        terminalNode->getName().toStdString()));

    UsdRiMaterialAPI(m_usdMaterial).SetBxdfSource(bxdfPath);

    UsdStagePtr stage = m_usdMaterial.GetPrim().GetStage();
    VopSet visitedVops;
    ParmDepMap parmDeps;

    _vopGraphToUsdTraversal(terminalNode,
                            stage,
                            m_usdMaterial.GetPath(),
                            visitedVops,
                            parmDeps,
                            m_shaderOutDir);

    _vopGraphToUsd(terminalNode, m_usdMaterial, parmDeps);
}


bool GusdShaderWrapper::
bind(UsdPrim& prim) const
{
    if(!isValid())
        return false;

    return m_usdMaterial.Bind(prim);
}


bool GusdShaderWrapper::
isValid() const
{
    return m_usdMaterial.GetPrim().IsValid();
}

PXR_NAMESPACE_CLOSE_SCOPE

