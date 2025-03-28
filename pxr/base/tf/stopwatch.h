//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_STOPWATCH_H
#define PXR_BASE_TF_STOPWATCH_H

/// \file tf/stopwatch.h
/// \ingroup group_tf_Performance

#include "pxr/pxr.h"

#include "pxr/base/arch/timing.h"
#include "pxr/base/tf/api.h"

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfStopwatch
/// \ingroup group_tf_Performance
///
/// Low-cost, high-resolution timer datatype.
///
/// A \c TfStopwatch can be used to perform very precise timings at runtime,
/// even in very tight loops.  The cost of "starting" or "stopping" a
/// \c TfStopwatch is very small: approximately 40 nanoseconds on a 900 Mhz
/// Pentium III Linux box, 300 nanoseconds on a 400 Mhz Sun, and 200
/// nanoseconds on a 250 Mhz SGI.
///
/// Note that this class is not thread-safe: if you need to take timings in a
/// multi-threaded region of a process, let each thread have its own
/// \c TfStopwatch and then combine results using the \c AddFrom() member
/// function.
///
class TfStopwatch
{
public:

    /// Record the current time for use by the next \c Stop() call.
    ///
    /// The \c Start() function records the current time.  A subsequent call
    /// to \c Start() before a call to \c Stop() simply records a later
    /// current time, but does not change the accumulated time of the \c
    /// TfStopwatch.
    inline void Start() {
        _startTick = ArchGetStartTickTime();
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
        _nTicks += ArchGetStopTickTime() - _startTick;
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

private:
    uint64_t    _nTicks = 0;
    uint64_t    _startTick = 0;
    size_t      _sampleCount = 0;
};

/// Output a TfStopwatch, using the format seconds.
///
/// The elapsed time in the stopwatch is output in seconds.  Note that the
/// timer need not be stopped.
///
/// \ingroup group_tf_DebuggingOutput
TF_API std::ostream& operator<<(std::ostream& out, const TfStopwatch& s);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_STOPWATCH_H
