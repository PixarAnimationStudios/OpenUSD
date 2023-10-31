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

#include "pxr/imaging/hdSt/hgiUnitTestHelper.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/envSetting.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
HdStBasicTest()
{
    HdSt_InitializationTestDriver driver;

    if (driver.GetHgi() == nullptr)
        return false;
    
    return true;
}

static bool
HdStPipelineCreateTest()
{
    HdSt_PipelineCreationTestDriver driver;
    
    if(!driver.CreateTestPipeline())
        return false;
    
    return true;
}

static bool
HdStExecuteGfxCmdBfrTest(bool writeToDisk, std::string fileName)
{
    HdSt_GfxCmdBfrExecutionTestDriver driver;

    if (!driver.CreateTestPipeline())
        return false;

    if (!driver.ExecuteTestGfxCmdBfr())
        return false;

    if (writeToDisk) {
        if (!driver.WriteToDisk(fileName))
            return false;
    }

    return true;
}

/// Entrypoint to this unit test 
/// Valid command line options for this unit test are : 
/// -write <fileaname> // writes render output to disk
int 
main(int argc, char **argv)
{
    std::string fileName = "";
    bool write = false;
    for (int i = 0; i < argc; ++i) {
        if (std::string(argv[i]) == "-write") {
            if(i+1 < argc){ 
                fileName = std::string(argv[i+1]);
                write = true;
            }            
            break;
        }
    }
    
    TfErrorMark mark;

    bool success = HdStBasicTest();
    success = success & HdStPipelineCreateTest();
    success = success & HdStExecuteGfxCmdBfrTest(write, fileName);

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
