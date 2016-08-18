#include "pxr/base/gf/vec3f.h"

#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/imaging/garch/gl.h"

#include <iostream>

static bool
TestGlfGLContext()
{
    // Grab the shared context and make some simple GL calls.
    GlfTestGLContextSharedPtr sharedContext = 
        boost::dynamic_pointer_cast<GlfTestGLContext>( GlfGLContext::GetSharedGLContext() );
    TF_AXIOM( sharedContext );
    GlfGLContext::MakeCurrent(sharedContext);

    std::cout << "vendor: " << glGetString(GL_VENDOR) << "\n";
    std::cout << "renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "version: " << glGetString(GL_VERSION) << "\n";

    //
    // Setup
    //
    GlfTestGLContextSharedPtr redContext = GlfTestGLContext::Create(sharedContext);
    GlfGLContext::MakeCurrent(redContext);
    glColor3f(1.0, 0.0, 0.0);

    GlfTestGLContextSharedPtr greenContext = GlfTestGLContext::Create(sharedContext);
    GlfGLContext::MakeCurrent(greenContext);
    glColor3f(0.0, 1.0, 0.0);

    GlfTestGLContextSharedPtr blueContext = GlfTestGLContext::Create(boost::shared_ptr<GlfTestGLContext>());
    GlfGLContext::MakeCurrent(blueContext);
    glColor3f(0.0, 0.0, 1.0);

    //
    // Test Sharing
    //
    std::cout << "Testing IsValid(), IsSharing(), and AreSharing()\n";
    if (not redContext or not redContext->IsValid() or not
        redContext->IsSharing(sharedContext)) {
        std::cout << "redContext should be valid, but isn't.\n";
        return false;
    }
    if (not greenContext or not greenContext->IsValid() or not
        greenContext->IsSharing(sharedContext)) {
        std::cout << "greenContext should be valid, but isn't.\n";
        return false;
    }
    if (not blueContext or not blueContext->IsValid() or not
        not blueContext->IsSharing(sharedContext)) {
        std::cout << "blueContext should be valid, but isn't.\n";
        return false;
    }
    if (not GlfGLContext::AreSharing(redContext, greenContext) or
        GlfGLContext::AreSharing(redContext, blueContext)) {
        std::cout << "contexts should be sharing, but aren't.\n";
        return false;
    }

    //
    // Test MakeCurrent
    //
    std::cout << "Testing MakeCurrent()\n";
    GfVec3f currentColor(0);

    glGetFloatv(GL_CURRENT_COLOR, &currentColor[0]);
    if (currentColor != GfVec3f(0, 0, 1.0)) {
        std::cout << "blue context should be blue, but isn't.\n";
        return false;
    }

    GlfGLContext::MakeCurrent(greenContext);
    glGetFloatv(GL_CURRENT_COLOR, &currentColor[0]);
    if (currentColor != GfVec3f(0, 1.0, 0)) {
        std::cout << "green context should be green, but isn't.\n";
        return false;
    }

    GlfGLContext::MakeCurrent(redContext);
    glGetFloatv(GL_CURRENT_COLOR, &currentColor[0]);
    if (currentColor != GfVec3f(1.0, 0, 0)) {
        std::cout << "red context should be red, but isn't.\n";
        return false;
    }

    GlfGLContext::MakeCurrent(sharedContext);
    glGetFloatv(GL_CURRENT_COLOR, &currentColor[0]);
    if (currentColor != GfVec3f(1.0, 1.0, 1.0)) {
        std::cout << "shared context should be white, but isn't.\n";
        return false;
    }

    //
    // Test DoneCurrent
    //
    std::cout << "Testing DoneCurrent()\n";
    sharedContext->DoneCurrent();
    GlfGLContextSharedPtr current = GlfGLContext::GetCurrentGLContext();
    if (current->IsValid()) {
        std::cout << "current context shouldn't be valid after DoneCurrent.\n";
        return false;
    }

    //
    // Test GlfSharedGLContextScopeHolder
    //
    std::cout << "Testing GlfSharedGLContextScopeHolder\n";

    // First make the red context current
    GlfGLContext::MakeCurrent(redContext);
    {
        GlfSharedGLContextScopeHolder sharedContextScopeHolder;

        glGetFloatv(GL_CURRENT_COLOR, &currentColor[0]);
        if (currentColor != GfVec3f(1.0, 1.0, 1.0)) {
            std::cout << "shared context should be current, but isn't (1).\n";
            return false;
        }
    }
    // Red context should still be current
    glGetFloatv(GL_CURRENT_COLOR, &currentColor[0]);
    if (currentColor != GfVec3f(1.0, 0, 0)) {
        std::cout << "red context should be red, but isn't.\n";
        return false;
    }

    // Next make no context current
    redContext->DoneCurrent();
    current = GlfGLContext::GetCurrentGLContext();
    if (current->IsValid()) {
        std::cout << "current context is valid after leaving shared scope.\n";
        return false;
    }
    {
        GlfSharedGLContextScopeHolder sharedContextScopeHolder;

        glGetFloatv(GL_CURRENT_COLOR, &currentColor[0]);
        if (currentColor != GfVec3f(1.0, 1.0, 1.0)) {
            std::cout << "shared context should be current, but isn't (2).\n";
            return false;
        }
    }
    // No context should still be current
    current = GlfGLContext::GetCurrentGLContext();
    if (current->IsValid()) {
        std::cout << "current context shouldn't be valid after leaving scope.\n";
        return false;
    }

    return true;
}

int
main(int argc, char **argv)
{

    // Initialize GlfGLContext with GlfTestGLWidget's shared context.
    GlfTestGLContext::RegisterGLContextCallbacks();

    bool passed = TestGlfGLContext();

    if (passed) {
        std::cout << "Passed\n";
        exit(0);
    } else {
        std::cout << "Failed\n";
        exit(1);
    }
}

