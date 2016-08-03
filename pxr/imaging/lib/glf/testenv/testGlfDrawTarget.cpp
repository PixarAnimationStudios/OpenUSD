//
// Copyright 2008, Pixar Animation Studios.  All rights reserved.

#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/testGLContext.h"
#include "pxr/imaging/glf/drawTarget.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"

#include "pxr/imaging/garch/gl.h"
#include "pxr/base/tf/diagnostic.h"

#include <iostream>
#include <set>
#include <vector>

using std::set;
using std::vector;

int viewWidth = 512, viewHeight = 512;
float cameraAngleX=45;
float cameraAngleY=45;
float cameraDistance=1;

void
InitLights()
{
    GLfloat lightKa[] = {0.1f, 0.1f, 0.1f, 1.0f};  // ambient light
    GLfloat lightKd[] = {0.9f, 0.9f, 0.9f, 1.0f};  // diffuse light
    GLfloat lightKs[] = {1.0f, 1.0f, 1.0f, 1.0f};           // specular light
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);

    // position the light
    float lightPos[4] = {-15, 15, 0, 1}; // positional light
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glEnable(GL_LIGHT0);                        // MUST enable each light source after configuration
}

void
DrawSphere( GfVec3f diffuseColor )
{
    float shininess = 15.0f;
    float specularColor[4] = {1.00000f, 0.980392f, 0.549020f, 1.0f};

    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess); 
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularColor);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3fv( (GLfloat *)(&diffuseColor));

    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    
    glFrontFace(GL_CW);
    
    // Draw a sphere
    int Long = 48;
    int Lat = 64; 
    float radius = 2.0;
    double longStep = (M_PI/Long); 
    double latStep = (2.0 * M_PI / Lat); 

    for (int i = 0; i < Long; ++i) {
        double a = i * longStep;
        double b = a + longStep; 
        double r0 = radius * sin(a);
        double r1 = radius * sin(b); 
        GLfloat z0 = radius * cos(a); 
        GLfloat z1 = radius * cos(b); 

        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= Lat; ++j) {
            double c = j * latStep; 
            GLfloat x = cos(c); 
            GLfloat y = sin(c); 

            glNormal3f((x * r0) / radius, (y * r0) / radius, z0 / radius); 
            glTexCoord2f(j / (GLfloat) Lat, i / (GLfloat) Long);
            glVertex3f(x * r0, y * r0, z0); 

            glNormal3f((x * r1) / radius, (y * r1) / radius, z1 / radius); 
            glTexCoord2f(j / (GLfloat) Lat, (i + 1) / (GLfloat) Long); 
            glVertex3f(x * r1, y * r1, z1);  
        }
        glEnd(); 
    }

    glFrontFace(GL_CCW);
}

