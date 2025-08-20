//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/dotGrapher.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/grapher.h"
#include "pxr/exec/vdf/grapherOptions.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/schedule.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"

#include <ios>
#include <ostream>
#include <sstream>
#include <fstream>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

//////////////////////////////////////////////////////////////////////////////
// Internal helper classes

namespace
{

// This is an abstract base class that has methods for printing a node.  It is
// derived from for various display styles.
class _NodePrinter {
public:

    _NodePrinter(std::ostream &os) : _os(os) {}

    virtual ~_NodePrinter() {}

    // Print the node header.
    virtual void PrintNodeHeader(const std::string &nodeid,
                                 const std::string &nodecolor) const = 0;

    // Print all the inputs  in \p inputSpecs.
    virtual void PrintInputs(
        const std::vector<const VdfInput *> &inputs,
        const VdfGrapherOptions &options,
        int colspan) const {}

    // Print the name of the node.  The argument \p colspan can be used
    // if needed to decide how wide the name column should be.
    virtual void PrintNodeName(const std::string &name, int colspan) const  {}

    // Print all the outputs in \p outSpecs.
    virtual void PrintOutputs(
        const std::vector<const VdfOutput *> &outputs,
        const std::string &nodeid,
        const VdfGrapherOptions &options,
        int colspan) const {}

    // Print the node footer to close off the node.
    virtual void PrintNodeFooter() const {}

protected:

    std::ostream &_os;
};


// Returns a string containing the address in hexadecimal with a leading
// 0x, all enclosed in double-quotes.
static std::string
_FormatAddress(const void *addr)
{
    std::ostringstream os;
    os << '"' << std::hex << std::showbase << uintptr_t(addr) << '"';
    return os.str();
}

// Returns the color string for a given input spec.
static std::string
_GetInputSpecColor(const VdfInputSpec &inSpec) 
{
    return (inSpec.GetAccess() == VdfInputSpec::READ) ? "#9999cc" : "#99cc99";
}

// Returns a unique string for a given \p output to be used as port.
static std::string
_GetPortId(const VdfOutput &output, bool unique) 
{
    if (unique) {
        return _FormatAddress(&output);
    }
    return TfStringPrintf("\"%s\"", output.GetName().GetText());
}

// Returns a unique string for a given \p input to be used as port.
static std::string
_GetPortId(const VdfInput &input, bool unique) 
{
    if (unique) {
        return _FormatAddress(&input);
    }
    return TfStringPrintf("\"%s\"", input.GetName().GetText());
}



// This is the node printer when the draw style is set to draw the full node.
class _NodePrinterFull : public _NodePrinter {
public:

    _NodePrinterFull(std::ostream &os) : _NodePrinter(os) {}

    virtual void PrintNodeHeader(const std::string &nodeid,
                                 const std::string &nodecolor) const {
        _os << "\tnode [shape=plaintext];\n";
        _os << "\t\t" << nodeid << " [label=<\n";
        _os << "\t\t<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\"";
        if (!nodecolor.empty()) {
            _os << " BGCOLOR=\"" << nodecolor << "\"";
        }
        _os << ">\n";
    }

    virtual void PrintInputs(
        const std::vector<const VdfInput *> &inputs, 
        const VdfGrapherOptions &options,
        int colspan) const {

        _os << "\t\t\t<TR>";

        for(size_t i=0; i<inputs.size(); i++) {

            const VdfInput *input = inputs[i];

            int col = (i == inputs.size() - 1) ? colspan : 1;
            colspan -= col;
            TF_AXIOM(col > 0);

            _os << "<TD PORT=" << _GetPortId(*input, options.GetUniqueIds()) << " " 
                << "COLSPAN=\"" << col << "\">"
                << "<FONT POINT-SIZE=\"10\" FACE=\"Arial\" COLOR=\""
                << _GetInputSpecColor(input->GetSpec()) << "\">"
                << TfGetXmlEscapedString(input->GetName()) << "</FONT></TD>";
        }

        _os << "</TR>\n";
    }

    virtual void PrintNodeName(const std::string &name, int colspan) const {
        _os << "\t\t\t<TR><TD ";
        if (colspan) {
            _os << "COLSPAN=\"" << colspan << "\"";
        }
        _os << "> " << TfGetXmlEscapedString(name) << " </TD></TR>\n";
    }

