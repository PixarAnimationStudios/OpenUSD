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
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/testArchUtil.h"

#include <string>
#include <cstdlib>

int main(int argc, char** argv)
{
    ArchSetProgramNameForErrors( "testArch ArchError" );
    ArchTestCrashArgParse(argc, argv);

    std::string log = ArchMakeTmpFileName("statusLogTester");
    FILE *logFile;

    ARCH_AXIOM((logFile = ArchOpenFile(log.c_str(), "w")) != NULL);
    fputs("fake log\n", logFile);
    fputs("let's throw in a weird printf %1024$s specifier\n", logFile);
    fclose(logFile);
    

    ArchLogStackTrace("Crashing", true, log.c_str());
    ArchUnlinkFile(log.c_str());

    ArchLogPostMortem("Test Crashing");

    // test crashing with and without spawning
    ArchTestCrash(ArchTestCrashMode::CorruptMemory);
    ArchTestCrash(ArchTestCrashMode::CorruptMemoryWithThread);

    // test GetStackTrace
    std::vector<std::string> stackTrace = ArchGetStackTrace(20);
    bool found = false;
    for (unsigned int i = 0; i < stackTrace.size(); i++) {
        found |= (stackTrace[i].find("main", 0) != std::string::npos);
    }
    ARCH_AXIOM(found);

    return 0;
}

