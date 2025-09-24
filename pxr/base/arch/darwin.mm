//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/arch/darwin.h"
#import <Foundation/Foundation.h>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


const char* Arch_DarwinGetTemporaryDirectory() {
    std::string tmpDir = [NSTemporaryDirectory() UTF8String];
    return tmpDir.c_str();
}

PXR_NAMESPACE_CLOSE_SCOPE