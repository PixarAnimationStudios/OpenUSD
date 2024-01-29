//
// Copyright 2023 Pixar
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

#include "pxr/imaging/hgi/unitTestHelper.h"

#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/envSetting.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
HgiBasicTest()
{
    HgiInitializationTestDriver driver;

    if (driver.GetHgi() == nullptr) {
        return false;
    }
    
    return true;
}

static bool
HgiPipelineCreateTest()
{
    HgiPipelineCreationTestDriver driver;
    
    if (!driver.CreateTestPipeline()) {
        return false;
    }
    
    return true;
}

static bool
HgiExecuteGfxCmdBfrTest(std::string fileName)
{
    HgiGfxCmdBfrExecutionTestDriver driver;

    if (!driver.CreateTestPipeline()) {
        return false;
    }

    if (!driver.ExecuteTestGfxCmdBfr()) {
        return false;
    }

    if (!fileName.empty()) {
        if (!driver.WriteToFile(fileName)) {
            return false;
        }
    }

    return true;
}

class HgiUnitTestWindow : public GarchGLDebugWindow {
public:
    HgiUnitTestWindow(const char *title, int width, int height)
    : GarchGLDebugWindow(title, width, height)
    {}

    ~HgiUnitTestWindow() {}

    void OnInitializeGL() override {
        GarchGLApiLoad();
    }
};

/// Entrypoint to this unit test 
/// Valid command line options for this unit test are : 
/// --write <filename> // writes render output to disk
int 
main(int argc, char **argv)
{
    std::string fileName;
    for (int i = 0; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--write") {
            if (i+1 < argc) { 
                fileName = std::string(argv[++i]);
            }            
            break;
        }
    }
    
    TfErrorMark mark;

    // Setup OpenGL context, needed for HgiGL version of test.
    HgiUnitTestWindow unitTestWindow("hgi", 256, 256);
    unitTestWindow.Init();

    bool success = HgiBasicTest();
    success = success && HgiPipelineCreateTest();
    success = success && HgiExecuteGfxCmdBfrTest(fileName);

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}