//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/pcp/diagnostic.h"

#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/composeSite.h"
#include "pxr/usd/pcp/debugCodes.h"
#include "pxr/usd/pcp/dependencies.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/node_Iterator.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/siteUtils.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/assign/list_of.hpp>
#include <fstream>
#include <sstream>

typedef std::set<PcpNodeRef> Pcp_NodeSet;

static const char*
_GetString(bool b) 
{
    return b ? "TRUE" : "FALSE";
}

typedef std::map<PcpNodeRef, int> _NodeToStrengthOrderMap;
typedef std::map<PcpNodeRef, SdfPrimSpecHandleVector> _NodeToPrimSpecsMap;

std::string Pcp_Dump(
    const PcpNodeRef& node,
    const _NodeToStrengthOrderMap& nodeToStrengthOrder,
    const _NodeToPrimSpecsMap& nodeToPrimSpecs,
    bool includeInheritOriginInfo,
    bool includeMaps)
{
    const PcpNodeRef parentNode = node.GetParentNode();

    std::string s;
    s += TfStringPrintf("Node %d:\n",
                        TfMapLookupByValue(nodeToStrengthOrder, node, 0));
    s += TfStringPrintf("    Parent node:              %s\n", 
        parentNode
        ? TfStringify(
            TfMapLookupByValue(nodeToStrengthOrder, parentNode, 0)).c_str() 
        : "NONE");
    s += TfStringPrintf("    Type:                     %s\n", 
        TfEnum::GetDisplayName(node.GetArcType()).c_str());

    // Dependency info
    s += "    DependencyType:           ";
    const int depFlags = PcpClassifyNodeDependency(node);
    s += PcpDependencyFlagsToString(depFlags) + "\n";

    s += TfStringPrintf("    Source path:              <%s>\n", 
        node.GetPath().GetText());
    s += TfStringPrintf("    Source layer stack:       %s\n", 
        TfStringify(node.GetLayerStack()).c_str());
    s += TfStringPrintf("    Target path:              <%s>\n", 
        parentNode
        ?  parentNode.GetPath().GetText()
        : "NONE");
    s += TfStringPrintf("    Target layer stack:       %s\n", 
        parentNode 
        ? TfStringify(parentNode.GetLayerStack()).c_str()
        : "NONE");

    const PcpNodeRef originNode = node.GetOriginNode();
    if (originNode != parentNode) {
        s += TfStringPrintf("    Origin node:              %d\n",
            TfMapLookupByValue(nodeToStrengthOrder, originNode, 0));
        s += TfStringPrintf("    Sibling # at origin:      %d\n",
            node.GetSiblingNumAtOrigin());
    }

    if (includeMaps) {
        s += TfStringPrintf("    Map to parent:\n");
        s += TfStringPrintf("        %s\n",
            TfStringReplace(
                node.GetMapToParent().GetString(),
                "\n", "\n        ").c_str());
        s += TfStringPrintf("    Map to root:\n");
        s += TfStringPrintf("        %s\n",
            TfStringReplace(
                node.GetMapToParent().GetString(),
                "\n", "\n        ").c_str());
    }

    s += TfStringPrintf("    Namespace depth:          %d\n",
        node.GetNamespaceDepth());
    s += TfStringPrintf("    Depth below introduction: %d\n", 
        node.GetDepthBelowIntroduction());
    s += TfStringPrintf("    Permission:               %s\n",
        TfEnum::GetDisplayName(node.GetPermission()).c_str());
    s += TfStringPrintf("    Is restricted:            %s\n", 
        _GetString(node.IsRestricted()));
    s += TfStringPrintf("    Is inert:                 %s\n", 
        _GetString(node.IsInert()));
    s += TfStringPrintf("    Contribute specs:         %s\n",
        _GetString(node.CanContributeSpecs()));
    s += TfStringPrintf("    Has specs:                %s\n",
        _GetString(node.HasSpecs()));
    s += TfStringPrintf("    Has symmetry:             %s\n",
        _GetString(node.HasSymmetry()));
    s += TfStringPrintf("    Has variant selection:    %s\n",
        _GetString(node.HasVariantSelections()));

    const SdfPrimSpecHandleVector* specs =
        TfMapLookupPtr(nodeToPrimSpecs, node);
    if (specs) {
        s += "    Prim stack:\n";
        TF_FOR_ALL(primIt, *specs) {
            const SdfPrimSpecHandle& primSpec = *primIt;
            std::string layerPath;
            SdfLayer::FileFormatArguments args;
            SdfLayer::SplitIdentifier( primSpec->GetLayer()->GetIdentifier(),
                                       &layerPath, &args );
            std::string basename = TfGetBaseName(layerPath);
            s += TfStringPrintf("      <%s> %s - @%s@\n",
                                primSpec->GetPath().GetText(),
                                basename.c_str(),
                                primSpec->GetLayer()->GetIdentifier().c_str());
        }
    }

    TF_FOR_ALL(childIt, Pcp_GetChildrenRange(node)) {
        s += Pcp_Dump(
            *childIt, nodeToStrengthOrder, nodeToPrimSpecs,
            includeInheritOriginInfo, includeMaps);
    }
    s += "\n";
    return s;
}

