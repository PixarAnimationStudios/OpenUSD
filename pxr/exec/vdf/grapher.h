//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_GRAPHER_H
#define PXR_EXEC_VDF_GRAPHER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/types.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfGrapherOptions;
class VdfNetwork;
class VdfNode;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfGrapher
///
/// This class is used to produce printable graphs of Vdf networks.
///
/// The simplest way to use this class is to use the convenient static methods:
///
/// \section cppcode_VdfGrapher C++ Code Sample
/// \code 
///       VdfGrapher::GraphToFile(network, "filename.dot");
/// \endcode
///
///
/// Note: The command to be used for generating graphs should be:
///
/// \code
/// dot -Gsize=80,80 -Gpage=95,95 -Tps <in.dot> | epstopdf --filter -o <out.pdf>
/// \endcode
///
/// For viewing use:
/// 
/// \code
/// acroread <out.pdf>
/// \endcode
///
class VdfGrapher 
{
public:

    /// \name Static API
    /// @{
    
    /// Produces a graph of the given \p network and writes it to \p filename.
    ///
    /// Uses the default grapher options.
    ///
    VDF_API
    static void GraphToFile(const VdfNetwork &network,
                            const std::string &filename);

    /// Produces a graph of the given \p network and writes it to \p filename.
    ///
    /// The given \p options are used to configure the output.
    ///
    VDF_API
    static void GraphToFile(const VdfNetwork &network,
                            const std::string &filename,
                            const VdfGrapherOptions &options);

    /// Produces a graph of the given \p network and writes it to a temporary
    /// file.
    ///
    /// The given \p options are used to configure the output.
    ///
    VDF_API
    static void GraphToTemporaryFile(const VdfNetwork &network,
                                     const VdfGrapherOptions &options);

    /// Produces a graph in the neighborhood of \p node.
    ///
    VDF_API
    static void GraphNodeNeighborhood(
        const VdfNode &node, 
        int maxInDepth, int maxOutDepth,
        const std::vector<std::string> &exclude = std::vector<std::string>());

    /// Returns the list of all registered nodes in the given \p network with
    /// the given \p name.
    ///
    VDF_API
    static std::vector<const VdfNode *> GetNodesNamed(
        const VdfNetwork &network, 
        const std::string &name);


    /// Returns a string that represents a shell command that will view
    /// the file \p dotFileName.
    ///
    VDF_API
    static std::string GetDotCommand(const std::string &dotFileName);


    /// @}

};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
