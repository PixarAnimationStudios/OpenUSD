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
#include "pxr/base/tf/timeStamp.h"

#include <iostream>

using namespace std;

static bool
Test_TfTimeStamp()
{
    bool ok = true;

    // Test constructor with initial value
    TfTimeStamp tInitial(23840);
    if (tInitial.Get() != 23840) {
        cout << "expected " << 23840 << ", got " << tInitial.Get() << endl;
        ok = false;
    }

    // Test copy constructor.
    TfTimeStamp tCopy(tInitial);
    if (tCopy.Get() != tInitial.Get()) {
        cout << "expected tCopy (timestamp = " << tCopy.Get() 
             << ") to contain same (copy-constructed) value from tInitial "
             << "(timestamp = " << tInitial.Get() << ")" << endl;
        ok = false;
    }

    // Test operator equal
    if (!(tCopy == tInitial)) {
        cout << "tCopy (timestamp = " << tCopy.Get() << ") does not seem "
             << "to think it's equal to tInitial (timestamp = " << tInitial.Get()
             << ")" << endl;
        ok = false;
    }
    TfTimeStamp tDifferent(23480293);
    if (tInitial == tDifferent) {
        cout << "tInitial (timestamp = " << tDifferent.Get() 
             << ") incorrectly thinks it's the same as tDifferent "
             << "(timestamp = " << tDifferent.Get() << ")" << endl;
        ok = false;
    }

    // Test operator inequal
    if (tCopy != tInitial) {
        cout << "tCopy (timestamp = " << tCopy.Get() << ") does not seem "
             << "to think it's equal to tInitial (timestamp = " << tInitial.Get()
             << ")" << endl;
        ok = false;
    }
    if (!(tCopy != tDifferent)) {
        cout << "tCopy (timestamp = " << tCopy.Get() << ") does not seem "
             << "to think it's different than tDifferent (timestamp = " << tDifferent.Get()
             << ")" << endl;
        ok = false;
    }

    // Test operator less than
    TfTimeStamp tSmaller(tCopy);
    tSmaller.Decrement();
    if (!(tSmaller < tCopy)) {
        cout << "tSmaller (timestamp = " << tSmaller.Get() << ") does not seem "
             << "to think it's less than tCopy (timestamp = " << tCopy.Get()
             << ")" << endl;
        ok = false;
    }

    // Test operator greater than
    TfTimeStamp tGreater(tCopy);
    tGreater.Increment();
    if (!(tGreater > tCopy)) {
        cout << "tGreater (timestamp = " << tGreater.Get() << ") does not seem "
             << "to think it's greater than tCopy (timestamp = " << tCopy.Get()
             << ")" << endl;
        ok = false;
    }

    // Test operator less than or equal
    if (!(tSmaller <= tCopy)) {
        cout << "tSmaller (timestamp = " << tSmaller.Get() << ") does not seem "
             << "to think it's less than or equal to tCopy (timestamp = " << tCopy.Get()
             << ")" << endl;
        ok = false;
    }

    // Test operator greater than or equal
    if (!(tGreater >= tCopy)) {
        cout << "tGreater (timestamp = " << tGreater.Get() << ") does not seem "
             << "to think it's greater than or equal to tCopy (timestamp = " << tCopy.Get()
             << ")" << endl;
        ok = false;
    }

    // Test operator less than or equal for an equal timestamp.
    TfTimeStamp tSame(tCopy);
    if (!(tSame <= tCopy)) {
        cout << "tSame (timestamp = " << tSame.Get() << ") does not seem "
             << "to think it's less than or equal to tCopy (timestamp = " << tCopy.Get()
             << ")" << endl;
        ok = false;
    }

    // Test operator greater than or equal for an equal timestamp.
    if (!(tSame >= tCopy)) {
        cout << "tSame (timestamp = " << tSame.Get() << ") does not seem "
             << "to think it's greater than or equal to tCopy (timestamp = " << tCopy.Get()
             << ")" << endl;
        ok = false;
    }

    //! Test increment
    tInitial.Increment();
    if (tInitial != TfTimeStamp(23841)) {
        cout << "Increment failed. tInitial should be 23841, but it's "
             << tInitial.Get();
        ok = false;
    }

    //! Test decrement
    tInitial.Decrement();
    tInitial.Decrement();
    if (tInitial != TfTimeStamp(23839)) {
        cout << "Increment failed. tInitial should be 23839, but it's "
             << tInitial.Get();
        ok = false;
    }

    return ok;
}

TF_ADD_REGTEST(TfTimeStamp);
