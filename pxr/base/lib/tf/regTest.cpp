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
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/instantiateSingleton.h"

#include <algorithm>
#include <iostream>
#include <signal.h>
#include <unistd.h>

TF_INSTANTIATE_SINGLETON(TfRegTest);

using std::cerr;
using std::endl;
using std::string;
using std::vector;

TfRegTest&
TfRegTest::GetInstance()
{
    return TfSingleton<TfRegTest>::GetInstance();
}

bool
TfRegTest::Register(const char* name, RegFunc func)
{
    _functionTable[name] = func;
    return true;
}

bool
TfRegTest::Register(const char* name, RegFuncWithArgs func)
{
    _functionTableWithArgs[name] = func;
    return true;
}

static int
_HandleErrors(const TfErrorMark &m, bool success)
{
    if (!success)
        return 1;

    if (m.IsClean())
        return 0;

    int rc(100);
    for (TfErrorMark::Iterator i = m.GetBegin(); i != m.GetEnd(); ++i)
    {
        ++rc;
        cerr << "*** Error in " << i->GetSourceFileName()
             << "@line " << i->GetSourceLineNumber()
             << "\n    " << i->GetCommentary() << "\n";
    }

    return(rc);
}

void
TfRegTest::_PrintTestNames()
{
    cerr << "Valid tests are:";

    vector<string> names;
    names.reserve(_functionTable.size() + _functionTableWithArgs.size());
    
    for (_Hash::const_iterator hi = _functionTable.begin();
                               hi != _functionTable.end(); ++hi)
        names.push_back(hi->first);

    for (_HashWithArgs::const_iterator hi = _functionTableWithArgs.begin();
                                       hi != _functionTableWithArgs.end();
                                       ++hi)
        names.push_back(hi->first);


    sort(names.begin(), names.end());
    for (const auto& name : names) {
        cerr << "\n    " << name;
    }

    cerr << endl;
}

static void
_Usage(const string &progName)
{
    cerr << "Usage: " << progName << " testName [args]\n";
}

static string _testName;

int
TfRegTest::_Main(int argc, char *argv[])
{
    string progName(argv[0]);

    if (argc < 2) {
        _Usage(progName);
        _PrintTestNames();
        return 2;
    }

    if (argc < 2) {
        _Usage(progName);
        _PrintTestNames();
        return 2;
    }

    _testName = argv[1];

    if (_functionTable.find(::_testName) != _functionTable.end()) {
        if (argc > 2) {
            cerr << progName << ": test function '" << _testName
                 << "' takes no arguments." << endl;
            return 2;
        }
        TfErrorMark m;
        return _HandleErrors(m, (*_functionTable[::_testName])());
    }
    else if (_functionTableWithArgs.find(::_testName) !=
             _functionTableWithArgs.end()) {
        TfErrorMark m;
        return _HandleErrors(m,
                (*_functionTableWithArgs[::_testName])(argc-1, argv+1));
    }
    else {
        cerr << progName << ": unknown test function " << _testName << ".\n";
        _PrintTestNames();
        return 3;
    }
}


