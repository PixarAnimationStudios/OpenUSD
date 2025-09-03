//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_NETWORK_STATS_H
#define PXR_EXEC_VDF_NETWORK_STATS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/base/tf/hash.h"

#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNetwork;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfNetworkStats
///
/// A VdfNetworkStats object represents some useful statistics about a network.
///
class VdfNetworkStats
{
public:

    // Struct used to keep track of statistics about a node type that we
    // care about.
    struct NodeTypeStats {
        int count;
        size_t memUsage;
    };

private:

    // Contains statistic information per node type.
    typedef std::map<std::string, NodeTypeStats> _TypeStatsMap;
    _TypeStatsMap _statsMap;

public:

    /// Builds the statistics structures from the given \p network.
    ///
    VDF_API
    VdfNetworkStats(const VdfNetwork &network);


    /// Returns the length of the longest type name encountered.
    ///
    size_t GetMaxTypeNameLength() const { return _maxTypeNameLength; }

    /// Returns the max fan in.
    ///
    size_t GetMaxFanIn() const { return _maxFanIn; }

    /// Returns the node name with the max fan in.
    ///
    const std::string &GetMaxFanInNodeName() const { 
        return _maxFanInNodeName; 
    }

    /// Returns the max fan out.
    ///
    size_t GetMaxFanOut() const { return _maxFanOut; }

    /// Returns the node name with the max fan out.
    ///
    const std::string &GetMaxFanOutNodeName() const { 
        return _maxFanOutNodeName; 
    }

    /// Returns the count map.
    ///
    const _TypeStatsMap &GetStatsMap() const { return _statsMap; }

private:


    // The length of the longest encountered type name.
    size_t _maxTypeNameLength;

    // The max number of inputs into a node and the node where it was 
    // encountered.
    size_t _maxFanIn;
    std::string _maxFanInNodeName;

    // The max number of outputs out of a ndoe and the node where it was
    // encountered.
    size_t _maxFanOut;
    std::string _maxFanOutNodeName;

};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
