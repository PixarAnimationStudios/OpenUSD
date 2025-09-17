//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_DOT_GRAPHER_H
#define PXR_EXEC_VDF_DOT_GRAPHER_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/grapherOptions.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/hash.h"

#include "pxr/base/tf/hashset.h"
#include <iosfwd>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// \class Vdf_DotGrapher
///
/// This is a grapher class that produces .dot files.
///
class Vdf_DotGrapher 
{
public:

    // Constructs a Vdf_DotGrapher object.
    //
    // Output from this grapher will go to the given output stream \p os.
    //
    Vdf_DotGrapher(std::ostream &os, const VdfGrapherOptions &options);

    // Produces the graph for the given \p network.
    //
    void Graph(const VdfNetwork &network);


private:

    // Populates \p nodesToGraph with the nodes that can be reached from
    // \p node withing \p maxInDepth and \p maxOutDepth.
    void _GetLimitedNodes(const VdfNode &node, 
                           int maxInDepth, int maxOutDepth,
                           std::vector<const VdfNode *> *nodesToGraph);


    // Prints the header of the .dot file.
    void _PrintHeader() const;

    // Prints the footer of the .dot file.
    void _PrintFooter() const;

    // Prints a node.
    VdfGrapherOptions::DisplayStyle _PrintNode(const VdfNode &node);

    // Prints the node's dependencies.  Printing of the dependencies
    // must come at the end of printing the node's attributes.  This is a
    // limitation of dot.
    void _PrintInputDependencies(
        const VdfNode &node, VdfGrapherOptions::DisplayStyle style) const;

    // Produces a unique string for the give node.
    std::string _DotId(const VdfNode &node) const;

    // Helper for _PrintNode.
    template<typename PortType, typename ConnectionCollection>
    void _CollectConnections(
        const PortType &port,
        ConnectionCollection *connectionCollection,
        std::vector<const PortType *> *portCollection) const;

private:

    // Output streams to which any graphing operations go.
    std::ostream &_os;

    // Temporary structure to mark the visited nodes in a traversal.
    VdfNodePtrSet _visitedNodes;

    // The options for this graph.
    const VdfGrapherOptions &_options;
};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
