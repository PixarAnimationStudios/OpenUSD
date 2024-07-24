//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/stringUtils.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE

static bool
IsClose(double a, double b, double epsilon=1e-3)
{
    auto diff = fabs(a-b);
    return diff <= epsilon * abs(a)
           && diff <= epsilon * abs(b);
}

static bool
IsCloselyBounded(double value, double lower, double upper, double epsilon=1e-3)
{
    return (1 - epsilon) * lower  <= value
           && value <= (1 + epsilon) * upper;
}

static bool
Test_TfStopwatch()
{
    bool ok = true;

    // Test constructor
    TfStopwatch watch1;

    // Test copy constructor.
    TfStopwatch watchCopy(watch1);
    if (watchCopy.GetSeconds() != watch1.GetSeconds()) {
        cout << "expected watchCopy to contain (0.0) but got (\""
             <<  watchCopy.GetSeconds()
             << ")" << endl;
        ok = false;
    }

    // Test the timer

    // Delay .5 seconds (500 million nanoseconds)
    //
    auto pre_start = std::chrono::steady_clock::now();
    watch1.Start();
    auto post_start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto pre_stop = std::chrono::steady_clock::now();
    watch1.Stop();
    auto post_stop = std::chrono::steady_clock::now();

    std::chrono::duration<double> minElapsed = pre_stop - post_start;
    std::chrono::duration<double> maxElapsed = post_stop - pre_start;

    // The value of watch1 should be bounded by minElapsed / maxElapsed:
    //     minElapsed < watch1 < maxElapsed
    // measured via std::chrono. This should be roughly .5 seconds,
    // but may be higher since sleep_for may overshoot the requested
    // time. We take the upper / lower bounds to account for possible thread
    // scheduling issues.
    if (!IsCloselyBounded(watch1.GetSeconds(), minElapsed.count(),
                          maxElapsed.count())) {
        cout << "Sleep for .5 seconds but got: " << endl
             << "TfStopwatch: " << watch1.GetSeconds() << endl
             << "std::chrono (lower bound): " << minElapsed.count() << endl
             << "std::chrono (upper bound): " << maxElapsed.count() << endl;
        ok = false;
    }

    // Delay another .5 seconds and see if watch is near 1
    //
    pre_start = std::chrono::steady_clock::now();
    watch1.Start();
    post_start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pre_stop = std::chrono::steady_clock::now();
    watch1.Stop();
    post_stop = std::chrono::steady_clock::now();


    minElapsed += pre_stop - post_start;
    maxElapsed += post_stop - pre_start;

    // The value of watch1 should match the updated elapsed duration.
    if (!IsCloselyBounded(watch1.GetSeconds(), minElapsed.count(),
                          maxElapsed.count())) {
        cout << "Sleep for 1.0 seconds but got: " << endl
             << "TfStopwatch: " << watch1.GetSeconds() << endl
             << "std::chrono (lower bound): " << minElapsed.count() << endl
             << "std::chrono (upper bound): " << maxElapsed.count() << endl;
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
    if (!IsClose(watchCopy.GetSeconds(), watch1.GetSeconds())) {
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
    if (!IsClose(watchCopy.GetSeconds()/watch1.GetSeconds(), 2.0)) {
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

    return ok;
}

TF_ADD_REGTEST(TfStopwatch);
