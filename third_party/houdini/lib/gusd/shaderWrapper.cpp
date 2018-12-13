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

#include "context.h"

#include <pxr/usd/usdShade/connectableAPI.h>

#include <pxr/usd/usdRi/materialAPI.h>

#include <DEP/DEP_MicroNode.h>
#include <PRM/PRM_ParmList.h>
#include <PRM/PRM_SpareData.h>
#include <UT/UT_Map.h>
#include <UT/UT_Set.h>
#include <VOP/VOP_Node.h>
#include <VOP/VOP_Types.h>
#include <VOP/VOP_CodeGenerator.h>
#include <UT/UT_Version.h>
#include <UT/UT_EnvControl.h>
#include <UT/UT_FileUtil.h>

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
    if (!UT_FileUtil::makeDirs(shaderOutDir.c_str())) {
        TF_CODING_ERROR("Failed to create dir '%s'", shaderOutDir.c_str());
        return false;
    }

    const string shaderNameOsl = shaderName.toStdString() + ".osl";

    // Update shaderName to include the .oso extension.
    shaderName.append(".oso");

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
                     shaderName.toStdString() + " " + shaderNameOsl;
    std::shared_ptr<FILE> pipe(popen(oslcCmd.c_str(), "r"),
			       [](FILE *f){ pclose(f); });
    if (!pipe) {
        TF_CODING_ERROR("Failed to run command '%s'", oslcCmd.c_str());
        return false;
    }

    return true;
}

void _buildRampParmInputs(const PRM_Parm* parm, UsdShadeShader& shaderObject)
{
    std::string rampName(parm->getToken());
    bool isDefault = true;

    // Get the multiparm count. This will be divisible by 3 since each
    // point in the ramp will have a position, value and interp type.
    const int count = parm->getMultiParmCount() / 3;

    // Begin by extracting the list of positions. Set the list size to
    // (count + 2) so that an extra end-condition point can be copied
    // onto each end of the list.
    VtArray<fpreal32> positions(count + 2);
    for (int i = 0; i < count; i++) {
        PRM_Parm* position = parm->getMultiParm(i * 3);
        isDefault = position->isTrueFactoryDefault() && isDefault;

        position->getValues(0, &(positions[i + 1]), SYSgetSTID());
    }

    // If any positions are not set to factory defaults, store the
    // whole positions list as a shader input.
    if (!isDefault) {
        // Copy first and last positions onto ends of list.
        positions[0] = positions[1];
        positions[positions.size() - 1] = positions[positions.size() - 2];

        TfToken positionName(rampName + "_the_key_positions");
        UsdShadeInput positionsInput =
            shaderObject.CreateInput(positionName,
                                     SdfValueTypeNames->FloatArray);
        positionsInput.Set(positions);
    }

    // Next extract the list of values. Again keep track of whether all
    // the values are set to factory defaults.
    isDefault = true;

    // The values can either be colors or floats.
    if (parm->isRampTypeColor()) {
        // Color type. Set the list size to (count + 2) so that an extra
        // end-condition value can be copied onto each end of the list.
        VtArray<GfVec3f> values(count + 2);

        for (int i = 0; i < count; i++) {
            // The +1 in the getMultiParm index is because the i'th value
            // parm comes right after the i'th position parm.
            PRM_Parm* value = parm->getMultiParm((i * 3) + 1);
            isDefault = value->isTrueFactoryDefault() && isDefault;

            fpreal32 color[3];
            value->getValues(0, color, SYSgetSTID());
            values[i + 1].Set(color);
        }

        // If any values are not set to factory defaults, store the
        // whole values list as a shader input.
        if (!isDefault) {
            // Copy first and last color values onto ends of list.
            values[0] = values[1];
            values[values.size() - 1] = values[values.size() - 2];

            TfToken valueName(rampName + "_the_key_values");
            UsdShadeInput valuesInput =
                shaderObject.CreateInput(valueName,
                                         SdfValueTypeNames->Color3fArray);
            valuesInput.Set(values);
        }
    } else {
        // Float type. Set the list size to (count + 2) so that an extra
        // end-condition value can be copied onto each end of the list.
        VtArray<fpreal32> values(count + 2);

        for (int i = 0; i < count; i++) {
            // The +1 in the getMultiParm index is because the i'th value
            // parm comes right after the i'th position parm.
            PRM_Parm* value = parm->getMultiParm((i * 3) + 1);
            isDefault = value->isTrueFactoryDefault() && isDefault;

            value->getValues(0, &(values[i + 1]), SYSgetSTID());
        }

        // If any values are not set to factory defaults, store the
        // whole values list as a shader input.
        if (!isDefault) {
            // Copy first and last float values onto ends of list.
            values[0] = values[1];
            values[values.size() - 1] = values[values.size() - 2];

            TfToken valueName(rampName + "_the_key_values");
            UsdShadeInput valuesInput =
                shaderObject.CreateInput(valueName,
                                         SdfValueTypeNames->FloatArray);
            valuesInput.Set(values);
        }
    }

    // Lastly, extract the interp type. Even though there is a separate
    // interp parm provided with each position and value, the shader only
    // uses the first. Use a getMultiParm index of 2 because the first
    // interp parm comes right after the first position and value parms.
    PRM_Parm* interp = parm->getMultiParm(2);

    // If the parm is not set to its factory default,
    // store it as a shader input.
    if (!interp->isTrueFactoryDefault()) {
        UT_String str;
        interp->getValue(0, str, 0, true, SYSgetSTID());

        TfToken interpName(rampName + "_the_basis_strings");
        UsdShadeInput interpInput =
            shaderObject.CreateInput(interpName, SdfValueTypeNames->String);
        interpInput.Set(str.toStdString());
    }
}

