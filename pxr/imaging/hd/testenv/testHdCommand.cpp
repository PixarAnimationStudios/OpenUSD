//
// Copyright 2021 Pixar
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

#include "pxr/imaging/hd/unitTestHelper.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
HdCommandBasicTest()
{
    Hd_TestDriver driver;
    HdUnitTestDelegate &sceneDelegate = driver.GetDelegate();

    driver.Draw();

    HdRenderDelegate *renderDelegate = 
        sceneDelegate.GetRenderIndex().GetRenderDelegate();

    if (!renderDelegate) {
        std::cout << "Failed to get a render delegate" << std::endl;
        return false;
    }

    HdCommandDescriptors commands = renderDelegate->GetCommandDescriptors();

    if (commands.empty()) {
        std::cout << "Failed to get commands" << std::endl;
        return false;
    }

    if (commands.size() == 1) {
        std::cout << "Got the following command: " << std::endl;
        std::cout << "    " << commands.front().commandName << std::endl;
    } else {
        std::cout << "Got the following commands: " << std::endl;
        for (const HdCommandDescriptor &cmd : commands) {
            std::cout << "    " << cmd.commandName << std::endl;
        }
    }
    std::cout << std::endl;

    // Try to invoke the print command
    HdCommandArgs args;
    args[TfToken("message")] = "Hello from test.";
    if (!renderDelegate->InvokeCommand(TfToken("print"), args)) {
        return false;
    }

    return true;
}

int 
main(int argc, char **argv)
{
    TfErrorMark mark;

    bool success = HdCommandBasicTest();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

