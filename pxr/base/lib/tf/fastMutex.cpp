#include "pxr/base/tf/fastMutex.h"

#include "pxr/base/arch/nap.h"


void
TfFastMutex::_WaitForLock() {
    for (;;) {
        /*
         * Supposedly, bus-snooping hardware will tell us when
         * this has changed, without hosing other processors.
         * When it has changed, try to grab the lock.
         */
        
        while (_value)
            ArchThreadYield();

        if (ArchAtomicCompareAndSwap(&_value, 0, 1))
            return;     // got it!
    }
}