std::string PcpDump(
    const PcpNodeRef& rootNode,
    bool includeInheritOriginInfo,
    bool includeMaps)
{
    if (not rootNode) {
        return std::string();
    }

    struct _Collector {
        _Collector(const PcpNodeRef& node)
        { 
            int tmp = 0;
            _CollectRecursively(node, tmp); 
        }

        void _CollectRecursively(const PcpNodeRef& node, int& nodeIdx)
        {
            nodeToStrengthMap[node] = nodeIdx++;
            TF_FOR_ALL(childIt, Pcp_GetChildrenRange(node)) {
                _CollectRecursively(*childIt, nodeIdx);
            }
        }

        _NodeToStrengthOrderMap nodeToStrengthMap;
    };

    _Collector c(rootNode);
    return Pcp_Dump(
        rootNode, c.nodeToStrengthMap, _NodeToPrimSpecsMap(), 
        includeInheritOriginInfo, includeMaps);
}

std::string PcpDump(
    const PcpPrimIndex& primIndex,
    bool includeInheritOriginInfo,
    bool includeMaps)
{
    if (not primIndex.GetRootNode()) {
        return std::string();
    }

    _NodeToStrengthOrderMap nodeToIndexMap;
    _NodeToPrimSpecsMap nodeToSpecsMap;
    {
        int nodeIdx = 0;
        TF_FOR_ALL(it, primIndex.GetNodeRange()) {
            nodeToIndexMap[*it] = nodeIdx++;
        }

        TF_FOR_ALL(it, primIndex.GetPrimRange()) {
            const SdfPrimSpecHandle prim = SdfGetPrimAtPath(*it);
            nodeToSpecsMap[it.base().GetNode()].push_back(prim);
        }
    }

    return Pcp_Dump(
        primIndex.GetRootNode(), nodeToIndexMap, nodeToSpecsMap,
        includeInheritOriginInfo, includeMaps);
}

static void
_WriteGraphHeader(std::ostream& out)
{
    out << "digraph PcpPrimIndex {\n";
}

