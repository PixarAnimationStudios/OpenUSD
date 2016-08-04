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
#include "pxr/base/arch/env.h"
#include "pxr/base/arch/systemInfo.h"
#if defined(ARCH_OS_WINDOWS)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#include <csignal>
#endif
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <time.h>

using std::string;

//most of these tests are just for code coverage
int main(int argc, char const* argv[])
{
    assert(! ArchGetUserName().empty());
    assert(ArchGetHomeDirectory("~nosuchuser").empty());
    assert(ArchGetHomeDirectory().find(ArchGetUserName(), 0) != string::npos);
    assert(ArchGetHomeDirectory(
                ArchGetUserName()).find(ArchGetUserName(), 0) != string::npos);

    assert(ArchGetExecutablePath().find("testArch", 0) != string::npos);

    return 0;
}

