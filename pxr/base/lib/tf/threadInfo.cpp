#include "pxr/base/tf/threadInfo.h"
#include "pxr/base/tf/threadBase.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/auto.h"
#include "pxr/base/tf/mutex.h"
#include "pxr/base/arch/atomicOperations.h"



pthread_key_t* TfThreadInfo::_tsdKey = NULL;
volatile bool TfThreadInfo::_initialized = false;
int TfThreadInfo::_threadDataKeyCount = 0;

/*
 * This function cleans up the TfThreadInfo associated with
 * a "free" thread (i.e. one not created by TfThreadDispatcher).
 */
void
TfThreadInfo::_AutoDtor(void* key)
{
    delete reinterpret_cast<TfThreadInfo*>(key);
}

void
TfThreadInfo::_InitializeTsdKey()
{
    TF_EXECUTE_ONCE({
        TfThreadInfo::_tsdKey = new pthread_key_t;
        pthread_key_create(TfThreadInfo::_tsdKey, TfThreadInfo::_AutoDtor);

        TfThreadInfo* mainThreadInfo = new TfThreadInfo(0, 1, NULL);
        mainThreadInfo->_SetSharedBarrier(TfThreadInfo::_SharedBarrier::New(1));
        mainThreadInfo->_Store();

        TfThreadInfo::_initialized = true;
    });
}

TfThreadInfo::TfThreadInfo(size_t index, size_t n, TfThreadInfo* parent)
    : _threadId(0), _threadIndex(index), _nThreads(n), _thread(0), _parent(parent)
{
    static int globalThreadCount = 0;
    _uniqueThreadId = ArchAtomicIntegerFetchAndAdd( &globalThreadCount, 1 );
    _shortTermThreadDataTable = new _ThreadDataTable;
    _longTermThreadDataTable = new _ThreadDataTable;

    if (parent) {
        // transfer state we need from the parent.
        // Note that we haven't started the child thread going yet.

// XXX        _mallocData = parent->_mallocData;
    }
}

TfThreadInfo::~TfThreadInfo()
{
    for (TfIterator<_ThreadDataTable> i = *_shortTermThreadDataTable; i; ++i)
        delete i->second;

    for (TfIterator<_ThreadDataTable> i = *_longTermThreadDataTable; i; ++i)
        delete i->second;

    delete _shortTermThreadDataTable;
    delete _longTermThreadDataTable;
}

TfThreadInfo*
TfThreadInfo::_CreateFree()
{
    TfThreadInfo* info = new TfThreadInfo(0, 1, NULL);
    info->_Store();

    return info;
}

TfThreadInfo::_UntypedThreadData::~_UntypedThreadData()
{
}


