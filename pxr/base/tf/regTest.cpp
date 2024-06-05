//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/instantiateSingleton.h"

#include <algorithm>
#include <iostream>
#include <signal.h>

using std::cerr;
using std::endl;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(TfRegTest);

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

int
TfRegTest::_Main(int argc, char *argv[])
{
    string progName(argv[0]);

    if (argc < 2) {
        _Usage(progName);
        _PrintTestNames();
        return 2;
    }

    std::string testName = argv[1];

    if (_functionTable.find(testName) != _functionTable.end()) {
        if (argc > 2) {
            cerr << progName << ": test function '" << testName
                 << "' takes no arguments." << endl;
            return 2;
        }
        TfErrorMark m;
        return _HandleErrors(m, (*_functionTable[testName])());
    }
    else if (_functionTableWithArgs.find(testName) !=
             _functionTableWithArgs.end()) {
        TfErrorMark m;
        return _HandleErrors(m,
                (*_functionTableWithArgs[testName])(argc-1, argv+1));
    }
    else {
        cerr << progName << ": unknown test function " << testName << ".\n";
        _PrintTestNames();
        return 3;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