static int
_WriteGraph(
    std::ostream& out, 
    const PcpNodeRef& node,
    bool includeInheritOriginInfo,
    bool includeMaps,
    const Pcp_NodeSet& nodesToHighlight = Pcp_NodeSet(),
    int count = 0)
{
    if (not node) {
        // This usually happens if we don't have a root node yet. To
        // ensure we see something in the graph, just write out an empty
        // node.
        out << "\t0 [label=\"...\",shape=box,style=dotted];\n";
        return 0;
    }

    bool hasSpecs = false;
    if (node.CanContributeSpecs()) {
        hasSpecs = PcpComposeSiteHasPrimSpecs(node.GetSite());
    }

    std::vector<std::string> status;
    if (node.IsRestricted()) {
        status.push_back("permission denied");
    }
    if (node.IsInert()) {
        status.push_back("inert");
    }
    if (node.IsCulled()) {
        status.push_back("culled");
    }

    std::string nodeDesc;
    if (not status.empty()) {
        nodeDesc = "\\n" + TfStringJoin(status, ", ");
    }

    if (not node.CanContributeSpecs()) {
        nodeDesc += "\\nCANNOT contribute specs";
    }
    nodeDesc += TfStringPrintf("\\ndepth: %i", node.GetNamespaceDepth());

    std::string nodeStyle = (hasSpecs ? "solid" : "dotted");
    if (nodesToHighlight.count(node) != 0) {
        nodeStyle += ", filled";
    }
    
    out << TfStringPrintf(
        "\t%zu [label=\"%s (%i)\\n%s\", shape=\"box\", style=\"%s\"];\n",
        size_t(node.GetUniqueIdentifier()),
        Pcp_FormatSite(node.GetSite()).c_str(),
        count,
        nodeDesc.c_str(),
        nodeStyle.c_str());

    count++;

    std::string msg;
    if (includeMaps) {
        msg += TfStringPrintf("\n");
        msg += "-- mapToParent:\n" + node.GetMapToParent().GetString() + "\n";

        if (not node.GetMapToRoot().IsNull() and 
            not node.GetMapToRoot().IsIdentity()) {
            msg += "-- mapToRoot:\n" + node.GetMapToRoot().GetString() + "\n";
        }
        // Replace newlines with escape sequence graphviz uses for newlines.
        msg = TfStringReplace(msg, "\n", "\\l");
    }

    std::string style;
    switch(node.GetArcType()) {
    case PcpArcTypeLocalInherit:
        style += TfStringPrintf("color=green, label=\"local inherit%s\"",
                                msg.c_str());
    break;
    case PcpArcTypeGlobalInherit:
        style += TfStringPrintf("color=green, label=\"global inherit%s\"",
                                msg.c_str());
        break;
    case PcpArcTypeReference:
        style += TfStringPrintf("color=red, label=\"reference%s\"",
                            msg.c_str());
        break;
    case PcpArcTypeRelocate:
        style += TfStringPrintf("color=purple, label=\"relocate%s\"",
                                msg.c_str());
        break;
    case PcpArcTypeVariant:
        style += TfStringPrintf("color=orange, label=\"variant\"");
        break;
    case PcpArcTypePayload:
        style += TfStringPrintf("color=indigo, label=\"payload%s\"",
                                msg.c_str());
        break;
    case PcpArcTypeLocalSpecializes:
        style += TfStringPrintf("color=sienna, label=\"local specializes%s\"",
                                msg.c_str());
        break;
    case PcpArcTypeGlobalSpecializes:
        style += TfStringPrintf("color=sienna, label=\"global specializes%s\"",
                                msg.c_str());
        break;

    case PcpArcTypeRoot:
        break;
    case PcpNumArcTypes:
        TF_CODING_ERROR("Invalid arc type");
        break;
    }
    if (node.GetOriginNode() and 
        node.GetOriginNode() != node.GetParentNode()) {
        if (not style.empty()) 
            style += ", ";
        style += "style=dashed";
    }

// XXX should we include the sibling #?
//        if (not style.empty()) 
//            style += ", ";
//        style += TfStringPrintf("label=\"%i\"", edge.siblingNumAtOrigin);

    // Parent arc
    if (node.GetParentNode()) {
        out << TfStringPrintf(
                   "\t%zu -> %zu [%s];\n",
                   size_t(node.GetParentNode().GetUniqueIdentifier()),
                   size_t(node.GetUniqueIdentifier()),
                   style.c_str());
    }

    // Origin arc
    if (includeInheritOriginInfo and
        node.GetOriginNode() and 
        node.GetOriginNode() != node.GetParentNode()) {
        out << TfStringPrintf(
                   "\t%zu -> %zu [style=dotted label=\"origin\" "
                   "constraint=\"false\"];\n",
                   size_t(node.GetUniqueIdentifier()),
                   size_t(node.GetOriginNode().GetUniqueIdentifier()));
    }

    // Arbitrary-order traversal.
    TF_FOR_ALL(child, Pcp_GetChildrenRange(node)) {
        count = _WriteGraph(
            out, *child, includeInheritOriginInfo, includeMaps, 
            nodesToHighlight, count);
    }
    return count;
}

