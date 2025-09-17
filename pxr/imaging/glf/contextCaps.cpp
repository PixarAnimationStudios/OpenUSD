//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/debugCodes.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/instantiateSingleton.h"

#include <iostream>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE


TF_INSTANTIATE_SINGLETON(GlfContextCaps);

// Set defaults based on GL spec minimums
static const int _DefaultMaxArrayTextureLayers        = 256;

// Initialize members to ensure a sane starting state.
GlfContextCaps::GlfContextCaps()
    : glVersion(0)
    , coreProfile(false)
    , maxArrayTextureLayers(_DefaultMaxArrayTextureLayers)
{
}

/*static*/
void
GlfContextCaps::InitInstance()
{
    // Initialize the render context caps.
    // This needs to be called on a thread that has the gl context
    // bound before we go wide on the cpus.

    // XXX: This should be called on
    // an render context change event api. (bug #124971)

    GlfContextCaps& caps = TfSingleton<GlfContextCaps>::GetInstance();

    GarchGLApiLoad();

    caps._LoadCaps();
}

/*static*/
const GlfContextCaps&
GlfContextCaps::GetInstance()
{
    GlfContextCaps& caps = TfSingleton<GlfContextCaps>::GetInstance();

    if (caps.glVersion == 0) {
        TF_CODING_ERROR("GlfContextCaps has not been initialized");
        // Return the default set
    }

    return caps;
}

void
GlfContextCaps::_LoadCaps()
{
    // Reset Values to reasonable defaults based of OpenGL minimums.
    // So that if we early out, systems can still depend on the
    // caps values being valid.
    //
    // _LoadCaps can also be called multiple times, so not want
    // to mix and match values in the event of an early out.
    glVersion                    = 0;
    coreProfile                  = false;
    maxArrayTextureLayers        = _DefaultMaxArrayTextureLayers;

    if (!TF_VERIFY(GlfGLContext::GetCurrentGLContext()->IsValid())) {
        return;
    }

    const char *glVendorStr = (const char*)glGetString(GL_VENDOR);
    const char *glRendererStr = (const char*)glGetString(GL_RENDERER);
    const char *glVersionStr = (const char*)glGetString(GL_VERSION);

    // GL hasn't been initialized yet.
    if (glVersionStr == NULL) return;

    const char *dot = strchr(glVersionStr, '.');
    if (TF_VERIFY((dot && dot != glVersionStr),
                  "Can't parse GL_VERSION %s", glVersionStr)) {
        // GL_VERSION = "4.5.0 <vendor> <version>"
        //              "4.1 <vendor-os-ver> <version>"
        //              "4.1 <vendor-os-ver>"
        int major = std::max(0, std::min(9, *(dot-1) - '0'));
        int minor = std::max(0, std::min(9, *(dot+1) - '0'));
        glVersion = major * 100 + minor * 10;
    }

    if (glVersion >= 320) {
        GLint profileMask = 0;
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profileMask);
        coreProfile = (profileMask & GL_CONTEXT_CORE_PROFILE_BIT);
    }
    
    if (glVersion >= 300) {
        glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayTextureLayers);
    }

    if (TfDebug::IsEnabled(GLF_DEBUG_CONTEXT_CAPS)) {
        std::cout
            << "GlfContextCaps: \n"
            << "  GL_VENDOR                          = "
            <<    glVendorStr << "\n"
            << "  GL_RENDERER                        = "
            <<    glRendererStr << "\n"
            << "  GL_VERSION                         = "
            <<    glVersionStr << "\n"
            << "  GL version                         = "
            <<    glVersion << "\n"
        ;
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

