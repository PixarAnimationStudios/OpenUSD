//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/arch/env.h"

PXR_NAMESPACE_USING_DIRECTIVE

static bool
Test_TfGetenv()
{
    bool status = true;
    std::string testVar = "GetEnvTestsuiteTestVar";
    ArchSetEnv(testVar.c_str(), "testing", true);
    status &= ( TfGetenv(testVar, "bogusValue") == "testing" );
    ArchRemoveEnv(testVar.c_str());
    status &= ( TfGetenv(testVar, "bogusValue") == "bogusValue" );

    ArchSetEnv(testVar.c_str(), "42", true);
    status &= ( TfGetenvInt(testVar, 99) == 42 );
    ArchRemoveEnv(testVar.c_str());
    status &= ( TfGetenvInt(testVar, 99) == 99 );

    ArchSetEnv(testVar.c_str(), "true", true);
    status &= ( TfGetenvBool(testVar, false) == true );
    ArchRemoveEnv(testVar.c_str());
    status &= ( TfGetenvBool(testVar, false) == false );

    ArchSetEnv(testVar.c_str(), "TRUE", true);
    status &= ( TfGetenvBool(testVar, false) == true );
    ArchRemoveEnv(testVar.c_str());
    status &= ( TfGetenvBool(testVar, false) == false );

    ArchSetEnv(testVar.c_str(), "yes", true);
    status &= ( TfGetenvBool(testVar, false) == true );

    ArchSetEnv(testVar.c_str(), "YES", true);
    status &= ( TfGetenvBool(testVar, false) == true );

    ArchSetEnv(testVar.c_str(), "1", true);
    status &= ( TfGetenvBool(testVar, false) == true );

    ArchSetEnv(testVar.c_str(), "ON", true);
    status &= ( TfGetenvBool(testVar, false) == true );

    ArchSetEnv(testVar.c_str(), "on", true);
    status &= ( TfGetenvBool(testVar, false) == true );

    ArchSetEnv(testVar.c_str(), "false", true);
    status &= ( TfGetenvBool(testVar, false) == false );

    ArchSetEnv(testVar.c_str(), "false", true);
    status &= ( TfGetenvBool(testVar, true) == false );

    ArchSetEnv(testVar.c_str(), "someothercrap", true);
    status &= ( TfGetenvBool(testVar, false) == false );

    ArchSetEnv(testVar.c_str(), "someothercrap", true);
    status &= ( TfGetenvBool(testVar, true) == false );

    return status;
}

TF_ADD_REGTEST(TfGetenv);
