//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_STATISTICS_H
#define PXR_USD_PCP_STATISTICS_H

#include "pxr/pxr.h"

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

class PcpCache;
class PcpPrimIndex;

/// Accumulate and print statistics about the contents of \p cache to
/// \p out.
void
Pcp_PrintCacheStatistics(
    const PcpCache* cache, std::ostream& out);

/// Accumulate and print statistics about the contents of \p primIndex to
/// \p out.
void
Pcp_PrintPrimIndexStatistics(
    const PcpPrimIndex& primIndex, std::ostream& out);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_STATISTICS_H
