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
#include "px_vp20/utils.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/imaging/garch/gl.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/glf/simpleMaterial.h"

#include <maya/MColor.h>
#include <maya/MDrawContext.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVector.h>
#include <maya/MIntArray.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MStatus.h>

#include <cmath>


bool px_vp20Utils::setupLightingGL( const MHWRender::MDrawContext& context)
{
    MStatus status;
    
    // Take into account only the 8 lights supported by the basic
    // OpenGL profile.
    const unsigned int nbLights =
        std::min(context.numberOfActiveLights(&status), 8u);
    if (status != MStatus::kSuccess) return false;

    if (nbLights > 0) {
        // Lights are specified in world space and needs to be
        // converted to view space.
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        const MMatrix worldToView =
            context.getMatrix(MHWRender::MDrawContext::kViewMtx, &status);
        if (status != MStatus::kSuccess) return false;
        glLoadMatrixd(worldToView.matrix[0]);

        glEnable(GL_LIGHTING);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glEnable(GL_COLOR_MATERIAL) ;
        glEnable(GL_NORMALIZE) ;

        
        {
            const GLfloat ambient[4]  = { 0.0f, 0.0f, 0.0f, 1.0f };
            const GLfloat specular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  ambient);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);

            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

            glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
        }

        for (unsigned int i=0; i<nbLights; ++i) {
            MFloatVector direction;
            float intensity;
            MColor color;
            bool hasDirection;
            bool hasPosition;
#if MAYA_API_VERSION >= 201300
            // Starting with Maya 2013, getLightInformation() uses MFloatPointArray for positions
            MFloatPointArray positions;
            status = context.getLightInformation(
                i, positions, direction, intensity, color,
                hasDirection, hasPosition);
            const MFloatPoint &position = positions[0];
#else 
            // Maya 2012, getLightInformation() uses MFloatPoint for position
            MFloatPoint position;
            status = context.getLightInformation(
                i, position, direction, intensity, color,
                hasDirection, hasPosition);
#endif
            if (status != MStatus::kSuccess) return false;

            if (hasDirection) {
                if (hasPosition) {
                    // Assumes a Maya Spot Light!
                    const GLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                    const GLfloat diffuse[4] = { intensity * color[0],
                                              intensity * color[1],
                                              intensity * color[2],
                                              1.0f };
                    const GLfloat pos[4] = { position[0],
                                              position[1],
                                              position[2],
                                              1.0f };
                    const GLfloat dir[3] = { direction[0],
                                              direction[1],
                                              direction[2]};
                        
                            
                    glLightfv(GL_LIGHT0+i, GL_AMBIENT,  ambient);
                    glLightfv(GL_LIGHT0+i, GL_DIFFUSE,  diffuse);
                    glLightfv(GL_LIGHT0+i, GL_POSITION, pos);
                    glLightfv(GL_LIGHT0+i, GL_SPOT_DIRECTION, dir);

                    // Maya's default value's for spot lights.
                    glLightf(GL_LIGHT0+i,  GL_SPOT_EXPONENT, 0.0);
                    glLightf(GL_LIGHT0+i,  GL_SPOT_CUTOFF,  20.0);
                }
                else {
                    // Assumes a Maya Directional Light!
                    const GLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                    const GLfloat diffuse[4] = { intensity * color[0],
                                                  intensity * color[1],
                                                  intensity * color[2],
                                                  1.0f };
                    const GLfloat pos[4] = { -direction[0],
                                              -direction[1],
                                              -direction[2],
                                              0.0f };
                        
                            
                    glLightfv(GL_LIGHT0+i, GL_AMBIENT,  ambient);
                    glLightfv(GL_LIGHT0+i, GL_DIFFUSE,  diffuse);
                    glLightfv(GL_LIGHT0+i, GL_POSITION, pos);
                    glLightf(GL_LIGHT0+i, GL_SPOT_CUTOFF, 180.0);
                }
            }
            else if (hasPosition) {
                // Assumes a Maya Point Light!
                const GLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                const GLfloat diffuse[4] = { intensity * color[0],
                                              intensity * color[1],
                                              intensity * color[2],
                                              1.0f };
                const GLfloat pos[4] = { position[0],
                                          position[1],
                                          position[2],
                                          1.0f };
                        
                            
                glLightfv(GL_LIGHT0+i, GL_AMBIENT,  ambient);
                glLightfv(GL_LIGHT0+i, GL_DIFFUSE,  diffuse);
                glLightfv(GL_LIGHT0+i, GL_POSITION, pos);
                glLightf(GL_LIGHT0+i, GL_SPOT_CUTOFF, 180.0);
            }
            else {
                // Assumes a Maya Ambient Light!
                const GLfloat ambient[4] = { intensity * color[0],
                                              intensity * color[1],
                                              intensity * color[2],
                                              1.0f };
                const GLfloat diffuse[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                const GLfloat pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                        
                            
                glLightfv(GL_LIGHT0+i, GL_AMBIENT,  ambient);
                glLightfv(GL_LIGHT0+i, GL_DIFFUSE,  diffuse);
                glLightfv(GL_LIGHT0+i, GL_POSITION, pos);
                glLightf(GL_LIGHT0+i, GL_SPOT_CUTOFF, 180.0);
            }

            glEnable(GL_LIGHT0+i);
        }
        glPopMatrix();
    }

    glDisable(GL_LIGHTING);
    return nbLights > 0;
}


