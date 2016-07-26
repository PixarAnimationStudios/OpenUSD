#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/semaphore.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/arch/threads.h"



#if defined(ARCH_SUPPORTS_PTHREAD_SEMAPHORE)

TfSemaphore::TfSemaphore(size_t count)
{
    if (sem_init(&_sem, 0, count) < 0) {
        TF_FATAL_ERROR("sem_init failed");
    }
}

TfSemaphore::~TfSemaphore()
{
    if (sem_destroy(&_sem) < 0) {
        if (TF_DEV_BUILD)
            TF_FATAL_ERROR("sem_destroy failed [possible waiting thread?]");
    }
}

#endif


