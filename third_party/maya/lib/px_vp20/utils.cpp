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
#include "pxr/imaging/garch/gl.h"

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


