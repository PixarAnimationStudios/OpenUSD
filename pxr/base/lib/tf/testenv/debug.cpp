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
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/enum.h"
#include <iostream>
#include <map>
#include <boost/assign/list_of.hpp>

enum DebugOff { OFF1, OFF2 };

TF_DEBUG_RANGE(DebugOff, OFF1, OFF2, false);


enum DebugFamily { GRANDPA, AUNT, FATHER, DAUGHTER, SON };

TF_DEBUG_RANGE(DebugFamily, GRANDPA, SON, true);


static bool
TestOff()
{
    bool retVal = true;

    TfDebug::DisableAll<DebugOff>();
    retVal &= !TfDebug::IsEnabled(OFF1) && !TfDebug::IsEnabled(OFF2);  

    TF_DEBUG(OFF1).Msg("off1");
    TF_DEBUG(OFF2).Msg("off2");

    TfDebug::EnableAll<DebugOff>();
    retVal &= !TfDebug::IsEnabled(OFF1) && !TfDebug::IsEnabled(OFF2);  

    TF_DEBUG(OFF1).Msg("off1");
    TF_DEBUG(OFF2).Msg("off2");

    if(retVal) printf("ok\n\n");
    else printf("error\n\n");
    return retVal;
}

static void
DebugFamilyMsg()
{
    TF_DEBUG(GRANDPA).Msg("grandpa\n");
    TF_DEBUG(AUNT).Msg("aunt\n");
    TF_DEBUG(FATHER).Msg("father\n");
    TF_DEBUG(DAUGHTER).Msg("daughter\n");
    TF_DEBUG(SON).Msg("son\n");
    printf("-\n");
}

static bool
TestFamily()
{
    bool retVal = true;
    TfDebug::DefineParentChild<DebugFamily>(GRANDPA, AUNT);
    TfDebug::DefineParentChild<DebugFamily>(GRANDPA, FATHER);
    TfDebug::DefineParentChild<DebugFamily>(FATHER, DAUGHTER);
    TfDebug::DefineParentChild<DebugFamily>(FATHER, SON);

    TfDebug::EnableAll<DebugFamily>();
    retVal &= TfDebug::IsEnabled(GRANDPA) && TfDebug::IsEnabled(AUNT) &&
        TfDebug::IsEnabled(FATHER) && TfDebug::IsEnabled(DAUGHTER) &&
        TfDebug::IsEnabled(SON);
    DebugFamilyMsg();

    TfDebug::DisableAll<DebugFamily>();
    retVal &= !TfDebug::IsEnabled(GRANDPA) && !TfDebug::IsEnabled(AUNT) &&
        !TfDebug::IsEnabled(FATHER) && !TfDebug::IsEnabled(DAUGHTER) &&
        !TfDebug::IsEnabled(SON);
    DebugFamilyMsg();

    TfDebug::Enable(GRANDPA);
    retVal &= TfDebug::IsEnabled(GRANDPA) && TfDebug::IsEnabled(AUNT) &&
        TfDebug::IsEnabled(FATHER) && TfDebug::IsEnabled(DAUGHTER) &&
        TfDebug::IsEnabled(SON);
    DebugFamilyMsg();

    TfDebug::Disable(GRANDPA);
    retVal &= !TfDebug::IsEnabled(GRANDPA) && !TfDebug::IsEnabled(AUNT) &&
        !TfDebug::IsEnabled(FATHER) && !TfDebug::IsEnabled(DAUGHTER) &&
        !TfDebug::IsEnabled(SON);
    DebugFamilyMsg();

    TfDebug::Enable(FATHER);
    retVal &= !TfDebug::IsEnabled(GRANDPA) && !TfDebug::IsEnabled(AUNT) &&
        TfDebug::IsEnabled(FATHER) && TfDebug::IsEnabled(DAUGHTER) &&
        TfDebug::IsEnabled(SON);
    DebugFamilyMsg();

    TfDebug::Disable(SON);
    retVal &= !TfDebug::IsEnabled(GRANDPA) && !TfDebug::IsEnabled(AUNT) &&
        TfDebug::IsEnabled(FATHER) && TfDebug::IsEnabled(DAUGHTER) &&
        !TfDebug::IsEnabled(SON);
    DebugFamilyMsg();

    if(retVal) printf("ok\n\n");
    else printf("error\n\n");
    return retVal;
}

static bool
Test_TfDebug()
{
    bool retVal = true;

    TfRegistryManager::RunUnloadersAtExit();
    retVal &= TestOff();
    retVal &= TestFamily();

    return retVal;
}


enum DebugTestEnv { FOO, FOOFLAM, FOOFLIMFLAM, FLIMFLAM, FLIM, FLAM };

TF_DEBUG_RANGE(DebugTestEnv, FOO, FLAM, true);


