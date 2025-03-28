//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// info.cpp
//

#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/glf/info.h"
#include "pxr/imaging/glf/glContext.h"

#include "pxr/base/tf/stringUtils.h"

#include <set>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


using std::set;
using std::string;
using std::vector;

static set<string>
Glf_BuildAvailableExtensions()
{
    GlfSharedGLContextScopeHolder sharedContextScopeHolder;

    set<string> availableExtensions;

    // Get the available extensions from OpenGL if we haven't yet.
    if (const char *extensions = (const char*) glGetString(GL_EXTENSIONS)) {
        const vector<string> extensionsList = TfStringTokenize(extensions);
        for (std::string const& extension : extensionsList) {
            availableExtensions.insert(extension);
        }
    }
    return availableExtensions;
}

bool
GlfHasExtensions(string const & queryExtensions)
{
    static set<string> availableExtensions = Glf_BuildAvailableExtensions();

    // Tokenize the queried extensions.
    const vector<string> extensionsList = TfStringTokenize(queryExtensions);

    // Return false if any queried extension is not available.
    for (std::string const& extension : extensionsList) {
        if (!availableExtensions.count(extension)) {
            return false;
        }
    }

    // All queried extensions were found.
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE

