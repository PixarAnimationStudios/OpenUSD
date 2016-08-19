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
#ifndef TF_STOPWATCH_H
#define TF_STOPWATCH_H

/// \file tf/stopwatch.h
/// \ingroup group_tf_Performance

#include "pxr/base/arch/timing.h"

#include <iosfwd>
#include <string>
#include <vector>

/// \class TfStopwatch
/// \ingroup group_tf_Performance
///
/// Low-cost, high-resolution timer datatype.
///
/// A \c TfStopwatch can be used to perform very precise timings at runtime,
/// even in very tight loops.  The cost of "starting" or "stopping" a \c
/// TfStopwatch is very small: approximately 40 nanoseconds on a 900 Mhz
/// Pentium III Linux box, 300 nanoseconds on a 400 Mhz Sun, and 200
/// nanoseconds on a 250 Mhz SGI.
///
/// Note that this class is not thread-safe: if you need to take timings in a
/// multi-threaded region of a process, let each thread have its own \c
/// TfStopwatch and then combine results using the \c AddFrom() member
/// function.
///
class TfStopwatch {
public:
    /// Constructor with optionally supplied name, which is used only by
    /// GetName().  If \c share is true, then this stopwatch is saved in an
    /// internal set and can be retrieved via \c GetNamedStopwatch.  No
    /// provision is made for multiple stopwatches with the same name.  So if
    /// you want to retrieve it, make sure you name it uniquely.
    TfStopwatch(const std::string& name = std::string(),
                bool share = false);

    /// Copy constructor.
    ///
    /// We have a copy constructor because copies are never shared.
    TfStopwatch(const TfStopwatch& other);

    /// Destroy a stopwatch.
    virtual ~TfStopwatch();

    /// Record the current time for use by the next \c Stop() call.
    ///
    /// The \c Start() function records the current time.  A subsequent call
    /// to \c Start() before a call to \c Stop() simply records a later
    /// current time, but does not change the accumulated time of the \c
    /// TfStopwatch.
    inline void Start() {
        _startTick = ArchGetTickTime();
    }

    /// Increases the accumulated time stored in the \c TfStopwatch.
    ///
    /// The \c Stop() function increases the accumulated time by the duration
    /// between the current time and the last time recorded by a \c Start()
    /// call.  A subsequent call to \c Stop() before another call to \c
    /// Start() will therefore double-count time and throw off the results.
    ///
    /// A \c TfStopwatch also counts the number of samples it has taken. The
    /// "sample count" is simply the number of times that \c Stop() has been
    /// called.
    inline void Stop() {
        _nTicks += ArchGetTickTime() - _startTick;
        _sampleCount++;
    }

    ///  Resets the accumulated time and the sample count to zero.
    void Reset() {
        _nTicks = 0;
        _sampleCount = 0;
    }

    /// Adds the accumulated time and sample count from \c t into the \c
    /// TfStopwatch.
    ///
    /// If you have several timers taking measurements, and you wish to
    /// combine them together, you can add one timer's results into another;
    /// for example, \c t2.AddFrom(t1) will add \c t1 's time and sample count
    /// into \c t2.
    void AddFrom(const TfStopwatch& t) {
        _nTicks += t._nTicks;
        _sampleCount += t._sampleCount;
    }
    
    /// Return the name of the \c TfStopwatch.
    const std::string& GetName() const {
        return _name;
    }

    /// Return a copy of a particular named stopwatch.
    ///
    /// \c GetNamedStopwatch returns an unshared copy of the named stopwatch.
    static TfStopwatch GetNamedStopwatch(const std::string& name);
    
    /// Return the accumulated time in nanoseconds.
    ///
    /// Note that this number can easily overflow a 32-bit counter, so take
    /// care to save the result in an \c int64_t, and not a regular \c int or
    /// \c long.
    int64_t GetNanoseconds() const {
        return ArchTicksToNanoseconds(_nTicks);
    }

    /// Return the accumulated time in microseconds
    ///
    /// Note that 45 minutes will overflow a 32-bit counter, so take care to
    /// save the result in an \c int64_t, and not a regular \c int or \c long.
    int64_t GetMicroseconds() const {
        return GetNanoseconds() / 1000;
    }

    /// Return the accumulated time in milliseconds.
    int64_t GetMilliseconds() const {
        return GetMicroseconds() / 1000;
    }

    /// Return the current sample count.
    ///
    /// The sample count, which is simply the number of calls to \c Stop()
    /// since creation or a call to \c Reset(), is useful for computing
    /// average running times of a repeated task.
    size_t GetSampleCount() const {
        return _sampleCount;
    }

    /// Return the accumulated time in seconds as a \c double.
    double GetSeconds() const {
        return ArchTicksToSeconds(_nTicks);
    }

    /// Return the names of the currently shared stopwatches.
    ///
    /// Return a vector of strings that are the names of the currently
    /// available shared stopwatches.  Note that in a multithreaded
    /// environment, the available stopwatches can change between the time you
    /// retrieve the list of their names and when you retrieve the stopwatch
    /// objects themselves.  Fortunately, TfStopwatch objects are typically
    /// static and even if they are not, it does no harm to request a named
    /// stopwatch that does not exist.
    static std::vector<std::string> GetStopwatchNames();

    /// Returns true if this stopwatch is shared.
    bool IsShared() const {
        return _shared;
    }

    /// Assignment operator
    ///
    /// We have a custom assignment operator because copies are never shared.
    TfStopwatch& operator=(const TfStopwatch& other);

private:
    uint64_t    _nTicks;
    uint64_t    _startTick;
    size_t      _sampleCount;
    std::string _name;
    bool        _shared;
};

/// Output a TfStopwatch, using the format seconds.
///
/// The elapsed time in the stopwatch is output in seconds.  Note that the
/// timer need not be stopped.
///
/// \ingroup group_tf_DebuggingOutput
std::ostream& operator<<(std::ostream& out, const TfStopwatch& s);

#endif
