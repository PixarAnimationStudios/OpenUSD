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
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/regTest.h"

PXR_NAMESPACE_USING_DIRECTIVE

static bool
Test_TfGetenv()
{
    bool status = true;
    std::string testVar = "GetEnvTestsuiteTestVar";
    setenv(testVar.c_str(), "testing", 1);
    status &= ( TfGetenv(testVar, "bogusValue") == "testing" );
    unsetenv(testVar.c_str());
    status &= ( TfGetenv(testVar, "bogusValue") == "bogusValue" );

    setenv(testVar.c_str(), "42", 1);
    status &= ( TfGetenvInt(testVar, 99) == 42 );
    unsetenv(testVar.c_str());
    status &= ( TfGetenvInt(testVar, 99) == 99 );

    setenv(testVar.c_str(), "true", 1);
    status &= ( TfGetenvBool(testVar, false) == true );
    unsetenv(testVar.c_str());
    status &= ( TfGetenvBool(testVar, false) == false );

    setenv(testVar.c_str(), "TRUE", 1);
    status &= ( TfGetenvBool(testVar, false) == true );
    unsetenv(testVar.c_str());
    status &= ( TfGetenvBool(testVar, false) == false );

    setenv(testVar.c_str(), "yes", 1);
    status &= ( TfGetenvBool(testVar, false) == true );

    setenv(testVar.c_str(), "YES", 1);
    status &= ( TfGetenvBool(testVar, false) == true );

    setenv(testVar.c_str(), "1", 1);
    status &= ( TfGetenvBool(testVar, false) == true );

    setenv(testVar.c_str(), "ON", 1);
    status &= ( TfGetenvBool(testVar, false) == true );

    setenv(testVar.c_str(), "on", 1);
    status &= ( TfGetenvBool(testVar, false) == true );

    setenv(testVar.c_str(), "false", 1);
    status &= ( TfGetenvBool(testVar, false) == false );

    setenv(testVar.c_str(), "false", 1);
    status &= ( TfGetenvBool(testVar, true) == false );

    setenv(testVar.c_str(), "someothercrap", 1);
    status &= ( TfGetenvBool(testVar, false) == false );

    setenv(testVar.c_str(), "someothercrap", 1);
    status &= ( TfGetenvBool(testVar, true) == false );

    return status;
}

TF_ADD_REGTEST(TfGetenv);
