#include "pxr/base/tf/thread.h"



// Specialization for void return type

TfThread<void>::~TfThread()
{
}

void
TfThread<void>::_ExecuteFunc()
{
    _StoreThreadInfo();
    _func();
}