void
TestGlfDrawTarget()
{
    GlfGLContext::MakeCurrent(GlfGLContext::GetSharedGLContext());

    GlfDrawTargetRefPtr
        dt = GlfDrawTarget::New( GfVec2i( viewWidth, viewHeight ) );
    TF_AXIOM(dt->GetFramebufferId() != 0);

    {
        TF_AXIOM(dt->IsBound()==false);

        dt->Bind();
        TF_AXIOM(dt->IsBound());
        TF_AXIOM(not dt->IsValid());
        TF_AXIOM(dt->GetSize()==GfVec2i(viewWidth,viewHeight));

        dt->Unbind();
        TF_AXIOM(not dt->IsBound());
    }
    
    GlfDrawTarget::AttachmentsMap const & aovs = dt->GetAttachments();
    GlfDrawTarget::AttachmentsMap::const_iterator it;
    
    {
        dt->Bind();
        TF_AXIOM(dt->IsBound());

        dt->AddAttachment( "color", GL_RGBA, GL_BYTE, GL_RGBA );
        dt->AddAttachment( "depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32F );
        TF_AXIOM(dt->IsValid());
        TF_AXIOM(aovs.size()==2);

        it = aovs.find("color");
        TF_AXIOM(it!=aovs.end());
        TF_AXIOM(it->second->GetGlTextureName()!=0);
        TF_AXIOM(it->second->GetFormat()==GL_RGBA);
        TF_AXIOM(it->second->GetType()==GL_BYTE);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_CULL_FACE);
        glEnable(GL_COLOR_MATERIAL);

        glClearColor(0.5,0.5,0.5,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        InitLights();

        glViewport(0, 0, viewWidth, viewHeight);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        // Replacement for gluPerspective
        GLdouble fovY = 60.0f;
        GLdouble aspect = (float) viewWidth / (float) viewHeight; 
        GLdouble zNear = 1.0; 
        GLdouble zFar = 100.0; 

        GLdouble fH = tan( fovY / 360.0 * M_PI ) * zNear;
        GLdouble fW = fH * aspect;
        glFrustum( -fW, fW, -fH, fH, zNear, zFar );

        glMatrixMode(GL_MODELVIEW);

        glTranslatef(0, 0.0f, -10.0f);
        glRotatef( -45.0f, 0, 1, 0);
        glRotatef( -45.0f, 0, 0, 1);

        DrawSphere( GfVec3f(1.0, 0.5, 0.5) );

        dt->WriteToFile("color", "testGlfDrawTarget_colorAOV_512x512.png");

        dt->Unbind();
        TF_AXIOM(not dt->IsBound());
    }

    it = aovs.find("color");
    TF_AXIOM(it!=aovs.end());
    size_t initialContentsID = it->second->GetContentsID();
    TF_AXIOM(initialContentsID!=0);

    {
        dt->Bind();
        TF_AXIOM(dt->IsBound());

        dt->SetSize( GfVec2i(256,256) );
        TF_AXIOM(dt->IsValid());
        TF_AXIOM(dt->GetSize()==GfVec2i(256,256));

        glClearColor(0.5,0.5,0.5,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, 256, 256);

        DrawSphere( GfVec3f(0.5, 1.0, 0.5) );

        dt->WriteToFile("color", "testGlfDrawTarget_colorAOV_256x256.png");

        dt->Unbind();
        TF_AXIOM(not dt->IsBound());
    }

    size_t secondContentsID = it->second->GetContentsID();
    TF_AXIOM(secondContentsID!=0);
    TF_AXIOM(initialContentsID!=secondContentsID);

    {
        dt->Bind();
        TF_AXIOM(dt->IsBound());

        dt->ClearAttachments( );
        TF_AXIOM(aovs.size()==0);
        TF_AXIOM(not dt->IsValid());

        dt->AddAttachment( "float_color", GL_RGBA, GL_FLOAT, GL_RGBA32F );
        dt->AddAttachment( "depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32F );
        TF_AXIOM(dt->IsValid());
        TF_AXIOM(aovs.size()==2);

        it = aovs.find("float_color");
        TF_AXIOM(it!=aovs.end());
        TF_AXIOM(it->second->GetGlTextureName()!=0);
        TF_AXIOM(it->second->GetFormat()==GL_RGBA);
        TF_AXIOM(it->second->GetType()==GL_FLOAT);

        glClearColor(0.5,0.5,0.5,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, 256, 256);

        DrawSphere( GfVec3f(0.0, 0.5, 1.0) );

        dt->WriteToFile("float_color", "testGlfDrawTarget_floatColorAOV_256x256.png");
        dt->WriteToFile("depth", "testGlfDrawTarget_floatDepthAOV_256x256.zfile");

        dt->Unbind();
        TF_AXIOM(not dt->IsBound());
    }

    size_t thirdContentsID = it->second->GetContentsID();
    TF_AXIOM(thirdContentsID!=0);
    TF_AXIOM(secondContentsID!=thirdContentsID);
}

int
main(int argc, char **argv)
{
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfGlewInit();

    TestGlfDrawTarget();

    std::cout << "Test PASSED\n";
}