static void
_WriteGraphFooter(std::ostream& out)
{
    out << "}\n";
}

void
PcpDumpDotGraph(
    const PcpPrimIndex& primIndex,
    const char *filename, 
    bool includeInheritOriginInfo,
    bool includeMaps)
{
    PcpDumpDotGraph(
        primIndex.GetRootNode(), filename, 
        includeInheritOriginInfo, includeMaps);
}

void
PcpDumpDotGraph(
    const PcpNodeRef& node, 
    const char *filename, 
    bool includeInheritOriginInfo,
    bool includeMaps)
{
    if (not node) {
        return;
    }

    std::ofstream f(filename, std::ofstream::out | std::ofstream::trunc);
    if (f) {
        _WriteGraphHeader(f);
        _WriteGraph(f, node, includeInheritOriginInfo, includeMaps);
        _WriteGraphFooter(f);
    } else {
        TF_RUNTIME_ERROR("Could not write to %s\n", filename);
    }
}

std::string
Pcp_FormatSite(const PcpSite& site)
{
    std::ostringstream stream;
    stream << PcpIdentifierFormatBaseName << site;
    return stream.str();
}

std::string
Pcp_FormatSite(const PcpLayerStackSite& site)
{
    std::ostringstream stream;
    stream << PcpIdentifierFormatBaseName << site;
    return stream.str();
}

////////////////////////////////////////////////////////////

// Helper class for managing the output of the various graph debugging
// annotations.
class Pcp_GraphOutputManager : public boost::noncopyable
{
public:
    Pcp_GraphOutputManager();

    void BeginGraph(PcpPrimIndex* index, const PcpLayerStackSite& site);
    void EndGraph();

    void BeginPhase(
        const std::string& msg, const PcpNodeRef& nodeForPhase = PcpNodeRef());
    void EndPhase();

    void Update(const std::string& msg, const PcpNodeRef& updatedNode);
    void Msg(const std::string& msg, const Pcp_NodeSet& nodes);

private:
    struct _Phase {
        _Phase(const std::string& desc) 
            : description(desc)
            { }

        std::string description;
        Pcp_NodeSet nodesToHighlight;
        std::vector<std::string> messages;
    };

    struct _Graph {
        _Graph(PcpPrimIndex* index_, const SdfPath& path_)
            : index(index_), path(path_), needsOutput(false) 
            { }

        PcpPrimIndex* index;
        SdfPath path;

        bool needsOutput;
        std::string dotGraph;
        std::string dotGraphLabel;

        std::vector<_Phase> phases;
    };

private:
    void _OutputToTerminal(const std::string& msg) const;
    void _OutputGraph() const;

    void _FlushGraphIfNeedsOutput();

    void _UpdateCurrentDotGraph();
    void _UpdateCurrentDotGraphLabel();

    size_t _GetNumPhases() const;

private:
    std::vector<_Graph> _graphs;
    mutable int _nextGraphFileIndex;
};

Pcp_GraphOutputManager::Pcp_GraphOutputManager()
    : _nextGraphFileIndex(0)
{
}

size_t 
Pcp_GraphOutputManager::_GetNumPhases() const
{
    size_t phases = 0;
    TF_FOR_ALL(graphIt, _graphs) {
        phases += graphIt->phases.size();
    }
    return phases;
}

