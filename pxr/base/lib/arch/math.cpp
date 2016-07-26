#include "pxr/base/arch/math.h"
#include "pxr/base/arch/defines.h"

#if defined(ARCH_OS_LINUX)

#include <fenv.h>

bool ArchTrapInvalidFPOperations(bool enable)
{
#ifdef _GNU_SOURCE
    if (enable)
        return feenableexcept(FE_INVALID) & FE_INVALID;
    else
        return fedisableexcept(FE_INVALID) & FE_INVALID;
#else
    #error _GNU_SOURCE not defined
#endif
}

#endif

