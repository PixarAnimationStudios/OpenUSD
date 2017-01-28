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
// info.cpp
//

#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/info.h"
#include "pxr/imaging/glf/glContext.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/stringUtils.h"

#include <cstdlib>
#include <set>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


using std::set;
using std::string;
using std::vector;

static set<string> *
Glf_BuildAvailableExtensions()
{
    GlfSharedGLContextScopeHolder sharedContextScopeHolder;

    static set<string> availableExtensions;

    // Get the available extensions from OpenGL if we haven't yet.
    const char *extensions = (const char*) glGetString(GL_EXTENSIONS);
    if ( extensions ) {
        vector<string> extensionsList = TfStringTokenize(extensions);
        TF_FOR_ALL(i, extensionsList) {
            availableExtensions.insert(*i);
        }
    }
    return &availableExtensions;
}

bool
GlfHasExtensions(string const & queryExtensions)
{
    static set<string> *availableExtensions = Glf_BuildAvailableExtensions();

    // Tokenize the queried extensions.
    vector<string> extensionsList = TfStringTokenize(queryExtensions);

    // Return false if any queried extension is not available.
    TF_FOR_ALL(i, extensionsList) {
        if (!availableExtensions->count(*i)) {
            return false;
        }
    }

    // All queried extensions were found.
    return true;
}


bool
GlfHasLegacyGraphics()
{
    GlfGlewInit();

    // if glew says we don't support OpenGL 2.0,
    // then we must have very limited graphics.  In
    // common usage, this should only be true for NX
    // clients.
    return !GLEW_VERSION_2_0;
}

PXR_NAMESPACE_CLOSE_SCOPE