void 
Pcp_GraphOutputManager::_OutputToTerminal(const std::string& msg) const
{
    const size_t indent = _GetNumPhases();
    const size_t numSpacesPerIndent = 4;
    const std::string indentation(indent * numSpacesPerIndent, ' ');
    const std::string finalMsg = TfStringReplace(msg, "\n", "\n" + indentation);

    // Dump to terminal.
    TfDebug::Helper::Msg(indentation);
    TfDebug::Helper::Msg(finalMsg);
    TfDebug::Helper::Msg("\n");
}

void 
Pcp_GraphOutputManager::_OutputGraph() const
{
    if (not TfDebug::IsEnabled(PCP_PRIM_INDEX_GRAPHS)) {
        return;
    }

    if (not TF_VERIFY(not _graphs.empty())) {
        return;
    }

    const _Graph& currentGraph = _graphs.back();

    // Figure out the next filename and open it for writing.
    const std::string filename =
        TfStringPrintf(
            "pcp.%s.%06d.dot",
            TfStringReplace(_graphs.front().path.GetName(), "/", "_").c_str(),
            _nextGraphFileIndex);

    std::ofstream f(
        filename.c_str(), std::ofstream::out | std::ofstream::trunc);
    if (not f) {
        TF_RUNTIME_ERROR("Unable to open %s to write graph", filename.c_str());
        return;
    }

    _nextGraphFileIndex++;

    // Write the graph and label out to the file.
    ::_WriteGraphHeader(f);

    f << "\tlabel = <" << currentGraph.dotGraphLabel << ">\n";
    f << "\tlabelloc = b\n";
    f << currentGraph.dotGraph;

    ::_WriteGraphFooter(f);
}

void 
Pcp_GraphOutputManager::_UpdateCurrentDotGraph()
{
    if (not TfDebug::IsEnabled(PCP_PRIM_INDEX_GRAPHS)) {
        return;
    }

    if (not TF_VERIFY(not _graphs.empty()) or
        not TF_VERIFY(not _graphs.back().phases.empty())) {
        return;
    }

    _Graph& currentGraph = _graphs.back();
    const _Phase& currentPhase = currentGraph.phases.back();

    std::stringstream ss;

    ::_WriteGraph(
        ss, 
        currentGraph.index->GetRootNode(),
        /* includeInheritOriginInfo = */ true,
        /* includeMaps = */ false, 
        currentPhase.nodesToHighlight);
    
    currentGraph.dotGraph = ss.str();
    currentGraph.needsOutput = true;
}

void 
Pcp_GraphOutputManager::_UpdateCurrentDotGraphLabel()
{
    if (not TfDebug::IsEnabled(PCP_PRIM_INDEX_GRAPHS)) {
        return;
    }

    if (not TF_VERIFY(not _graphs.empty()) or
        not TF_VERIFY(not _graphs.back().phases.empty())) {
        return;
    }

    _Graph& currentGraph = _graphs.back();
    const _Phase& currentPhase = currentGraph.phases.back();

    // Create a nicely formatted HTML label that contains the current and
    // queued phases.
    const std::string tableFormat = 
        "\n<table cellborder=\"0\" border=\"0\">"
        "\n<tr><td balign=\"left\" align=\"left\">"
        "\n%s"
        "\n</td></tr>"
        "\n<tr><td bgcolor=\"black\" height=\"1\" cellpadding=\"0\">"
        "\n</td></tr>"
        "\n<tr><td balign=\"left\" align=\"left\">"
        "\nTasks:<br/>"
        "\n%s"
        "\n</td></tr>"
        "\n</table>";

    int numPhases = _GetNumPhases();

    // Generate the left side of the label, which shows the current phase and
    // any associated messages.
    std::string infoAboutCurrentPhase = TfStringPrintf(
        "%d. %s\n", numPhases--, currentPhase.description.c_str());

    TF_FOR_ALL(msgIt, currentPhase.messages) {
        infoAboutCurrentPhase += "- " + *msgIt + "\n";
    }

    infoAboutCurrentPhase = 
        TfStringReplace(
            TfGetXmlEscapedString(infoAboutCurrentPhase), "\n", "<br/>\n");

    // Generate the right side of the label, which shows the stack of active
    // phases.
    int numActivePhasesToShow = 5;

    std::string infoAboutPendingPhases;
    TF_REVERSE_FOR_ALL(graphIt, _graphs) {
        if (numActivePhasesToShow == 0) {
            break;
        }

        TF_REVERSE_FOR_ALL(phaseIt, graphIt->phases) {
            if (&*phaseIt != &currentPhase) {
                infoAboutPendingPhases += 
                    TfStringPrintf(
                        "%d. %s\n", numPhases--, phaseIt->description.c_str());
                
                if (--numActivePhasesToShow == 0) {
                    break;
                }
            }
        }
    }

    infoAboutPendingPhases = 
        TfStringReplace(
            TfGetXmlEscapedString(infoAboutPendingPhases), "\n", "<br/>\n");

    currentGraph.dotGraphLabel = TfStringPrintf(
        tableFormat.c_str(), 
        infoAboutCurrentPhase.c_str(),
        infoAboutPendingPhases.c_str());
    currentGraph.needsOutput = true;
}

