#!/pxrpythonsubst
#
# Copyright 2011, Pixar Animation Studios. All Rights Reserved.
#

from Mentor.Runtime import Runner, Fixture, AssertEqual, AssertTrue, RequiredException, FindDataFile

import datetime, os

class TestGlfPythonBindings(Fixture):
    '''Check that we can access Glf via Python.
    '''

    def TestGlfPythonBindings(self):
        '''Check that we can access Glf via Python.
        '''

        from pxr import Glf
        
        AssertTrue(Glf)
        AssertTrue(Glf.Texture)
        AssertTrue(Glf.TextureRegistry)
        AssertTrue(Glf.UVTexture)
        AssertTrue(Glf.DrawTarget)
        
    def TestGlfTextures(self):
        from OpenGL.GLUT import glutInit, glutCreateWindow
        import sys
        glutInit(sys.argv)
        glutCreateWindow(sys.argv[0])
        from pxr import Glf
        path = FindDataFile("testGlfPythonBindings.testenv/grid_t.jpg")
        AssertTrue(os.path.exists(path), "Sample texture not found")
        
        image = Glf.UVTexture(path)
        AssertTrue(image != None)
        image.memoryRequested = 0
        AssertEqual(image.width, 256)
        AssertEqual(image.height, 256)
        AssertTrue(image.GlTextureName > 0)

        image = Glf.UVTexture(path, 4, 4, 8, 8)
        AssertTrue(image != None)
        image.memoryRequested = 0
        AssertEqual(image.width, 240)
        AssertEqual(image.height, 248)
        
    def TestGlfDrawTarget(self):
        '''
        Draw a teapot to a png file. This is taken directly from 
        testGlfDrawTarget.cpp.
        '''
        from OpenGL.GLUT import (glutInit, 
                                 glutCreateWindow, 
                                 glutSolidTeapot)
        from OpenGL.GLU import gluPerspective
        from OpenGL import GL
        import sys
        width = 256
        height = 256
        glutInit(sys.argv)
        glutCreateWindow(sys.argv[0])
        from pxr import Glf
        drawTarget = Glf.DrawTarget(width, height)
        drawTarget.Bind()
        drawTarget.AddAttachment("color", GL.GL_RGBA, GL.GL_FLOAT, 
                                  GL.GL_RGBA)
        drawTarget.AddAttachment("depth", GL.GL_DEPTH_COMPONENT, 
                                  GL.GL_FLOAT,
                                  GL.GL_DEPTH_COMPONENT)
        GL.glEnable(GL.GL_DEPTH_TEST)
	GL.glEnable(GL.GL_LIGHTING)
	GL.glEnable(GL.GL_CULL_FACE)
	GL.glEnable(GL.GL_COLOR_MATERIAL)
        GL.glClearColor(0.5,0.5,0.5,1)
        GL.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT)
        
        lightKa = [0.1, 0.1, 0.1, 1.0]  # ambient light
        lightKd = [0.9, 0.9, 0.9, 1.0]  # diffuse light
        lightKs = [1.0, 1.0, 1.0, 1.0] # specular light
        GL.glLightfv(GL.GL_LIGHT0, GL.GL_AMBIENT, lightKa)
        GL.glLightfv(GL.GL_LIGHT0, GL.GL_DIFFUSE, lightKd)
        GL.glLightfv(GL.GL_LIGHT0, GL.GL_SPECULAR, lightKs)
    
        # position the light
        lightPos = [-15, 15, 0, 1] # positional light
        GL.glLightfv(GL.GL_LIGHT0, GL.GL_POSITION, lightPos)
    
        GL.glEnable(GL.GL_LIGHT0) # MUST enable each light source after configuration

        GL.glViewport(0, 0, width, height)
        
        GL.glMatrixMode(GL.GL_PROJECTION)
	GL.glLoadIdentity()
	gluPerspective(60.0, 1.0, 1.0, 100.0)
	GL.glMatrixMode(GL.GL_MODELVIEW)
        
        GL.glTranslatef(0, 0.0, -10.0)
	GL.glRotatef( -45.0, 0, 1, 0)
	GL.glRotatef( -45.0, 0, 0, 1)
        
        shininess = 15.0
        specularColor = [1.00000, 0.980392, 0.549020, 1.0]
        diffuseColor = [1.0, 0.5, 0.5]
    
        GL.glMaterialf(GL.GL_FRONT_AND_BACK, GL.GL_SHININESS, shininess)
        GL.glMaterialfv(GL.GL_FRONT_AND_BACK, GL.GL_SPECULAR, specularColor)
    
        GL.glColorMaterial(GL.GL_FRONT_AND_BACK, GL.GL_AMBIENT_AND_DIFFUSE)
        GL.glColor3fv(diffuseColor)
    
        GL.glEnableClientState(GL.GL_NORMAL_ARRAY)
        GL.glEnableClientState(GL.GL_VERTEX_ARRAY)
        
        GL.glFrontFace(GL.GL_CW)
        glutSolidTeapot( 3.0 )
        GL.glFrontFace(GL.GL_CCW)
        
        drawTarget.WriteToFile('color', 'testGlfPythonBindings_TestDrawTarget.png')
        drawTarget.Unbind()

if __name__ == '__main__':
    Runner().Main()