static bool
Test_TfDebugTestEnv()
{
    bool retVal = true;

    TF_DEBUG_ENVIRONMENT_SYMBOL(FOO, "fake foo env var");
    TF_DEBUG_ENVIRONMENT_SYMBOL(FOOFLAM, "fake fooflam env var");
    TF_DEBUG_ENVIRONMENT_SYMBOL(FOOFLIMFLAM, "fake fooflimflam env var");
    TF_DEBUG_ENVIRONMENT_SYMBOL(FLIMFLAM, "fake flimflam env var");
    TF_DEBUG_ENVIRONMENT_SYMBOL(FLIM, "fake flim env var");
    TF_DEBUG_ENVIRONMENT_SYMBOL(FLAM, "fake flam env var");

    TfDebug::SetDebugSymbolsByName("FLIM", false);
    TfDebug::SetDebugSymbolsByName("FLAM*", false);

    std::vector<std::string> symNames = TfDebug::GetDebugSymbolNames();
    std::sort(symNames.begin(), symNames.end());
    std::vector<std::string> expSymNames =
        boost::assign::list_of
            ("FLAM")
            ("FLIM")
            ("FLIMFLAM")
            ("FOO")
            ("FOOFLAM")
            ("FOOFLIMFLAM");
    std::vector<std::string> result;
    std::set_intersection(
        symNames.begin(), symNames.end(),
        expSymNames.begin(), expSymNames.end(),
        std::back_inserter(result));
    if (expSymNames != result) {
        printf("Error: could not find all expected symbol names!\n");
        return false;
    }

    std::map<std::string, std::string> expDescriptions =
        boost::assign::map_list_of
            ("FOO",         "fake foo env var")
            ("FOOFLAM",     "fake fooflam env var")
            ("FOOFLIMFLAM", "fake fooflimflam env var")
            ("FLIMFLAM",    "fake flimflam env var")
            ("FLIM",        "fake flim env var")
            ("FLAM",        "fake flam env var");
    for (std::map<std::string, std::string>::const_iterator i =
         expDescriptions.begin(); i != expDescriptions.end(); ++i) {
        std::string description = TfDebug::GetDebugSymbolDescription(i->first);
        if (description != i->second) {
            printf("Error: unexpected description for symbol '%s'\n"
                   "  expected: '%s'\n"
                   "    actual: '%s'\n",
                   i->first.c_str(),
                   i->second.c_str(),
                   description.c_str());
            return false;
        }
    }

    // ok, so diff our results
    printf("%s\n", TfDebug::GetDebugSymbolDescriptions().c_str());

    return retVal;
}

static bool
Test_TfDebugTestEnvList()
{
    // diff our results
    // note -- this function isn't actually called because TF_DEBUG=list
    //         will cause an equivalent printf() in libtf and then exit.
    printf("%s\n", TfDebug::GetDebugSymbolDescriptions().c_str());
    return true;
}

static bool
Test_TfDebugTestEnvHelp()
{
    // this test is executed simply by including the header and
    // setting TF_DEBUG to "help" in the environment for this test
    // it will only fail if for some reason the debug registry init stuff
    // somehow runtime errors if it gets the special help var
    printf("should print help msg\b");
    return true;
}


static bool
Test_TfDebugFatal_1()
{
    bool retVal = false;

    TF_DEBUG_ENVIRONMENT_SYMBOL(GRANDPA, "loading of blah-blah files");
    TF_DEBUG_ENVIRONMENT_SYMBOL(FATHER, "parsing of foo-foo code");
    std::cerr << "Note: the following TfAbort is expected...\n";
    std::cerr << "------------------------------------------\n";

    TF_DEBUG_ENVIRONMENT_SYMBOL(FATHER, "some other thing that e1 does");

    return retVal;
}

static bool
Test_TfDebugFatal_2()
{
    bool retVal = false;

    std::cerr << "Note: the following TfAbort is expected...\n";
    std::cerr << "------------------------------------------\n";

    TF_DEBUG_ENVIRONMENT_SYMBOL(GRANDPA, NULL);

    return retVal;
}

static bool
Test_TfDebugFatal_3()
{
    bool retVal = false;

    std::cerr << "Note: the following TfAbort is expected...\n";
    std::cerr << "------------------------------------------\n";

    TF_DEBUG_ENVIRONMENT_SYMBOL(FATHER, "\0dasad");

    return retVal;
}

enum bogus { BOGUS1, BOGUS2 };

TF_DEBUG_RANGE(bogus, BOGUS1, BOGUS1, true);


static bool
Test_TfDebugFatal_4()
{
    bool retVal = false;

    std::cerr << "Note: the following TfAbort is expected...\n";
    std::cerr << "------------------------------------------\n";

    TF_DEBUG_ENVIRONMENT_SYMBOL(BOGUS2, "some other thing that e1 does");

    return retVal;
}

TF_ADD_REGTEST(TfDebug);
TF_ADD_REGTEST(TfDebugTestEnv);
TF_ADD_REGTEST(TfDebugTestEnvList);
TF_ADD_REGTEST(TfDebugTestEnvHelp);
TF_ADD_REGTEST(TfDebugFatal_1);
TF_ADD_REGTEST(TfDebugFatal_2);
TF_ADD_REGTEST(TfDebugFatal_3);
TF_ADD_REGTEST(TfDebugFatal_4);


