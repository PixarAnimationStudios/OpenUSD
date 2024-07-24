//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/virtualMemory.h"
#include "pxr/usd/sdf/pool.h"

PXR_NAMESPACE_OPEN_SCOPE

char *
Sdf_PoolReserveRegion(size_t numBytes)
{
    return static_cast<char *>(ArchReserveVirtualMemory(numBytes));
}

bool
Sdf_PoolCommitRange(char *start, char *end)
{
    return ArchCommitVirtualMemoryRange(start, end-start);
}

PXR_NAMESPACE_CLOSE_SCOPE