    virtual void PrintOutputs(
        const std::vector<const VdfOutput *> &outputs,
        const std::string &nodeid,
        const VdfGrapherOptions &options,
        int colspan) const {

        // If we only have one output, we're leaving it off the graph to reduce
        // clutter.

        bool drawOutputs =
            outputs.size() > 1 ||
            options.GetDrawAffectsMasks() ||
            options.GetPrintSingleOutputs();

        if (!drawOutputs)
            return;

        _os << "\t\t\t<TR>";

        for(size_t i=0; i<outputs.size(); i++) {

            const VdfOutput *output = outputs[i];

            int col = (i == outputs.size() - 1) ? colspan : 1;
            colspan -= col;
            TF_AXIOM(col > 0);

            _os << "<TD PORT=" << _GetPortId(*output, options.GetUniqueIds()) << " "
                << "COLSPAN=\"" << col << "\""
                << ">"
                << "<FONT POINT-SIZE=\"10\" FACE=\"Arial\" COLOR=\"#cc9999\">";

            // Actual port name.
            _os << TfGetXmlEscapedString(output->GetName());

            if (const VdfMask *mask = output->GetAffectsMask())
                _os << "   " << mask->GetRLEString();

            _os << "</FONT></TD>";
        }

        _os << "</TR>\n";
    }

    virtual void PrintNodeFooter() const {
        _os << "\t\t</TABLE>>];\n";
    }
};

// This is the node printer when the draw style is set to draw the node
// without input or output connectors.
class _NodePrinterNoLabels : public _NodePrinter {
public:

    _NodePrinterNoLabels(std::ostream &os) : _NodePrinter(os) {}

    // XXX:codeCleanup
    // This should not output an HTML table, it should just use a simple
    // shape=box.
    virtual void PrintNodeHeader(const std::string &nodeid,
                                 const std::string &nodecolor) const {
        _os << "\tnode [shape=plaintext];\n";
        _os << "\t\t" << nodeid << " [label=<\n";
        _os << "\t\t<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\"";
        if (!nodecolor.empty()) {
            _os << " BGCOLOR=\"" << nodecolor << "\"";
        }
        _os << ">\n";
    }

    virtual void PrintNodeName(const std::string &name, int colspan) const {
        _os << "\t\t\t<TR><TD> " << TfGetXmlEscapedString(name)
            << " </TD></TR>\n";
    }

    virtual void PrintNodeFooter() const {
        _os << "\t\t</TABLE>>];\n";
    }
};

// This is the node printer when the draw style is set to draw a summary
// node.
class _NodePrinterSummary : public _NodePrinter {
public:
    _NodePrinterSummary(std::ostream &os) : _NodePrinter(os) {}

