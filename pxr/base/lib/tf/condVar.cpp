#include "pxr/base/tf/condVar.h"
#include    <sys/time.h>
#include    <stdio.h>
#include    <errno.h>



TfCondVar::TfCondVar()
{
    pthread_cond_init(&_cond, NULL);
    _timeLimitSet = false;
}

void
TfCondVar::SetTimeLimit(double duration)
{
    timeval cutoff;
    gettimeofday(&cutoff, NULL);            // now
    
    cutoff.tv_sec += size_t(duration);      // + duration
    cutoff.tv_usec += long(1e6 * (duration - (size_t)duration));

    if (cutoff.tv_usec > 1000000) {
        // CODE_COVERAGE_OFF_NO_REPORT - this is completely dependant on time
        cutoff.tv_usec -= 1000000;
        cutoff.tv_sec++;
        // CODE_COVERAGE_ON_NO_REPORT
    }

    _timeLimit.tv_sec = cutoff.tv_sec;
    _timeLimit.tv_nsec = 1000 * cutoff.tv_usec;
    _timeLimitSet = true;
}

bool
TfCondVar::TimedWait(TfMutex& mutex)
{
    if (!_timeLimitSet) {
        return false;
    }
    else {
        if (pthread_cond_timedwait(&_cond, &mutex._GetRawMutex(),
                                   &_timeLimit) == ETIMEDOUT) {
            _timeLimitSet = false;
            return false;
        }
    }

    return true;
}


