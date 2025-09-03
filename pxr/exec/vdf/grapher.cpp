//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/grapher.h"

#include "pxr/exec/vdf/grapherOptions.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/schedule.h"

#include "pxr/exec/vdf/dotGrapher.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/stackTrace.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/tf/hashmap.h"

#include <functional>
#include <iostream>
#include <ostream>
#include <sstream>
#include <fstream>

PXR_NAMESPACE_OPEN_SCOPE

//////////////////////////////////////////////////////////////////////////////
//
// Static Methods
//

void 
VdfGrapher::GraphToFile(const VdfNetwork &network,
                        const std::string &filename,
                        const VdfGrapherOptions &options)
{
    std::ofstream of(filename.c_str());
    Vdf_DotGrapher grapher(of, options);
    grapher.Graph(network);
}

void 
VdfGrapher::GraphToFile(const VdfNetwork &network, const std::string &filename)
{
    // Graph with the default options.
    VdfGrapher::GraphToFile(network, filename, VdfGrapherOptions());
}

// CODE_COVERAGE_OFF_VDF_DIAGNOSTICS
void 
VdfGrapher::GraphToTemporaryFile(const VdfNetwork &network,
                                 const VdfGrapherOptions &options)
{
    // Generate a temporary file name
    std::string dotFile =
        ArchMakeTmpFileName( 
            TfStringPrintf("vdfgraph_%s", ArchGetProgramNameForErrors())) +
        ".dot";
    
    VdfGrapher::GraphToFile(network, dotFile, options);
    
    std::cerr 
        << "The graph can be found in " << dotFile
        << std::endl
        << "The graph can be viewed by running the following command: "
        << std::endl
        << VdfGrapher::GetDotCommand(dotFile)
        << std::endl;
}

void 
VdfGrapher::GraphNodeNeighborhood(
    const VdfNode &node, 
    int maxInDepth,
    int maxOutDepth,
    const std::vector<std::string> &exclude)
{
    // Graph
    VdfGrapherOptions opts;
    opts.SetDrawMasks(true);
    opts.SetPageSize(1000,1000);
    opts.SetDrawAffectsMasks(true);
    opts.AddNodeToGraph(node, maxInDepth, maxOutDepth);
    opts.SetNodeFilterCallback(
        std::bind(&VdfGrapherOptions::DebugNameFilter,
            exclude, false, std::placeholders::_1));

    VdfGrapher::GraphToTemporaryFile(node.GetNetwork(), opts);

    // Print out a message saying that the graph was generated.
    std::cerr 
        << "--------------------------------------------------------------"
        << std::endl
        << "A graph was generated for the neighborhood around node: "
        << std::endl
        << node.GetDebugName()
        << std::endl;
}
// CODE_COVERAGE_ON

std::vector<const VdfNode *> 
VdfGrapher::GetNodesNamed(const VdfNetwork &network, const std::string &name)
{
    std::vector<const VdfNode *> result;
    for (size_t i = 0; i < network.GetNodeCapacity(); ++i) {
        if (const VdfNode *node = network.GetNode(i)) {
            if (node->GetDebugName() == name) {
                result.push_back(node);
            }
        }
    }
    return result;
}

std::string 
VdfGrapher::GetDotCommand(const std::string &dotFileName)
{
    return "xdot " + dotFileName;
}

PXR_NAMESPACE_CLOSE_SCOPE
