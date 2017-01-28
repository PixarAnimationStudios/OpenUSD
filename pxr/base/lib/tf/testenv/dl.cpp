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

#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/dl.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/arch/symbols.h"

using std::string;
PXR_NAMESPACE_USING_DIRECTIVE

static bool
Test_TfDl()
{
    // We should not be in the process of opening/closing a DL right now
    TF_AXIOM(!Tf_DlOpenIsActive());
    TF_AXIOM(!Tf_DlCloseIsActive());

    // Turn on TfDlopen debugging so we get coverage on the debug output too
    TfDebug::Enable(TF_DLOPEN);
    TfDebug::Enable(TF_DLCLOSE);

    // Check that opening a non-existing shared library fails
    TF_AXIOM(!TfDlopen("nonexisting" ARCH_LIBRARY_SUFFIX, ARCH_LIBRARY_NOW));

    // Check that TfDlopen fills in our error string with something
    string dlerror;
    TfDlopen("nonexisting" ARCH_LIBRARY_SUFFIX, ARCH_LIBRARY_NOW, &dlerror);
    TF_AXIOM(!dlerror.empty());

    // Compute path to test library.
    string dlname;
    TF_AXIOM(ArchGetAddressInfo((void*)Test_TfDl, &dlname, NULL, NULL, NULL));
    dlname = TfGetPathName(dlname) +
        "lib" ARCH_PATH_SEP "libTestTfDl" ARCH_LIBRARY_SUFFIX;

    // Make sure that this .so does indeed exist first
    printf("Checking test shared lib: %s\n", dlname.c_str());
    TF_AXIOM(!ArchFileAccess(dlname.c_str(), R_OK));

    // Check that we can open the existing library.
    void *handle =
        TfDlopen(dlname, ARCH_LIBRARY_LAZY|ARCH_LIBRARY_LOCAL, &dlerror);
    TF_AXIOM(handle);
    TF_AXIOM(dlerror.empty());
    TF_AXIOM(!TfDlclose(handle));

    // we should not be in the process of opening/closing a DL now either
    TF_AXIOM(!Tf_DlOpenIsActive());
    TF_AXIOM(!Tf_DlCloseIsActive());

    return true;
}

TF_ADD_REGTEST(TfDl);
