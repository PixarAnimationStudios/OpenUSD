//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/arch/error.h"

PXR_NAMESPACE_USING_DIRECTIVE

using std::string;

//most of these tests are just for code coverage
int main(int /*argc*/, char const** /*argv*/)
{
#ifdef ARCH_OS_WASM_VM
    // note: for wasm, we use a hardcoded patch since there is not an
    // inherent way to retrieve this path.
    ARCH_AXIOM(ArchGetExecutablePath().find("/wasm", 0) != string::npos);
#else
    ARCH_AXIOM(ArchGetExecutablePath().find("testArch", 0) != string::npos);
#endif
    return 0;
}