void 
Pcp_GraphOutputManager::_FlushGraphIfNeedsOutput()
{
    if (not _graphs.empty() and _graphs.back().needsOutput) {
        _OutputGraph();

        // Clear dirtied flags from our phase and graph structures.
        _graphs.back().phases.back().messages.clear();
        _graphs.back().needsOutput = false;
    }
}

void 
Pcp_GraphOutputManager::BeginGraph(PcpPrimIndex* index,
                                   const PcpLayerStackSite& site)
{
    _FlushGraphIfNeedsOutput();
    _graphs.push_back(_Graph(index, site.path));

    const std::string msg = TfStringPrintf(
        "Computing prim index for %s", Pcp_FormatSite(site).c_str());
    BeginPhase(msg);
}

void 
Pcp_GraphOutputManager::EndGraph()
{
    if (not TF_VERIFY(not _graphs.empty()) or
        not TF_VERIFY(not _graphs.back().phases.empty())) {
        return;
    }

    _Phase& currentPhase = _graphs.back().phases.back();

    const std::string msg = "DONE - " + currentPhase.description;
    currentPhase.messages.push_back(msg);
    _UpdateCurrentDotGraph();
    _UpdateCurrentDotGraphLabel();

    EndPhase();
    _graphs.pop_back();

    if (_graphs.empty()) {
        _OutputToTerminal(msg);
        _nextGraphFileIndex = 0;
    }
}

void 
Pcp_GraphOutputManager::BeginPhase(
    const std::string& msg, const PcpNodeRef& nodeForPhase)
{
    if (not TF_VERIFY(not _graphs.empty())) {
        return;
    }

    _OutputToTerminal(msg);
    _FlushGraphIfNeedsOutput();

    _graphs.back().phases.push_back(msg);

    if (nodeForPhase) {
        _Phase& currentPhase = _graphs.back().phases.back();
        currentPhase.nodesToHighlight.clear();
        currentPhase.nodesToHighlight.insert(nodeForPhase);
        _UpdateCurrentDotGraph();
    }

    _UpdateCurrentDotGraphLabel();
}

void 
Pcp_GraphOutputManager::EndPhase()
{
    if (not TF_VERIFY(not _graphs.empty()) or
        not TF_VERIFY(not _graphs.back().phases.empty())) {
        return;
    }

    // We don't output anything to the terminal at the end of a phase.
    // The indentation levels should be enough to delineate the phase's end.
    _FlushGraphIfNeedsOutput();

    _graphs.back().phases.pop_back();
    if (not _graphs.back().phases.empty()) {
        _UpdateCurrentDotGraph();
        _UpdateCurrentDotGraphLabel();
        _graphs.back().needsOutput = false;
    }
}

