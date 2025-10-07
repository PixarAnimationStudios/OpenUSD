//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/arch/darwin.h"
#import <Foundation/Foundation.h>

PXR_NAMESPACE_OPEN_SCOPE


const char* Arch_DarwinGetTemporaryDirectory() {
    return [NSTemporaryDirectory() UTF8String];
}

PXR_NAMESPACE_CLOSE_SCOPE