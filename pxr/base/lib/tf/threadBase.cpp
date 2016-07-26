#include "pxr/base/tf/threadBase.h"
#include "pxr/base/tf/threadInfo.h"
#include "pxr/base/tf/threadDispatcher.h"



TfThreadBase::TfThreadBase(TfThreadInfo* threadInfo)
    : _finished(false)
    , _threadInfo(threadInfo)
{
    if (!_threadInfo) {
        _launchedSingleThreaded = true;
        _threadInfo = TfThreadInfo::Find();
    }
    else
        _launchedSingleThreaded = false;
    
    _threadInfo->_thread = this;
    _canceled = false;
    _finishedFunc = false;
}

TfThreadBase::~TfThreadBase()
{
}

void
TfThreadBase::_PossiblyRunPendingThread()
{
    /*
     * When waiting on a thread from your own pool, you should start running some
     * other pending thread, to avoid deadlock.
     */
    if (TfThreadInfo* callerInfo = TfThreadInfo::Find()) {
        if (callerInfo->_thread &&
            callerInfo->_thread->_inDispatcherPool &&
            callerInfo->_thread->_dispatcher == _dispatcher) {
            bool finished;
            {
                TF_SCOPED_AUTO(_finishedMutex);
                finished = _finished;
            }
            while(!finished) {
                // OK: pick up one of the jobs from my own pool...
                _dispatcher->_RunThreadFromQueue(callerInfo->_longTermThreadDataTable);
                callerInfo->_Store();

                TF_SCOPED_AUTO(_finishedMutex);
                finished = _finished;
            }
        }
    }
}


