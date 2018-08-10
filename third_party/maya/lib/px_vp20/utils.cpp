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
// glew must be included before any other GL header.
#include "pxr/imaging/glf/glew.h"

#include "px_vp20/utils.h"

#include "px_vp20/glslProgram.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/glf/simpleMaterial.h"

#include <maya/MBoundingBox.h>
#include <maya/MColor.h>
#include <maya/MDrawContext.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVector.h>
#include <maya/MFrameContext.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MMatrix.h>
#include <maya/MSelectionContext.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MStatus.h>
#include <maya/MTransformationMatrix.h>

#include <cmath>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE


/* static */
bool
px_vp20Utils::setupLightingGL(const MHWRender::MDrawContext& context)
{
    MStatus status;

    // Take into account only the 8 lights supported by the basic
    // OpenGL profile.
    const unsigned int nbLights =
        std::min(context.numberOfActiveLights(&status), 8u);
    if (status != MStatus::kSuccess) {
        return false;
    }

    if (nbLights == 0u) {
        return true;
    }

    // Lights are specified in world space and needs to be
    // converted to view space.
    const MMatrix worldToView =
        context.getMatrix(MHWRender::MFrameContext::kViewMtx, &status);
    if (status != MStatus::kSuccess) {
        return false;
    }

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixd(worldToView.matrix[0]);

    glEnable(GL_LIGHTING);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);

    {
        const GLfloat ambient[4]  = { 0.0f, 0.0f, 0.0f, 1.0f };
        const GLfloat specular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);

        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
    }

    for (unsigned int i = 0u; i < nbLights; ++i) {
        MFloatVector direction;
        float intensity;
        MColor color;
        bool hasDirection;
        bool hasPosition;

        MFloatPointArray positions;
        status = context.getLightInformation(
            i,
            positions,
            direction,
            intensity,
            color,
            hasDirection,
            hasPosition);
        if (status != MStatus::kSuccess) {
            return false;
        }

        const MFloatPoint& position = positions[0];

        if (hasDirection) {
            if (hasPosition) {
                // Assumes a Maya Spot Light!
                const GLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                const GLfloat diffuse[4] = {
                    intensity * color[0],
                    intensity * color[1],
                    intensity * color[2],
                    1.0f };
                const GLfloat pos[4] = {
                    position[0],
                    position[1],
                    position[2],
                    1.0f };
                const GLfloat dir[3] = {
                    direction[0],
                    direction[1],
                    direction[2] };

                glLightfv(GL_LIGHT0 + i, GL_AMBIENT, ambient);
                glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, diffuse);
                glLightfv(GL_LIGHT0 + i, GL_POSITION, pos);
                glLightfv(GL_LIGHT0 + i, GL_SPOT_DIRECTION, dir);

                // Maya's default values for spot lights.
                glLightf(GL_LIGHT0 + i, GL_SPOT_EXPONENT, 0.0);
                glLightf(GL_LIGHT0 + i, GL_SPOT_CUTOFF, 20.0);
            }
            else {
                // Assumes a Maya Directional Light!
                const GLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                const GLfloat diffuse[4] = {
                    intensity * color[0],
                    intensity * color[1],
                    intensity * color[2],
                    1.0f };
                const GLfloat pos[4] = {
                    -direction[0],
                    -direction[1],
                    -direction[2],
                    0.0f };

                glLightfv(GL_LIGHT0 + i, GL_AMBIENT, ambient);
                glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, diffuse);
                glLightfv(GL_LIGHT0 + i, GL_POSITION, pos);
                glLightf(GL_LIGHT0 + i, GL_SPOT_CUTOFF, 180.0f);
            }
        }
        else if (hasPosition) {
            // Assumes a Maya Point Light!
            const GLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            const GLfloat diffuse[4] = {
                intensity * color[0],
                intensity * color[1],
                intensity * color[2],
                1.0f };
            const GLfloat pos[4] = {
                position[0],
                position[1],
                position[2],
                1.0f };

            glLightfv(GL_LIGHT0 + i, GL_AMBIENT, ambient);
            glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, diffuse);
            glLightfv(GL_LIGHT0 + i, GL_POSITION, pos);
            glLightf(GL_LIGHT0 + i, GL_SPOT_CUTOFF, 180.0f);
        }
        else {
            // Assumes a Maya Ambient Light!
            const GLfloat ambient[4] = {
                intensity * color[0],
                intensity * color[1],
                intensity * color[2],
                1.0f };
            const GLfloat diffuse[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            const GLfloat pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

            glLightfv(GL_LIGHT0 + i, GL_AMBIENT, ambient);
            glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, diffuse);
            glLightfv(GL_LIGHT0 + i, GL_POSITION, pos);
            glLightf(GL_LIGHT0 + i, GL_SPOT_CUTOFF, 180.0f);
        }

        glEnable(GL_LIGHT0 + i);
    }

    glDisable(GL_LIGHTING);

    glPopMatrix();

    return true;
}