//------------------------------------------------------------------------------
//
void px_vp20Utils::unsetLightingGL( const MHWRender::MDrawContext& context)
{
    MStatus status;
    
    // Take into account only the 8 lights supported by the basic
    // OpenGL profile.
    const unsigned int nbLights =
        std::min(context.numberOfActiveLights(&status), 8u);
    if (status != MStatus::kSuccess) return;

    // Restore OpenGL default values for anything that we have
    // modified.

    if (nbLights > 0) {
        for (unsigned int i=0; i<nbLights; ++i) {
            glDisable(GL_LIGHT0+i);
            
            const GLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            glLightfv(GL_LIGHT0+i, GL_AMBIENT,  ambient);

            if (i==0) {
                const GLfloat diffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glLightfv(GL_LIGHT0+i, GL_DIFFUSE,  diffuse);

                const GLfloat spec[4]    = { 1.0f, 1.0f, 1.0f, 1.0f };
                glLightfv(GL_LIGHT0+i, GL_SPECULAR, spec);
            }
            else {
                const GLfloat diffuse[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                glLightfv(GL_LIGHT0+i, GL_DIFFUSE,  diffuse);

                const GLfloat spec[4]    = { 0.0f, 0.0f, 0.0f, 1.0f };
                glLightfv(GL_LIGHT0+i, GL_SPECULAR, spec);
            }
            
            const GLfloat pos[4]     = { 0.0f, 0.0f, 1.0f, 0.0f };
            glLightfv(GL_LIGHT0+i, GL_POSITION, pos);
            
            const GLfloat dir[3]     = { 0.0f, 0.0f, -1.0f };
            glLightfv(GL_LIGHT0+i, GL_SPOT_DIRECTION, dir);
            
            glLightf(GL_LIGHT0+i,  GL_SPOT_EXPONENT,  0.0);
            glLightf(GL_LIGHT0+i,  GL_SPOT_CUTOFF,  180.0);
        }
        
        glDisable(GL_LIGHTING);
        glDisable(GL_COLOR_MATERIAL) ;
        glDisable(GL_NORMALIZE) ;

        const GLfloat ambient[4]  = { 0.2f, 0.2f, 0.2f, 1.0f };
        const GLfloat specular[4] = { 0.8f, 0.8f, 0.8f, 1.0f };

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);

        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
        

    }
}