UsdShadeShader
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

    _buildCustomOslNode(vopNode, shaderName, shaderOutDir);

    const VtValue pathAttr(TfToken(shaderName.toStdString()));

    const SdfPath shaderObjectPath = lookPath.AppendPath(SdfPath(nodeName));
    UsdShadeShader shaderObject;

    // Check if this node has already been visited. If so, retrieve
    // its corresponding UsdRiRisObject from the usd stage.
    if(visitedVops.find(nodeName) != visitedVops.end()) {
        shaderObject = UsdShadeShader(stage->GetPrimAtPath(shaderObjectPath));
    } else {
        // The node hasn't been visited yet.
        shaderObject = UsdShadeShader::Define(stage, shaderObjectPath);
        shaderObject.CreateIdAttr(pathAttr);
        visitedVops.insert(nodeName);
    }

    if(!shaderObject.GetPrim().IsValid()) {
        TF_CODING_ERROR("Error creating or retrieving USD prim '%s'.",
                         nodeName.c_str());
        return UsdShadeShader();
    }

    //
    // Iterate through each parameter in vopNode's parm list.
    //
    const PRM_ParmList* nodeParams = vopNode->getParmList();         
    for (int i=0; i<nodeParams->getEntries(); ++i) {

        const PRM_Parm* parm = nodeParams->getParmPtr(i);

        //
        // Handle ramp parameters as a special-case. (Their multi-parms
        // need to be processed all together rather than individually).
        //
        if (parm->isRampType()) {
            _buildRampParmInputs(parm, shaderObject);
            continue;
        }

        int inIdx = vopNode->getInputFromName(parm->getToken());
        bool isConnected = inIdx >= 0
                           && const_cast<VOP_Node*>(vopNode)->isConnected(inIdx, true);

        //
        // Create a UsdShadeInput for each connected input, and
        // for each parameter that is set to a non-default value.
        //
        if (isConnected || !parm->isTrueFactoryDefault()) {
            const char* type = NULL;

            // Figure out if this parm has spare data named "script_ritype".
            // (This would have been added to this parm when the otl/hda for
            // this vopNode was generated.)
            if (auto* spare = parm->getSparePtr()) {
                type = spare->getValue("script_ritype");
            }

            if (type == NULL) {
                continue;
            }

            //
            // For each type, multiparms are treated the same as parms,
            // ie. int and int[] are handled as the same type; float and
            // float[], string and string[], etc.
            // This works because a multiparm doesn't actually "contain" its
            // elements; instead, its elements just show up as regular parms
            // alongside the multiparm. Thus, this loop will process all parms.
            //
            // TODO: This will probably need to be changed at some point so
            // that instead of multiparms and their element parms being handled
            // individually, some mechanism needs to be put in place to combine
            // these elements into a single array.
            //
            if (strcmp(type, "string") == 0 ||
                strcmp(type, "string[]") == 0) {

                UsdShadeInput shadeInput; 

                // If the parameter is a menu, it may be a string which represents
                // a numeric value.
                // TODO Add metadata to the parameter templates to distinguish this
                //     case.
                UT_String str;
                parm->getValue(0, str, 0, true, SYSgetSTID());
                bool isInteger = parm->getChoiceListPtr() && str.isInteger(); 

                if(isInteger) {
                    shadeInput = shaderObject.CreateInput(TfToken(parm->getToken()),
                                                    SdfValueTypeNames->Int);

                }
                else {
                    shadeInput = shaderObject.CreateInput(TfToken(parm->getToken()),
                                                    SdfValueTypeNames->String);
                }

                parmDeps[parm] = shadeInput;

                if(!isConnected) {
                    if(isInteger) {
                        shadeInput.Set(str.toInt());
                    }
                    else {
                        shadeInput.Set(str.toStdString());
                    }
                }
            }
            else if (strcmp(type, "float") == 0 ||
                     strcmp(type, "float[]") == 0) {

                UsdShadeInput shadeInput 
                    = shaderObject.CreateInput(TfToken(parm->getToken()),
                                            SdfValueTypeNames->Float);
                parmDeps[parm] = shadeInput;

                if(!isConnected) {
                    fpreal val;
                    parm->getValue(0, val, 0, SYSgetSTID());
                    shadeInput.Set((float)val);
                }
            }
            else if (strcmp(type, "int") == 0 ||
                     strcmp(type, "int[]") == 0) {

                UsdShadeInput shadeInput 
                    = shaderObject.CreateInput(TfToken(parm->getToken()),
                                            SdfValueTypeNames->Int);
                parmDeps[parm] = shadeInput;

                if(!isConnected) {
                    int val;   
                    parm->getValue(0, val, 0, SYSgetSTID());
                    shadeInput.Set(val);
                }
            }
            else if (strcmp(type, "color") == 0 ||
                     strcmp(type, "color[]") == 0) {

                UsdShadeInput shadeInput 
                    = shaderObject.CreateInput(TfToken(parm->getToken()),
                                            SdfValueTypeNames->Color3f);
                parmDeps[parm] = shadeInput;

                if(!isConnected) {
                    fpreal32 color[3];   
                    parm->getValues(0, color, SYSgetSTID());
                    shadeInput.Set(GfVec3f(color));
                }
            }
            else if (strcmp(type, "normal") == 0 ||
                     strcmp(type, "normal[]") == 0) {

                UsdShadeInput shadeInput 
                    = shaderObject.CreateInput(TfToken(parm->getToken()),
                                            SdfValueTypeNames->Normal3f);
                parmDeps[parm] = shadeInput;

                if(!isConnected) {
                    fpreal32 normal[3];   
                    parm->getValues(0, normal, SYSgetSTID());
                    shadeInput.Set(GfVec3f(normal));
                }
            }
            else if (strcmp(type, "point") == 0 ||
                     strcmp(type, "point[]") == 0) {

                UsdShadeInput shadeInput 
                    = shaderObject.CreateInput(TfToken(parm->getToken()),
                                            SdfValueTypeNames->Point3f);
                parmDeps[parm] = shadeInput;

                if(!isConnected) {
                    fpreal32 point[3];   
                    parm->getValues(0, point, SYSgetSTID());
                    shadeInput.Set(GfVec3f(point));
                }
            }
            else if (strcmp(type, "vector") == 0 ||
                     strcmp(type, "vector[]") == 0) {

                UsdShadeInput shadeInput 
                    = shaderObject.CreateInput(TfToken(parm->getToken()),
                                            SdfValueTypeNames->Vector3f);
                parmDeps[parm] = shadeInput;

                if(!isConnected) {
                    fpreal32 vector[3];   
                    parm->getValues(0, vector, SYSgetSTID());
                    shadeInput.Set(GfVec3f(vector));
                }
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

            UsdShadeShader inputPrim = _vopGraphToUsdTraversal(inputVop,
                                                               stage,
                                                               lookPath,
                                                               visitedVops,
                                                               parmDeps,
                                                               shaderOutDir);

            UT_String inputName, outputName;
            vopNode->getInputName(inputName, inIdx);
            inputVop->getOutputName(outputName,outIdx);

            UsdShadeInput shadeInput
                = shaderObject.GetInput(TfToken(inputName.toStdString()));
            UsdShadeConnectableAPI::ConnectToSource(shadeInput, 
                inputPrim, TfToken(outputName.toStdString()));

        }
    }

    return shaderObject;
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


    SdfPath bxdfPath = m_usdMaterial.GetPath().AppendChild(TfToken(
        terminalNode->getName().toStdString()));

    auto riMatAPI = UsdRiMaterialAPI::Apply(m_usdMaterial.GetPrim());
    riMatAPI.SetSurfaceSource(bxdfPath);
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

