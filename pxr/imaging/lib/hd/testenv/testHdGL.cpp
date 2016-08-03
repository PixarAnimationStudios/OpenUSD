#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/tf/errorMark.h"

static void TestEnableBit(GLenum enableBit, GLenum attribBit)
{
    glEnable(enableBit);
    {
        glPushAttrib(attribBit);
        glDisable(enableBit);
        TF_VERIFY(not glIsEnabled(enableBit));
        glPopAttrib();
    }
    TF_VERIFY(glIsEnabled(enableBit));

    glDisable(enableBit);
    {
        glPushAttrib(attribBit);
        glEnable(enableBit);
        TF_VERIFY(glIsEnabled(enableBit));
        glPopAttrib();
    }
    TF_VERIFY(not glIsEnabled(enableBit));
}

static void TestPolygonBit()
{
    GLfloat factor = 1.0;
    GLfloat units = 1.0;
    glPolygonOffset(factor, units);
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &factor);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &units);
    TF_VERIFY(factor == 1.0);
    TF_VERIFY(units == 1.0);
    {
        glPushAttrib(GL_POLYGON_BIT);
        glPolygonOffset(2.0, 3.0);
        glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &factor);
        glGetFloatv(GL_POLYGON_OFFSET_UNITS, &units);
        TF_VERIFY(factor == 2.0);
        TF_VERIFY(units == 3.0);
        glPopAttrib();
    }
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &factor);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &units);
    TF_VERIFY(factor == 1.0);
    TF_VERIFY(units == 1.0);
}

static void TestDepthBufferBit()
{
    GLint func = 0;

    glDepthFunc(GL_NEVER);
    glGetIntegerv(GL_DEPTH_FUNC, &func);
    TF_VERIFY(func == GL_NEVER);
    {
        glPushAttrib(GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_LEQUAL);
        glGetIntegerv(GL_DEPTH_FUNC, &func);
        TF_VERIFY(func == GL_LEQUAL);
        glPopAttrib();
    }
    glGetIntegerv(GL_DEPTH_FUNC, &func);
    TF_VERIFY(func == GL_NEVER);
}

static void TestAttribStack()
{
    TestEnableBit(GL_POLYGON_OFFSET_FILL,      GL_ENABLE_BIT);
    TestEnableBit(GL_POLYGON_OFFSET_FILL,      GL_POLYGON_BIT);
    TestEnableBit(GL_SAMPLE_ALPHA_TO_COVERAGE, GL_ENABLE_BIT);
    TestEnableBit(GL_SAMPLE_ALPHA_TO_COVERAGE, GL_MULTISAMPLE_BIT);
    TestEnableBit(GL_PROGRAM_POINT_SIZE,       GL_ENABLE_BIT);
    TestEnableBit(GL_CLIP_DISTANCE0,           GL_ENABLE_BIT);
    TestEnableBit(GL_CLIP_DISTANCE1,           GL_ENABLE_BIT);
    TestEnableBit(GL_CLIP_DISTANCE2,           GL_ENABLE_BIT);
    TestEnableBit(GL_CLIP_DISTANCE3,           GL_ENABLE_BIT);
    TestEnableBit(GL_DEPTH_TEST,               GL_DEPTH_BUFFER_BIT);

    TestPolygonBit();
    TestDepthBufferBit();
}

int main(int argc, char **argv)
{
    TfErrorMark mark;

    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfGlewInit();
    GlfSharedGLContextScopeHolder sharedContext;

    TestAttribStack();

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
