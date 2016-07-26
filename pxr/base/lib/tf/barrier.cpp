#include "pxr/base/tf/barrier.h"
#include "pxr/base/tf/auto.h"

#include "pxr/base/arch/nap.h"


TfBarrier::~TfBarrier() {
    if (IsWaitActive())
        TF_FATAL_ERROR("cannot destroy barrier with active threads");
}

void
TfBarrier::SetSize(size_t nThreads)
{
    if (IsWaitActive())
        TF_FATAL_ERROR("cannot change barrier size while threads are active");

    _nThreads = nThreads;
    _ctr = _nThreads;
}

/*
 * Wait for all threads to reach this barrier;
 * Refer to Programming with Posix Threads for the logic of why
 * this works (Very quickly: you wait until cycle increments, if you're
 * not the last thread.  It is not safe to wait until _ctr bumps down
 * to zero).
 */
    
void
TfBarrier::Wait()
{
    if (_nThreads == 1)
        return;
    
    if (_canSpin) {
        _fastMutex.Lock();
        
        if (--_ctr == 0) {
            _ctr = _nThreads;
            _cycle++;

            _fastMutex.Unlock();
            return;
        }
        else {
            size_t saveCycle = _cycle;

            _fastMutex.Unlock();
            while (saveCycle == _cycle)
                ArchThreadYield();
            return;
        }
    }
    else {
        TF_SCOPED_AUTO(_mutex);
        
        if (--_ctr == 0) {
            _ctr = _nThreads;
            _cycle++;
            _cond.Broadcast();
        }
        else {
            size_t saveCycle = _cycle;

            while (saveCycle == _cycle)
                _cond.Wait(_mutex);
        }
    }
}



