//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