    // The summary printer just draws a little circle for each node.
    virtual void PrintNodeHeader(const std::string &nodeid,
                                 const std::string &nodecolor) const {
        _os << "\tnode [shape=circle, style=filled, "
            <<  "label=\"\", width=0.2, height=0.2";
        if (!nodecolor.empty()) {
            _os << ",color=" << nodecolor;
        }
        _os << "]; " << nodeid << ";\n";
    }
};

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////
//
// class Vdf_DotGrapher
//
Vdf_DotGrapher::Vdf_DotGrapher(
    std::ostream &os, 
    const VdfGrapherOptions &options)
:   _os(os),
    _options(options)
{
}

void
Vdf_DotGrapher::Graph(const VdfNetwork &network)
{
    _PrintHeader();

    size_t numNodes = network.GetNodeCapacity();
    size_t i;

    // Clear the list of nodes we've visited.
    _visitedNodes.clear();

    // This is the vector of all nodes that we will graph.
    // Note that we use a vector instead of a set to keep track of the 
    // printed nodes so that insertion is stable for diffing in test runs.
    std::vector<const VdfNode *> nodesToGraph;

    // If the specific nodes to graph haven't been specified, graph everyone.
    if (_options.GetNodesToGraph().empty()) {

        const VdfGrapherOptions::NodeFilterCallback &filterCallback =
            _options.GetNodeFilterCallback();

        // Graph all nodes in the network.
        for (i = 0; i < numNodes; ++i) {
            const VdfNode *node = network.GetNode(i);

            if (!node)
                continue;

            // Filter out the nodes that need filtering
            if (filterCallback && (!filterCallback(*node))) {
                continue;
            }

            nodesToGraph.push_back(node);

            _visitedNodes.insert(node);
        }

    } else {

        // Otherwise we want to graph a subset of the network.
        TF_FOR_ALL(iter, _options.GetNodesToGraph()) {
            _GetLimitedNodes(*iter->node, iter->maxInDepth, iter->maxOutDepth,
                             &nodesToGraph);
        }
    }

    // Set of nodes that we should render using a summary style only.
    TfHashMap<const VdfNode *, VdfGrapherOptions::DisplayStyle, TfHash> usedStyle;

    // Now print all the nodes that we've determined should be printed.
    for(const VdfNode *n : nodesToGraph) 
        usedStyle[n] = _PrintNode(*n);

    // Print the dependencies last. This is so that all the nodes referenced
    // have their attributes specified before we reference them in the 
    // dependency chart.  This both makes dot faster and correct.
    for(const VdfNode *n : nodesToGraph) 
        _PrintInputDependencies(*n, usedStyle[n]);

    _PrintFooter();
}

void 
Vdf_DotGrapher::_PrintInputDependencies(
    const VdfNode &node, VdfGrapherOptions::DisplayStyle style) const
{
    // Determine whether or not we should link to ports or not.  Only the
    // Full display style uses ports.
    bool usePorts = style == VdfGrapherOptions::DisplayStyleFull;

    // Process input dependencies.
    TF_FOR_ALL(inputConnector, node.GetInputsIterator()) {

        TF_FOR_ALL(iter, inputConnector->second->GetConnections()) {

            const VdfConnection *connection = *iter;

            // If this node has never been visited, we shouldn't include it.
            if (!_visitedNodes.count(&connection->GetSourceNode())) {
                continue;
            }
            
            TfToken color = _options.GetColor(connection);

            if (_options.GetDrawColorizedConnectionsOnly() && color.IsEmpty())
                continue;

            _os << "\t\t"
                << _DotId(connection->GetSourceNode());

            size_t numOutputs =
                connection->GetSourceNode().GetOutputSpecs().GetSize();

            // If we have more than 1 output, draw links to them explicitly.
            if (usePorts && numOutputs > 1) {

                _os << ":" << _GetPortId(
                    connection->GetSourceOutput(), _options.GetUniqueIds());
            }

            const TfToken &inputName = inputConnector->first;

            _os << " -> " << _DotId(node);
            if (usePorts) {

                // Compass point is "n" for north.
                _os << ":" << _GetPortId(
                    connection->GetTargetInput(), _options.GetUniqueIds())
                    << ":n";
            }

            std::vector<std::string> attributes;

            // Adding a weight on links called "pool" so that they will
            // tend towards a straight line.  "pool" is chosen because the
            // execution system uses that label for the point pool input which
            // has special meaning.
            //
            // XXX:codeCleanup
            // It would be nice not to hard code the word ".pool" for our
            // current specific usage in the execution system.
            if (inputName == ".pool")
                attributes.push_back("weight = \"100\"");

            std::string label;

            if (_options.GetDrawMasks()) {
                const VdfMask &mask = connection->GetMask();

                // If the number of bits is greater than 10 (arbitrary)
                // we will draw out the full mask, otherwise we will
                // display it in a compressed format.

                if (mask.GetSize() == 0) 
                    label += "(empty)";
                else if (mask.GetSize() <= 10)
                    label += mask.GetBits().GetAsStringLeftToRight();
                else
                    label += mask.GetRLEString();
            }
            
            // Append any annotation?
            std::string annotation = _options.GetAnnotation(connection);
            if (!annotation.empty()) {
                if (!label.empty())
                    label += ' ';
                label += '[' + annotation + ']';
            }

            //XXX: The ' ' before label should be removed, but it requires 
            //     baseline upgrades.
            if (!label.empty())
                attributes.push_back(" label = \"" + label + '"');

            if (!color.IsEmpty())
                attributes.push_back("color = \"" + color.GetString() + '\"');

            if (!attributes.empty()) {
                _os << "[ ";

                for(size_t i=0; i<attributes.size(); i++) {
                    _os << attributes[i];
                    if (i+1 < attributes.size())
                        _os << ", ";
                }
                _os << "];";
            } else {
                _os << ';';
            }

            _os << '\n';
        }
    }
}

template<typename PortType, typename ConnectionCollection>
void
Vdf_DotGrapher::_CollectConnections(
    const PortType &port,
    ConnectionCollection *connectionCollection,
    std::vector<const PortType *> *portCollection) const
{
    bool include = !_options.GetOmitUnconnectedSpecs();
    if (!include) {
        for(const auto &c : port.GetConnections()) {
    
            if (_options.GetDrawColorizedConnectionsOnly() &&
                _options.GetColor(c).IsEmpty()) {
                continue;
            }
                
            // Note that we want to populate result, so we can't break.
            if (std::is_same<PortType, VdfInput>::value) {

                if (_visitedNodes.count(&c->GetSourceNode())) {
                    connectionCollection->push_back(c);
                    include = true;
                }

            } else if (std::is_same<PortType, VdfOutput>::value) {

                if (_visitedNodes.count(&c->GetTargetNode())) {
                    connectionCollection->push_back(c);
                    include = true;
                }
            }
        }
    }
    
    if (include)
        portCollection->push_back(&port);
}

VdfGrapherOptions::DisplayStyle
Vdf_DotGrapher::_PrintNode(const VdfNode &node)
{
    VdfConnectionVector drawnIn, drawnOut;

    // Filter inputs and outputs as needed.
    std::vector<const VdfInput *> inputs;
    for(const auto &i : node.GetInputsIterator()) 
        _CollectConnections(*i.second, &drawnIn, &inputs);

    std::vector<const VdfOutput *> outputs;
    for(const auto &i : node.GetOutputsIterator()) 
        _CollectConnections(*i.second, &drawnOut, &outputs);

    // Select the node printer to use based on the draw style in our options.
    _NodePrinter *printer = NULL;

    VdfGrapherOptions::DisplayStyle style = _options.GetDisplayStyle();

    // Switch to summary style if needed.
    if (const VdfGrapherOptions::NodeStyleCallback &nodeStyleCallback =
        _options.GetNodeStyleCallback()) {

        style = nodeStyleCallback(node, drawnIn, drawnOut);
    }

    if (style == VdfGrapherOptions::DisplayStyleSummary)
        printer = new _NodePrinterSummary(_os);
    else if (style == VdfGrapherOptions::DisplayStyleFull)
        printer = new _NodePrinterFull(_os);
    else if (style == VdfGrapherOptions::DisplayStyleNoLabels)
        printer = new _NodePrinterNoLabels(_os);

    // Print the node header.
    printer->PrintNodeHeader(_DotId(node), _options.GetColor(&node));

    // Compute max. # col.
    const size_t numCol = TfMax(inputs.size(), outputs.size());

    // Print input connectors
    if (inputs.size())
        printer->PrintInputs(inputs, _options, numCol);

    // Print node name, values, etc...
    printer->PrintNodeName(node.GetDebugName(), numCol);

    // Print output connectors.
    if (outputs.size())
        printer->PrintOutputs(outputs, _DotId(node), _options, numCol);

    // Close off the table
    printer->PrintNodeFooter();

    delete printer;
    
    return style;
}

void 
Vdf_DotGrapher::_GetLimitedNodes(const VdfNode &node, 
                                 int maxInDepth, int maxOutDepth,
                                 std::vector<const VdfNode *> *nodesToGraph)
{
    TF_AXIOM(maxInDepth >= 0 && maxOutDepth >= 0);

    const VdfGrapherOptions::NodeFilterCallback &filterCallback =
        _options.GetNodeFilterCallback(); 

    // Filter out the nodes that need filtering
    if (filterCallback && (!filterCallback(node))) {
        return;
    }

    // Only add visited nodes to the list once but we need to traverse them
    // multiple times.  If this node was visited by the traversal of previous
    // node's neighborhood, bailing out early could cause us to fail to fully
    // expand the desired neighborhood around this node.
    if (_visitedNodes.insert(&node).second) {
        nodesToGraph->push_back(&node);
    }

    // If we haven't exhausted our input limit, traverse our inputs.
    if (maxInDepth) {

        TF_FOR_ALL(inputConnector, node.GetInputsIterator()) {
            TF_FOR_ALL(iter, inputConnector->second->GetConnections()) {

                // Print our inputs with one less depth level on its inputs
                // and none of its outputs.  Another possibly good choice
                // for outputDepth here is 1.
                _GetLimitedNodes((*iter)->GetSourceNode(), maxInDepth - 1, 0,
                                  nodesToGraph);
            }
        }

    }

    // If we haven't exhausted our output limit, traverse our outputs.
    if (maxOutDepth) {

        // Iterate through the outputs.
        TF_FOR_ALL(output, node.GetOutputsIterator()) {

            const VdfConnectionVector &outputConnections =
                output->second->GetConnections();

            TF_FOR_ALL(iter, outputConnections) {

                // Recurse through our outputs, this time with one less output
                // depth.  
                // We use a depth of zero on the inputs of our outputs, because
                // we don't want any of their inputs drawn.  Another possibly
                // good option is 1.

                _GetLimitedNodes(
                    (*iter)->GetTargetNode(), 0, maxOutDepth - 1, nodesToGraph);
            }
        }
    }
}


void 
Vdf_DotGrapher::_PrintHeader() const
{
    _os << "digraph network {\n";

    // Configure the page direction (Top-Bottom)
    _os << "\trankdir=TB;\n";       

    // Configure the page dimensions
    double w = _options.GetPageWidth();
    double h = _options.GetPageHeight();

    if (w > 0.0 && h > 0.0)
        _os << "\tpage=\"" << w << ", " << h << "\";\n";   

    _os << "\n";
}

void 
Vdf_DotGrapher::_PrintFooter() const
{
    _os << "}\n";
}

std::string 
Vdf_DotGrapher::_DotId(const VdfNode &node) const
{
    if (_options.GetUniqueIds()) {
        return _FormatAddress(&node);
    }
    return "\"" + node.GetDebugName() + "\"";
}

PXR_NAMESPACE_CLOSE_SCOPE