/* static */
void
px_vp20Utils::unsetLightingGL(const MHWRender::MDrawContext& context)
{
    MStatus status;

    // Take into account only the 8 lights supported by the basic
    // OpenGL profile.
    const unsigned int nbLights =
        std::min(context.numberOfActiveLights(&status), 8u);
    if (status != MStatus::kSuccess || nbLights == 0u) {
        return;
    }

    // Restore OpenGL default values for anything that we have modified.

    for (unsigned int i = 0u; i < nbLights; ++i) {
        glDisable(GL_LIGHT0 + i);

        const GLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glLightfv(GL_LIGHT0 + i, GL_AMBIENT, ambient);

        if (i == 0u) {
            const GLfloat diffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, diffuse);

            const GLfloat spec[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glLightfv(GL_LIGHT0 + i, GL_SPECULAR, spec);
        }
        else {
            const GLfloat diffuse[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, diffuse);

            const GLfloat spec[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            glLightfv(GL_LIGHT0 + i, GL_SPECULAR, spec);
        }

        const GLfloat pos[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
        glLightfv(GL_LIGHT0 + i, GL_POSITION, pos);

        const GLfloat dir[3] = { 0.0f, 0.0f, -1.0f };
        glLightfv(GL_LIGHT0 + i, GL_SPOT_DIRECTION, dir);

        glLightf(GL_LIGHT0 + i, GL_SPOT_EXPONENT, 0.0);
        glLightf(GL_LIGHT0 + i, GL_SPOT_CUTOFF, 180.0);
    }

    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_NORMALIZE);

    const GLfloat ambient[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    const GLfloat specular[4] = { 0.8f, 0.8f, 0.8f, 1.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
}

static
bool
_GetLightingParam(
        const MIntArray& intValues,
        const MFloatArray& floatValues,
        bool& paramValue)
{
    bool gotParamValue = false;

    if (intValues.length() > 0u) {
        paramValue = (intValues[0u] == 1);
        gotParamValue = true;
    } else if (floatValues.length() > 0u) {
        paramValue = GfIsClose(floatValues[0u], 1.0f, 1e-5);
        gotParamValue = true;
    }

    return gotParamValue;
}

static
bool
_GetLightingParam(
        const MIntArray& intValues,
        const MFloatArray& floatValues,
        int& paramValue)
{
    bool gotParamValue = false;

    if (intValues.length() > 0u) {
        paramValue = intValues[0u];
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

    if (floatValues.length() > 0u) {
        paramValue = floatValues[0u];
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

    if (intValues.length() >= 2u) {
        paramValue[0u] = intValues[0u];
        paramValue[1u] = intValues[1u];
        gotParamValue = true;
    } else if (floatValues.length() >= 2u) {
        paramValue[0u] = floatValues[0u];
        paramValue[1u] = floatValues[1u];
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

    if (intValues.length() >= 3u) {
        paramValue[0u] = intValues[0u];
        paramValue[1u] = intValues[1u];
        paramValue[2u] = intValues[2u];
        gotParamValue = true;
    } else if (floatValues.length() >= 3u) {
        paramValue[0u] = floatValues[0u];
        paramValue[1u] = floatValues[1u];
        paramValue[2u] = floatValues[2u];
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

    if (intValues.length() >= 3u) {
        paramValue[0u] = intValues[0u];
        paramValue[1u] = intValues[1u];
        paramValue[2u] = intValues[2u];
        if (intValues.length() > 3u) {
            paramValue[3u] = intValues[3u];
        }
        gotParamValue = true;
    } else if (floatValues.length() >= 3u) {
        paramValue[0u] = floatValues[0u];
        paramValue[1u] = floatValues[1u];
        paramValue[2u] = floatValues[2u];
        if (floatValues.length() > 3u) {
            paramValue[3u] = floatValues[3u];
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

    const unsigned int numMayaLights =
        context.numberOfActiveLights(
            MHWRender::MDrawContext::kFilteredToLightLimit,
            &status);
    if (status != MS::kSuccess || numMayaLights < 1u) {
        return lightingContext;
    }

    bool viewDirectionAlongNegZ = context.viewDirectionAlongNegZ(&status);
    if (status != MS::kSuccess) {
        // If we fail to find out the view direction for some reason, assume
        // that it's along the negative Z axis (OpenGL).
        viewDirectionAlongNegZ = true;
    }

    const MMatrix worldToViewMat =
        context.getMatrix(MHWRender::MFrameContext::kViewMtx, &status);
    CHECK_MSTATUS_AND_RETURN(status, lightingContext);

    const MMatrix projectionMat =
        context.getMatrix(MHWRender::MFrameContext::kProjectionMtx, &status);
    CHECK_MSTATUS_AND_RETURN(status, lightingContext);

    lightingContext->SetCamera(GfMatrix4d(worldToViewMat.matrix),
                               GfMatrix4d(projectionMat.matrix));

    GlfSimpleLightVector lights;

    for (unsigned int i = 0u; i < numMayaLights; ++i) {
        MHWRender::MLightParameterInformation* mayaLightParamInfo =
            context.getLightParameterInformation(i);
        if (!mayaLightParamInfo) {
            continue;
        }

        // Setup some default values before we read the light parameters.
        bool lightEnabled = true;

        GfMatrix4d lightTransform(1.0);

        // Some Maya lights may have multiple positions (e.g. area lights).
        // We'll accumulate all the positions and use the average of them.
        size_t  lightNumPositions = 0u;
        GfVec4f lightPosition(0.0f);
        bool    lightHasDirection = false;
        GfVec3f lightDirection(0.0f, 0.0f, -1.0f);
        if (!viewDirectionAlongNegZ) {
            // The convention for DirectX is positive Z.
            lightDirection[2] = 1.0f;
        }

        float      lightIntensity = 1.0f;
        GfVec4f    lightColor = blackColor;
        bool       lightEmitsDiffuse = true;
        bool       lightEmitsSpecular = false;
        float      lightDecayRate = 0.0f;
        float      lightDropoff = 0.0f;
        // The cone angle is 180 degrees by default.
        GfVec2f    lightCosineConeAngle(-1.0f);
        GfMatrix4d lightShadowMatrix(1.0);
        int        lightShadowResolution = 512;
        float      lightShadowBias = 0.0f;
        bool       lightShadowOn = false;

        bool globalShadowOn = false;

        const MDagPath& mayaLightDagPath = mayaLightParamInfo->lightPath();
        if (mayaLightDagPath.isValid()) {
            const MMatrix mayaLightMatrix = mayaLightDagPath.inclusiveMatrix();
            lightTransform.Set(mayaLightMatrix.matrix);
        }

        MStringArray paramNames;
        mayaLightParamInfo->parameterList(paramNames);

        for (unsigned int paramIndex = 0u; paramIndex < paramNames.length(); ++paramIndex) {
            const MString paramName = paramNames[paramIndex];
            const MHWRender::MLightParameterInformation::ParameterType paramType =
                mayaLightParamInfo->parameterType(paramName);
            const MHWRender::MLightParameterInformation::StockParameterSemantic paramSemantic =
                mayaLightParamInfo->parameterSemantic(paramName);

            MIntArray intValues;
            MFloatArray floatValues;
            MMatrix matrixValue;

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
                case MHWRender::MLightParameterInformation::kFloat4x4Row:
                    mayaLightParamInfo->getParameter(paramName, matrixValue);
                    break;
                case MHWRender::MLightParameterInformation::kFloat4x4Col:
                    mayaLightParamInfo->getParameter(paramName, matrixValue);
                    // Gf matrices are row-major.
                    matrixValue = matrixValue.transpose();
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
                case MHWRender::MLightParameterInformation::kWorldPosition: {
                    GfVec4f tempPosition(0.0f, 0.0f, 0.0f, 1.0f);
                    if (_GetLightingParam(intValues, floatValues, tempPosition)) {
                        lightPosition += tempPosition;
                        ++lightNumPositions;
                    }
                    break;
                }
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
                    // XXX: Remap the kShadowBias value back into the light's
                    // bias attribute value so it can be used by Hydra.
                    // Maya's default value for the "Bias" attribute on lights
                    // is 0.001, but that value gets reported here as 0.0022.
                    // When the attribute is set to -1.0, 0.0, or 1.0, it is
                    // reported back to us by the MLightParameterInformation as
                    // -0.198, 0.002, and 0.202, respectively.
                    lightShadowBias = (lightShadowBias - 0.002f) / 0.2f;
                    break;
                case MHWRender::MLightParameterInformation::kShadowMapSize:
                    _GetLightingParam(intValues, floatValues, lightShadowResolution);
                    break;
                case MHWRender::MLightParameterInformation::kShadowViewProj:
                    lightShadowMatrix.Set(matrixValue.matrix);
                    break;
                case MHWRender::MLightParameterInformation::kGlobalShadowOn:
                    _GetLightingParam(intValues, floatValues, globalShadowOn);
                    break;
                case MHWRender::MLightParameterInformation::kShadowOn:
                    _GetLightingParam(intValues, floatValues, lightShadowOn);
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

        // Set position back to the origin if we didn't get one, or average the
        // positions if we got more than one.
        if (lightNumPositions == 0u) {
            lightPosition = GfVec4f(0.0f, 0.0f, 0.0f, 1.0f);
        } else if (lightNumPositions > 1u) {
            lightPosition /= lightNumPositions;
        }

        lightColor[0u] *= lightIntensity;
        lightColor[1u] *= lightIntensity;
        lightColor[2u] *= lightIntensity;

        // Populate a GlfSimpleLight from the light information from Maya.
        GlfSimpleLight light;

        GfVec4f lightAmbient = blackColor;
        GfVec4f lightDiffuse = blackColor;
        GfVec4f lightSpecular = blackColor;

        // We receive the cone angle from Maya as a pair of floats which
        // includes the penumbra, but GlfSimpleLights don't currently support
        // that, so we only use the primary cone angle value.
        float lightCutoff = GfRadiansToDegrees(std::acos(lightCosineConeAngle[0u]));
        float lightFalloff = lightDropoff;

        // Maya's decayRate is effectively the attenuation exponent, so we
        // convert that into the three floats the GlfSimpleLight uses:
        // - 0.0 = no attenuation
        // - 1.0 = linear attenuation
        // - 2.0 = quadratic attenuation
        // - 3.0 = cubic attenuation
        GfVec3f lightAttenuation(0.0f);
        if (lightDecayRate > 2.5f) {
            // Cubic attenuation.
            lightAttenuation[0u] = 1.0f;
            lightAttenuation[1u] = 1.0f;
            lightAttenuation[2u] = 1.0f;
        } else if (lightDecayRate > 1.5f) {
            // Quadratic attenuation.
            lightAttenuation[2u] = 1.0f;
        } else if (lightDecayRate > 0.5f) {
            // Linear attenuation.
            lightAttenuation[1u] = 1.0f;
        } else {
            // No/constant attenuation.
            lightAttenuation[0u] = 1.0f;
        }

        if (lightHasDirection && lightNumPositions == 0u) {
            // This is a directional light. Set the direction as its position.
            lightPosition[0u] = -lightDirection[0u];
            lightPosition[1u] = -lightDirection[1u];
            lightPosition[2u] = -lightDirection[2u];
            lightPosition[3u] = 0.0f;

            // Revert direction to the default value.
            lightDirection = GfVec3f(0.0f, 0.0f, -1.0f);
            if (!viewDirectionAlongNegZ) {
                lightDirection[2u] = 1.0f;
            }
        }

        if (lightNumPositions == 0u && !lightHasDirection) {
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

        light.SetTransform(lightTransform);
        light.SetAmbient(lightAmbient);
        light.SetDiffuse(lightDiffuse);
        light.SetSpecular(lightSpecular);
        light.SetPosition(lightPosition);
        light.SetSpotDirection(lightDirection);
        light.SetSpotCutoff(lightCutoff);
        light.SetSpotFalloff(lightFalloff);
        light.SetAttenuation(lightAttenuation);
        light.SetShadowMatrix(lightShadowMatrix);
        light.SetShadowResolution(lightShadowResolution);
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

/* static */
bool
px_vp20Utils::RenderBoundingBox(
        const MBoundingBox& bounds,
        const GfVec4f& color,
        const MMatrix& worldViewMat,
        const MMatrix& projectionMat)
{
    static const GfVec3f cubeLineVertices[24u] = {
        // Vertical edges
        GfVec3f(-0.5f, -0.5f, 0.5f),
        GfVec3f(-0.5f, 0.5f, 0.5f),

        GfVec3f(0.5f, -0.5f, 0.5f),
        GfVec3f(0.5f, 0.5f, 0.5f),

        GfVec3f(0.5f, -0.5f, -0.5f),
        GfVec3f(0.5f, 0.5f, -0.5f),

        GfVec3f(-0.5f, -0.5f, -0.5f),
        GfVec3f(-0.5f, 0.5f, -0.5f),

        // Top face edges
        GfVec3f(-0.5f, 0.5f, 0.5f),
        GfVec3f(0.5f, 0.5f, 0.5f),

        GfVec3f(0.5f, 0.5f, 0.5f),
        GfVec3f(0.5f, 0.5f, -0.5f),

        GfVec3f(0.5f, 0.5f, -0.5f),
        GfVec3f(-0.5f, 0.5f, -0.5f),

        GfVec3f(-0.5f, 0.5f, -0.5f),
        GfVec3f(-0.5f, 0.5f, 0.5f),

        // Bottom face edges
        GfVec3f(-0.5f, -0.5f, 0.5f),
        GfVec3f(0.5f, -0.5f, 0.5f),

        GfVec3f(0.5f, -0.5f, 0.5f),
        GfVec3f(0.5f, -0.5f, -0.5f),

        GfVec3f(0.5f, -0.5f, -0.5f),
        GfVec3f(-0.5f, -0.5f, -0.5f),

        GfVec3f(-0.5f, -0.5f, -0.5f),
        GfVec3f(-0.5f, -0.5f, 0.5f),
    };

    static const std::string vertexShaderSource(
        "#version 140\n"
        "\n"
        "in vec3 position;\n"
        "uniform mat4 mvpMatrix;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(position, 1.0) * mvpMatrix;\n"
        "}\n");

    static const std::string fragmentShaderSource(
        "#version 140\n"
        "\n"
        "uniform vec4 color;\n"
        "out vec4 outColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    outColor = color;\n"
        "}\n");

    PxrMayaGLSLProgram renderBoundsProgram;

    if (!renderBoundsProgram.CompileShader(GL_VERTEX_SHADER,
                                           vertexShaderSource)) {
        MGlobal::displayError("Failed to compile bounding box vertex shader");
        return false;
    }

    if (!renderBoundsProgram.CompileShader(GL_FRAGMENT_SHADER,
                                           fragmentShaderSource)) {
        MGlobal::displayError("Failed to compile bounding box fragment shader");
        return false;
    }

    if (!renderBoundsProgram.Link()) {
        MGlobal::displayError("Failed to link bounding box render program");
        return false;
    }

    if (!renderBoundsProgram.Validate()) {
        MGlobal::displayError("Failed to validate bounding box render program");
        return false;
    }

    GLuint renderBoundsProgramId = renderBoundsProgram.GetProgramId();

    glUseProgram(renderBoundsProgramId);

    // Populate an array buffer with the cube line vertices.
    GLuint cubeLinesVBO;
    glGenBuffers(1, &cubeLinesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeLinesVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(cubeLineVertices),
                 cubeLineVertices,
                 GL_STATIC_DRAW);

    // Create a transformation matrix from the bounding box's center and
    // dimensions.
    MTransformationMatrix bboxTransformMatrix = MTransformationMatrix::identity;
    bboxTransformMatrix.setTranslation(bounds.center(), MSpace::kTransform);
    const double scales[3] = { bounds.width(), bounds.height(), bounds.depth() };
    bboxTransformMatrix.setScale(scales, MSpace::kTransform);

    const MMatrix mvpMatrix =
        bboxTransformMatrix.asMatrix() * worldViewMat * projectionMat;

    GLfloat mvpMatrixArray[4][4];
    mvpMatrix.get(mvpMatrixArray);

    // Populate the shader variables.
    GLuint mvpMatrixLocation = glGetUniformLocation(renderBoundsProgramId, "mvpMatrix");
    glUniformMatrix4fv(mvpMatrixLocation, 1, GL_TRUE, &mvpMatrixArray[0][0]);

    GLuint colorLocation = glGetUniformLocation(renderBoundsProgramId, "color");
    glUniform4fv(colorLocation, 1, color.data());

    // Enable the position attribute and draw.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_LINES, 0, sizeof(cubeLineVertices));
    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &cubeLinesVBO);

    glUseProgram(0);

    return true;
}

/* static */
bool
px_vp20Utils::GetSelectionMatrices(
        const MHWRender::MSelectionInfo& selectionInfo,
        const MHWRender::MDrawContext& context,
        GfMatrix4d& viewMatrix,
        GfMatrix4d& projectionMatrix)
{
    MStatus status;

    const MMatrix viewMat =
        context.getMatrix(MHWRender::MFrameContext::kViewMtx, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MMatrix projectionMat =
        context.getMatrix(MHWRender::MFrameContext::kProjectionMtx, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    int viewportOriginX;
    int viewportOriginY;
    int viewportWidth;
    int viewportHeight;
    status = context.getViewportDimensions(viewportOriginX,
                                           viewportOriginY,
                                           viewportWidth,
                                           viewportHeight);
    CHECK_MSTATUS_AND_RETURN(status, false);

    unsigned int selectRectX;
    unsigned int selectRectY;
    unsigned int selectRectWidth;
    unsigned int selectRectHeight;
    status = selectionInfo.selectRect(selectRectX,
                                      selectRectY,
                                      selectRectWidth,
                                      selectRectHeight);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MMatrix selectionMatrix;
    selectionMatrix[0][0] = (double)viewportWidth / (double)selectRectWidth;
    selectionMatrix[1][1] = (double)viewportHeight / (double)selectRectHeight;
    selectionMatrix[3][0] =
        ((double)viewportWidth - (double)(selectRectX * 2 + selectRectWidth)) /
            (double)selectRectWidth;
    selectionMatrix[3][1] =
        ((double)viewportHeight - (double)(selectRectY * 2 + selectRectHeight)) /
            (double)selectRectHeight;

    projectionMat *= selectionMatrix;

    viewMatrix = GfMatrix4d(viewMat.matrix);
    projectionMatrix = GfMatrix4d(projectionMat.matrix);

    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
