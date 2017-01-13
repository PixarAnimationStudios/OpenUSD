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
#include "pxr/base/arch/nap.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/stringUtils.h"

#include <iostream>
#include <algorithm>

using namespace std;

static void
Pause(double seconds)
{
    // Create a shared stopwatch named "Pause" and then
    // sleep for some number of seconds accumulating the
    // time in "Pause"
    //
    static TfStopwatch pauseWatch("pwatch", true);

    pauseWatch.Start();
    ArchNap(static_cast<size_t>(seconds * 100.0));
    pauseWatch.Stop();
}

static bool
Test_TfStopwatch()
{
    bool ok = true;

    // Test constructor
    TfStopwatch watch1("watch1");
    if (watch1.GetName() != "watch1") {
        cout << "GetName: expected \"watch1\", got "
             << watch1.GetName()
             << endl;
        ok = false;
    }

    // Test copy constructor.
    TfStopwatch watchCopy(watch1);
    if (watchCopy.GetSeconds() != watch1.GetSeconds() ||
        watchCopy.GetName()    != watch1.GetName()) {
        cout << "expected watchCopy to contain (\"watch1\", 0.0) but got (\""
             << watchCopy.GetName()
             << "\", " << watchCopy.GetSeconds()
             << ")" << endl;
        ok = false;
    }

    // Test the timer

    // Delay .5 seconds (500 million nanoseconds)
    //
    watch1.Start();
    ArchNap(50);
    watch1.Stop();

    // The value of watch1 should be "near" 0.5 seconds
    if (fabs(watch1.GetSeconds() - 0.5) > 0.05) {
        cout << "Sleep for .5 seconds but measured time was "
             << watch1.GetSeconds()
             << " seconds."
             << endl;
        ok = false;
    }

    // Delay another .5 seconds and see if watch is near 1
    //
    watch1.Start();
    ArchNap(50);
    watch1.Stop();

    // The value of watch1 should be "near" 1.0 seconds
    if (fabs(watch1.GetSeconds() - 1.0) > 0.1) {
        cout << "Sleep for 1.0 seconds but measured time was "
             << watch1.GetSeconds()
             << " seconds."
             << endl;
        ok = false;
    }

    // The value of watchCopy should be zero
    //
    if (watchCopy.GetSeconds() != 0.0) {
        cout << "watchCopy has non-zero initial time of "
             << watchCopy.GetSeconds()
             << " seconds."
             << endl;
        ok = false;
    }

    // Test AddFrom
    //
    watchCopy.AddFrom(watch1);
    if (watchCopy.GetSeconds() != watch1.GetSeconds()) {
        cout << "AddFrom: watchCopy has time of "
             << watchCopy.GetSeconds()
             << " instead of "
             << watch1.GetSeconds()
             << " seconds."
             << endl;
        ok = false;
    }

    // Test AddFrom
    //
    watchCopy.AddFrom(watch1);
    if (fabs(watchCopy.GetSeconds()/watch1.GetSeconds() - 2.0) > 0.00001) {
        cout << "AddFrom: watchCopy has time of "
             << watchCopy.GetSeconds()
             << " instead of "
             << 2 * watch1.GetSeconds()
             << " seconds."
             << endl;
        ok = false;
    }

    // Test Reset
    watchCopy.Reset();
    if (watchCopy.GetSeconds() != 0.0) {
        cout << "Reset: watchCopy has time of "
             << watchCopy.GetSeconds()
             << " instead of "
             << 0.0
             << " seconds."
             << endl;
        ok = false;
    }


    //////////////// Shared Stopwatches ////////////////

    // Test constructor
    TfStopwatch swatch1("swatch1", true);
    if (swatch1.GetName()  != "swatch1" ||
        swatch1.IsShared() != true)
    {
        cout << "GetName: expected \"swatch1\", got "
             << swatch1.GetName()
             << endl;
        ok = false;
    }

    // Test copy constructor.
    TfStopwatch swatchCopy(swatch1);
    if (swatchCopy.GetSeconds() != swatch1.GetSeconds() ||
        swatchCopy.GetName()    != swatch1.GetName() ||
        swatchCopy.IsShared()   != false)
    {
        cout << "expected watchCopy to contain (\"swatch1\", 0.0, false) but got (\""
             << swatchCopy.GetName()
             << "\", "
             << swatchCopy.GetSeconds()
             << ", "
             << swatchCopy.IsShared()
             << ")" << endl;
        ok = false;
    }

    // Test the timer
    // Delay .5 seconds (500 million nanoseconds)
    //
    swatch1.Start();
    ArchNap(50);
    swatch1.Stop();

    // The value of swatch1 should be "near" 0.5 seconds
    if (fabs(swatch1.GetSeconds() - 0.5) > 0.05) {
        cout << "Sleep for .5 seconds but measured time was "
             << swatch1.GetSeconds()
             << " seconds."
             << endl;
        ok = false;
    }

    // Delay another .5 seconds and see if swatch is near 1
    //
    swatch1.Start();
    ArchNap(50);
    swatch1.Stop();

    // The value of swatch1 should be "near" 1.0 seconds
    if (fabs(swatch1.GetSeconds() - 1.0) > 0.1) {
        cout << "Sleep for 1.0 seconds but measured time was "
             << swatch1.GetSeconds()
             << " seconds."
             << endl;
        ok = false;
    }

    // Test the assignment operator
    //
    watch1 = swatch1;
    if (watch1.GetSeconds() != swatch1.GetSeconds() ||
        watch1.GetName()    != swatch1.GetName() ||
        watch1.IsShared()   != false)
    {
        cout << "expected watch1 to contain (\"swatch1\", 0.0, false) but got (\""
             << watch1.GetName()
             << "\", "
             << watch1.GetSeconds()
             << ", "
             << watch1.IsShared()
             << ")" << endl;
        ok = false;
    }

    TfStopwatch swatch2("swatch2", true);

    // The value of swatch2 should be zero
    //
    if (swatch2.GetSeconds() != 0.0) {
        cout << "swatch2 has non-zero initial time of "
             << swatch2.GetSeconds()
             << " seconds."
             << endl;
        ok = false;
    }

    // Test AddFrom
    //
    swatch2.AddFrom(swatch1);
    if (swatch2.GetSeconds() != swatch1.GetSeconds()) {
        cout << "AddFrom: swatch2 has time of "
             << swatch2.GetSeconds()
             << " instead of "
             << swatch1.GetSeconds()
             << " seconds."
             << endl;
        ok = false;
    }

    // Test AddFrom
    //
    swatch2.AddFrom(swatch1);
    if (fabs(swatch2.GetSeconds()/swatch1.GetSeconds() - 2.0) > 0.00001) {
        cout << "AddFrom: swatch2 has time of "
             << swatch2.GetSeconds()
             << " instead of "
             << 2 * swatch1.GetSeconds()
             << " seconds."
             << endl;
        ok = false;
    }

    // Test Reset
    swatch2.Reset();
    if (swatch2.GetSeconds() != 0.0) {
        cout << "Reset: swatch2 has time of "
             << swatch2.GetSeconds()
             << " instead of "
             << 0.0
             << " seconds."
             << endl;
        ok = false;
    }

    // Test GetStopwatchNames
    vector<string> names = TfStopwatch::GetStopwatchNames();
    sort(names.begin(), names.end());

    if (TfStringJoin(names) != "swatch1 swatch2") {
        cout << "GetStopwatchNames returned: ("
             << TfStringJoin(names, ", ")
             << ") instead of (swatch1, swatch2)."
             << endl;
        ok = false;
    }

    // Call Pause, this should create a 3rd name
    //
    Pause(0.5);

    names = TfStopwatch::GetStopwatchNames();
    sort(names.begin(), names.end());

    if (TfStringJoin(names) != "pwatch swatch1 swatch2") {
        cout << "GetStopwatchNames returned: ("
             << TfStringJoin(names, ", ")
             << ") instead of (pwatch, swatch1, swatch2)."
             << endl;
        ok = false;
    }

    
    TfStopwatch pauseWatch = TfStopwatch::GetNamedStopwatch("pwatch");

    if (fabs(pauseWatch.GetSeconds() - 0.5) > 0.05) {
        cout << "pause for .5 seconds but measured time was "
             << pauseWatch.GetSeconds()
             << " seconds."
             << endl;
        ok = false;
    }

    // Now pause for another half second and then get the result time.
    //
    Pause(0.5);

    pauseWatch = TfStopwatch::GetNamedStopwatch("pwatch");
    if (fabs(pauseWatch.GetSeconds() - 1.0) > 0.1) {
        cout << "pause for 1.0 seconds but measured time was "
             << pauseWatch.GetSeconds()
             << " seconds."
             << endl;
        ok = false;
    }

    // Test removing from the set of named watches.  Copying over a shared
    // stopwatch should make it unshared and remove it from the list.
    //
    swatch2 = swatch1;

    names = TfStopwatch::GetStopwatchNames();
    sort(names.begin(), names.end());

    if (TfStringJoin(names) != "pwatch swatch1" ||
        swatch2.GetSeconds() != swatch1.GetSeconds() ||
        swatch2.GetName()    != swatch1.GetName() ||
        swatch2.IsShared()   != false)
    {
        cout << "Assignment to remove a shared stopwatch failed.\n"
             << "  GetStopwatchNames returned: ("
             << TfStringJoin(names, ", ")
             << "), expected (pwatch, swatch1).\n"
             << "  GetSeconds returned: " << swatch2.GetSeconds()
             << ", expected " << swatch1.GetSeconds() << "\n"
             << "  GetName returned: " << swatch2.GetName()
             << ", expected " << swatch1.GetName() << "\n"
             << "  IsShared returned: " << swatch2.IsShared()
             << ", expected false"
             << endl;
        ok = false;
    }

    // Test removing names in the destructor
    //
    TfStopwatch* swatchTemp = new TfStopwatch("swatchTemp", true);

    string names1 = TfStringJoin(TfStopwatch::GetStopwatchNames());

    delete swatchTemp;

    string names2 = TfStringJoin(TfStopwatch::GetStopwatchNames());

    if (names1 != "pwatch swatch1 swatchTemp" ||
        names2 != "pwatch swatch1")
    {
        cout << "Allocating and deleting swatchTemp failed to add or remove it.\n"
             << "  After allocting, name list was (" << names1
             << "), expected (pwatch swatch1 swatchTemp)\n"
             << "  After deleting, name list was (" << names2
             << "), expected (pwatch swatch1)"
             << endl;
        ok = false;
    }

    return ok;
}

TF_ADD_REGTEST(TfStopwatch);
