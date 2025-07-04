//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/arch/darwin.h"
#import <Foundation/Foundation.h>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


const char* Arch_DarwinGetTemporaryDirectory() {
    char* temporaryDirectory;

    const char *nsTmpDir = [NSTemporaryDirectory() UTF8String];
    temporaryDirectory = new char[strlen(nsTmpDir) + 1];
    strcpy(temporaryDirectory, nsTmpDir);

    return temporaryDirectory;
}

PXR_NAMESPACE_CLOSE_SCOPE