static
bool
_GetLightingParam(
        const MIntArray& intValues,
        const MFloatArray& floatValues,
        bool& paramValue)
{
    bool gotParamValue = false;

    if (intValues.length() > 0) {
        paramValue = (intValues[0] == 1);
        gotParamValue = true;
    } else if (floatValues.length() > 0) {
        paramValue = GfIsClose(floatValues[0], 1.0f, 1e-5);
        gotParamValue = true;
    }

    return gotParamValue;
}

static
bool
_GetLightingParam(
        const MIntArray& intValues,
        const MFloatArray& floatValues,
        float& paramValue)
{
    bool gotParamValue = false;

    if (floatValues.length() > 0) {
        paramValue = floatValues[0];
        gotParamValue = true;
    }

    return gotParamValue;
}

static
bool
_GetLightingParam(
        const MIntArray& intValues,
        const MFloatArray& floatValues,
        GfVec2f& paramValue)
{
    bool gotParamValue = false;

    if (intValues.length() >= 2) {
        paramValue[0] = intValues[0];
        paramValue[1] = intValues[1];
        gotParamValue = true;
    } else if (floatValues.length() >= 2) {
        paramValue[0] = floatValues[0];
        paramValue[1] = floatValues[1];
        gotParamValue = true;
    }

    return gotParamValue;
}

static
bool
_GetLightingParam(
        const MIntArray& intValues,
        const MFloatArray& floatValues,
        GfVec3f& paramValue)
{
    bool gotParamValue = false;

    if (intValues.length() >= 3) {
        paramValue[0] = intValues[0];
        paramValue[1] = intValues[1];
        paramValue[2] = intValues[2];
        gotParamValue = true;
    } else if (floatValues.length() >= 3) {
        paramValue[0] = floatValues[0];
        paramValue[1] = floatValues[1];
        paramValue[2] = floatValues[2];
        gotParamValue = true;
    }

    return gotParamValue;
}

static
bool
_GetLightingParam(
        const MIntArray& intValues,
        const MFloatArray& floatValues,
        GfVec4f& paramValue)
{
    bool gotParamValue = false;

    if (intValues.length() >= 3) {
        paramValue[0] = intValues[0];
        paramValue[1] = intValues[1];
        paramValue[2] = intValues[2];
        if (intValues.length() > 3) {
            paramValue[3] = intValues[3];
        }
        gotParamValue = true;
    } else if (floatValues.length() >= 3) {
        paramValue[0] = floatValues[0];
        paramValue[1] = floatValues[1];
        paramValue[2] = floatValues[2];
        if (floatValues.length() > 3) {
            paramValue[3] = floatValues[3];
        }
        gotParamValue = true;
    }

    return gotParamValue;
}

/* static */
GlfSimpleLightingContextRefPtr
px_vp20Utils::GetLightingContextFromDrawContext(
        const MHWRender::MDrawContext& context)
{
    const GfVec4f blackColor(0.0f, 0.0f, 0.0f, 1.0f);
    const GfVec4f whiteColor(1.0f, 1.0f, 1.0f, 1.0f);

    GlfSimpleLightingContextRefPtr lightingContext =
        GlfSimpleLightingContext::New();

    MStatus status;

    unsigned int numMayaLights =
        context.numberOfActiveLights(MHWRender::MDrawContext::kFilteredToLightLimit,
                                     &status);
    if (status != MS::kSuccess || numMayaLights < 1) {
        return lightingContext;
    }

    bool viewDirectionAlongNegZ = context.viewDirectionAlongNegZ(&status);
    if (status != MS::kSuccess) {
        // If we fail to find out the view direction for some reason, assume
        // that it's along the negative Z axis (OpenGL).
        viewDirectionAlongNegZ = true;
    }

    GlfSimpleLightVector lights;

    for (unsigned int i = 0; i < numMayaLights; ++i) {
        MHWRender::MLightParameterInformation* mayaLightParamInfo =
            context.getLightParameterInformation(i);

        if (!mayaLightParamInfo) {
            continue;
        }

        // Setup some default values before we read the light parameters.
        bool lightEnabled = true;

        bool    lightHasPosition = false;
        GfVec4f lightPosition(0.0f, 0.0f, 0.0f, 1.0f);
        bool    lightHasDirection = false;
        GfVec3f lightDirection(0.0f, 0.0f, -1.0f);
        if (!viewDirectionAlongNegZ) {
            // The convention for DirectX is positive Z.
            lightDirection[2] = 1.0f;
        }

        float   lightIntensity = 1.0f;
        GfVec4f lightColor = blackColor;
        bool    lightEmitsDiffuse = true;
        bool    lightEmitsSpecular = false;
        float   lightDecayRate = 0.0f;
        float   lightDropoff = 0.0f;
        // The cone angle is 180 degrees by default.
        GfVec2f lightCosineConeAngle(-1.0f);
        float   lightShadowBias = 0.0f;
        bool    lightShadowOn = false;

        bool globalShadowOn = false;

        MStringArray paramNames;
        mayaLightParamInfo->parameterList(paramNames);

        for (unsigned int paramIndex = 0; paramIndex < paramNames.length(); ++paramIndex) {
            const MString paramName = paramNames[paramIndex];
            const MHWRender::MLightParameterInformation::ParameterType paramType =
                mayaLightParamInfo->parameterType(paramName);
            const MHWRender::MLightParameterInformation::StockParameterSemantic paramSemantic =
                mayaLightParamInfo->parameterSemantic(paramName);

            MIntArray intValues;
            MFloatArray floatValues;

            switch (paramType) {
                case MHWRender::MLightParameterInformation::kBoolean:
                case MHWRender::MLightParameterInformation::kInteger:
                    mayaLightParamInfo->getParameter(paramName, intValues);
                    break;
                case MHWRender::MLightParameterInformation::kFloat:
                case MHWRender::MLightParameterInformation::kFloat2:
                case MHWRender::MLightParameterInformation::kFloat3:
                case MHWRender::MLightParameterInformation::kFloat4:
                    mayaLightParamInfo->getParameter(paramName, floatValues);
                    break;
                default:
                    // Unsupported paramType.
                    continue;
                    break;
            }

            switch (paramSemantic) {
                case MHWRender::MLightParameterInformation::kLightEnabled:
                    _GetLightingParam(intValues, floatValues, lightEnabled);
                    break;
                case MHWRender::MLightParameterInformation::kWorldPosition:
                    if (_GetLightingParam(intValues, floatValues, lightPosition)) {
                        lightHasPosition = true;
                    }
                    break;
                case MHWRender::MLightParameterInformation::kWorldDirection:
                    if (_GetLightingParam(intValues, floatValues, lightDirection)) {
                        lightHasDirection = true;
                    }
                    break;
                case MHWRender::MLightParameterInformation::kIntensity:
                    _GetLightingParam(intValues, floatValues, lightIntensity);
                    break;
                case MHWRender::MLightParameterInformation::kColor:
                    _GetLightingParam(intValues, floatValues, lightColor);
                    break;
                case MHWRender::MLightParameterInformation::kEmitsDiffuse:
                    _GetLightingParam(intValues, floatValues, lightEmitsDiffuse);
                    break;
                case MHWRender::MLightParameterInformation::kEmitsSpecular:
                    _GetLightingParam(intValues, floatValues, lightEmitsSpecular);
                    break;
                case MHWRender::MLightParameterInformation::kDecayRate:
                    _GetLightingParam(intValues, floatValues, lightDecayRate);
                    break;
                case MHWRender::MLightParameterInformation::kDropoff:
                    _GetLightingParam(intValues, floatValues, lightDropoff);
                    break;
                case MHWRender::MLightParameterInformation::kCosConeAngle:
                    _GetLightingParam(intValues, floatValues, lightCosineConeAngle);
                    break;
                case MHWRender::MLightParameterInformation::kShadowBias:
                    _GetLightingParam(intValues, floatValues, lightShadowBias);
                    break;
                case MHWRender::MLightParameterInformation::kShadowOn:
                    _GetLightingParam(intValues, floatValues, lightShadowOn);
                    break;
                case MHWRender::MLightParameterInformation::kGlobalShadowOn:
                    _GetLightingParam(intValues, floatValues, globalShadowOn);
                    break;
                default:
                    // Unsupported paramSemantic.
                    continue;
                    break;
            }

            if (!lightEnabled) {
                // Stop reading light parameters if the light is disabled.
                break;
            }
        }

        if (!lightEnabled) {
            // Skip to the next light if this light is disabled.
            continue;
        }

        lightColor[0] *= lightIntensity;
        lightColor[1] *= lightIntensity;
        lightColor[2] *= lightIntensity;

        // Populate a GlfSimpleLight from the light information from Maya.
        GlfSimpleLight light;

        GfVec4f lightAmbient = blackColor;
        GfVec4f lightDiffuse = blackColor;
        GfVec4f lightSpecular = blackColor;

        // We receive the cone angle from Maya as a pair of floats which
        // includes the penumbra, but GlfSimpleLights don't currently support
        // that, so we only use the primary cone angle value.
        float lightCutoff = GfRadiansToDegrees(std::acos(lightCosineConeAngle[0]));
        float lightFalloff = lightDropoff;

        // decayRate is actually an enum in Maya that we receive as a float:
        // - 0.0 = no attenuation
        // - 1.0 = linear attenuation
        // - 2.0 = quadratic attenuation
        // - 3.0 = cubic attenuation (not supported by GlfSimpleLight)
        GfVec3f lightAttenuation(0.0f);
        if (lightDecayRate > 1.5) {
            // Quadratic attenuation.
            lightAttenuation[2] = 1.0f;
        } else if (lightDecayRate > 0.5f) {
            // Linear attenuation.
            lightAttenuation[1] = 1.0f;
        } else {
            // No/constant attenuation.
            lightAttenuation[0] = 1.0f;
        }

        if (lightHasDirection && !lightHasPosition) {
            // This is a directional light. Set the direction as its position.
            lightPosition[0] = -lightDirection[0];
            lightPosition[1] = -lightDirection[1];
            lightPosition[2] = -lightDirection[2];
            lightPosition[3] = 0.0f;

            // Revert direction to the default value.
            lightDirection = GfVec3f(0.0f, 0.0f, -1.0f);
            if (!viewDirectionAlongNegZ) {
                lightDirection[2] = 1.0f;
            }
        }

        if (!lightHasPosition && !lightHasDirection) {
            // This is an ambient light.
            lightAmbient = lightColor;
        } else {
            if (lightEmitsDiffuse) {
                lightDiffuse = lightColor;
            }
            if (lightEmitsSpecular) {
                // XXX: It seems that the specular color cannot be specified
                // separately from the diffuse color on Maya lights.
                lightSpecular = lightColor;
            }
        }

        light.SetAmbient(lightAmbient);
        light.SetDiffuse(lightDiffuse);
        light.SetSpecular(lightSpecular);
        light.SetPosition(lightPosition);
        light.SetSpotDirection(lightDirection);
        light.SetSpotCutoff(lightCutoff);
        light.SetSpotFalloff(lightFalloff);
        light.SetAttenuation(lightAttenuation);
        light.SetShadowBias(lightShadowBias);
        light.SetHasShadow(lightShadowOn && globalShadowOn);

        lights.push_back(light);
    }

    lightingContext->SetLights(lights);

    // XXX: These material settings match what we used to get when we read the
    // material from OpenGL. This should probably eventually be something more
    // sophisticated.
    GlfSimpleMaterial material;
    material.SetAmbient(whiteColor);
    material.SetDiffuse(whiteColor);
    material.SetSpecular(blackColor);
    material.SetEmission(blackColor);
    material.SetShininess(0.0001f);

    lightingContext->SetMaterial(material);

    lightingContext->SetSceneAmbient(blackColor);

    return lightingContext;
}