void 
Pcp_GraphOutputManager::Update(
    const std::string& msg, const PcpNodeRef& updatedNode)
{
    if (not TF_VERIFY(not _graphs.empty()) or
        not TF_VERIFY(not _graphs.back().phases.empty())) {
        return;
    }

    _OutputToTerminal(msg);
    _FlushGraphIfNeedsOutput();
    
    _Phase& currentPhase = _graphs.back().phases.back();
    currentPhase.messages.push_back(msg);
    currentPhase.nodesToHighlight = boost::assign::list_of<>(updatedNode)
        .convert_to_container<Pcp_NodeSet>();

    _UpdateCurrentDotGraph();
    _UpdateCurrentDotGraphLabel();
    _FlushGraphIfNeedsOutput();
}

void 
Pcp_GraphOutputManager::Msg(
    const std::string& msg, const Pcp_NodeSet& nodes)
{
    if (not TF_VERIFY(not _graphs.empty()) or
        not TF_VERIFY(not _graphs.back().phases.empty())) {
        return;
    }

    _OutputToTerminal(msg);

    _Phase& currentPhase = _graphs.back().phases.back();

    if (currentPhase.nodesToHighlight != nodes) {
        _FlushGraphIfNeedsOutput();

        currentPhase.nodesToHighlight = nodes;
        _UpdateCurrentDotGraph();
    }

    currentPhase.messages.push_back(msg);
    _UpdateCurrentDotGraphLabel();
}

static TfStaticData<Pcp_GraphOutputManager> _outputManager;

////////////////////////////////////////////////////////////

Pcp_GraphScope::Pcp_GraphScope(PcpPrimIndex* index,
                               const PcpLayerStackSite& site)
    : _on(TfDebug::IsEnabled(PCP_PRIM_INDEX))
{
    if (_on) {
        _outputManager->BeginGraph(index, site);
    }
}

Pcp_GraphScope::~Pcp_GraphScope()
{
    if (_on) {
        _outputManager->EndGraph();
    }
}

Pcp_PhaseScope::Pcp_PhaseScope(
    const PcpNodeRef& node, const char* msg)
    : _on(TfDebug::IsEnabled(PCP_PRIM_INDEX))
{
    if (_on) {
        _outputManager->BeginPhase(msg, node);
    }
}

Pcp_PhaseScope::~Pcp_PhaseScope()
{
    if (_on) {
        _outputManager->EndPhase();
    }
}

std::string
Pcp_PhaseScope::Helper(const char* f, ...)
{
    va_list ap;
    va_start(ap, f);
    const std::string result = TfVStringPrintf(f, ap);
    va_end(ap);
    return result;
}

void
Pcp_GraphUpdate(
    const PcpNodeRef& node, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    const std::string msg = TfVStringPrintf(format, ap);
    va_end(ap);

    _outputManager->Update(msg, node);
}

// Define overloads for the Pcp_GraphMsg functions that take different
// numbers of nodes to highlight.
#define _ADD_TO_NODES(z, n, a)                  \
    nodes.insert(BOOST_PP_CAT(a, n));

#define BOOST_PP_LOCAL_LIMITS (0, 2)
#define BOOST_PP_LOCAL_MACRO(n)                                         \
void Pcp_GraphMsg(                                                      \
    BOOST_PP_ENUM_PARAMS(n, const PcpNodeRef& node) BOOST_PP_COMMA_IF(n)\
    const char* format, ...)                                            \
{                                                                       \
    va_list ap;                                                         \
    va_start(ap, format);                                               \
    const std::string msg = TfVStringPrintf(format, ap);                \
    va_end(ap);                                                         \
                                                                        \
    Pcp_NodeSet nodes;                                                  \
    BOOST_PP_REPEAT(n, _ADD_TO_NODES, node);                            \
    _outputManager->Msg(msg, nodes);                                    \
}                                                   

#include BOOST_PP_LOCAL_ITERATE()
#undef _ADD_TO_NODES
