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

#include "pxr/pxr.h"

#include "pxr/usd/pcp/diagnostic.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/composeSite.h"
#include "pxr/usd/pcp/debugCodes.h"
#include "pxr/usd/pcp/dependencies.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/node_Iterator.h"
#include "pxr/usd/pcp/primIndex_StackFrame.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/siteUtils.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/stringUtils.h"

#include <tbb/concurrent_hash_map.h>
#include <fstream>
#include <mutex>
#include <sstream>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

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
    if (!rootNode) {
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
    if (!primIndex.GetRootNode()) {
        return std::string();
    }

    _NodeToStrengthOrderMap nodeToIndexMap;
    _NodeToPrimSpecsMap nodeToSpecsMap;
    {
        int nodeIdx = 0;
        for (const PcpNodeRef &node: primIndex.GetNodeRange()) {
            nodeToIndexMap[node] = nodeIdx++;
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
    if (!node) {
        // This usually happens if we don't have a root node yet. To
        // ensure we see something in the graph, just write out an empty
        // node.
        out << "\t0 [label=\"...\",shape=box,style=dotted];\n";
        return 0;
    }

    bool hasSpecs = false;
    if (node.CanContributeSpecs()) {
        hasSpecs = PcpComposeSiteHasPrimSpecs(node);
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
    if (!status.empty()) {
        nodeDesc = "\\n" + TfStringJoin(status, ", ");
    }

    if (!node.CanContributeSpecs()) {
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

        if (!node.GetMapToRoot().IsNull() &&
            !node.GetMapToRoot().IsIdentity()) {
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
    if (node.GetOriginNode() &&
        node.GetOriginNode() != node.GetParentNode()) {
        if (!style.empty()) 
            style += ", ";
        style += "style=dashed";
    }

// XXX should we include the sibling #?
//        if (!style.empty()) 
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
    if (includeInheritOriginInfo &&
        node.GetOriginNode()     &&
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
    if (!node) {
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

// Helper class for managing the output of the various indexing debugging
// annotations.
class Pcp_IndexingOutputManager
{
public:
    Pcp_IndexingOutputManager();
    Pcp_IndexingOutputManager(Pcp_IndexingOutputManager const &) = delete;
    Pcp_IndexingOutputManager &
    operator=(Pcp_IndexingOutputManager const &) = delete;

    void PushIndex(PcpPrimIndex const *originatingIndex,
                   PcpPrimIndex const *index, PcpLayerStackSite const &site);
    void PopIndex(PcpPrimIndex const *originatingIndex);

    void BeginPhase(PcpPrimIndex const *originatingIndex,
                    std::string &&msg,
                    const PcpNodeRef& nodeForPhase = PcpNodeRef());
    void EndPhase(PcpPrimIndex const *originatingIndex);

    void Update(PcpPrimIndex const *originatingIndex,
                const PcpNodeRef& updatedNode, std::string &&msg);

    void Msg(PcpPrimIndex const *originatingIndex,
             std::string &&msg, const Pcp_NodeSet& nodes);

private:
    struct _Phase {
        explicit _Phase(std::string &&desc) : description(std::move(desc)) {}
        std::string description;
        Pcp_NodeSet nodesToHighlight;
        std::vector<std::string> messages;
    };

    struct _IndexInfo {
        _IndexInfo(PcpPrimIndex const *index, const SdfPath& path)
            : index(index)
            , path(path)
            , needsOutput(false) {}

        PcpPrimIndex const *index;
        SdfPath path;
        std::string dotGraph;
        std::string dotGraphLabel;
        std::vector<_Phase> phases;
        bool needsOutput;
    };

    struct _DebugInfo {
        void BeginPhase(std::string &&msg,
                        const PcpNodeRef& nodeForPhase = PcpNodeRef()) {
            if (!TF_VERIFY(!indexStack.empty())) {
                return;
            }
            WriteDebugMessage(msg);
            FlushGraphIfNeedsOutput();
            indexStack.back().phases.emplace_back(std::move(msg));
            if (nodeForPhase) {
                _Phase& currentPhase = indexStack.back().phases.back();
                currentPhase.nodesToHighlight.clear();
                currentPhase.nodesToHighlight.insert(nodeForPhase);
                UpdateCurrentDotGraph();
            }
            UpdateCurrentDotGraphLabel();
        }
        
        void EndPhase() {
            if (!TF_VERIFY(!indexStack.empty()) ||
                !TF_VERIFY(!indexStack.back().phases.empty())) {
                return;
            }
            // We don't output anything to the terminal at the end of a phase.
            // The indentation levels should be enough to delineate the phase's
            // end.
            FlushGraphIfNeedsOutput();
            
            indexStack.back().phases.pop_back();
            if (!indexStack.back().phases.empty()) {
                UpdateCurrentDotGraph();
                UpdateCurrentDotGraphLabel();
                indexStack.back().needsOutput = false;
            }
        }
        
        void Update(const PcpNodeRef& updatedNode, std::string &&msg) {
            if (!TF_VERIFY(!indexStack.empty()) ||
                !TF_VERIFY(!indexStack.back().phases.empty())) {
                return;
            }
            
            WriteDebugMessage(msg);
            FlushGraphIfNeedsOutput();
            
            _Phase& currentPhase = indexStack.back().phases.back();
            currentPhase.messages.push_back(std::move(msg));
            currentPhase.nodesToHighlight = { updatedNode };
            
            UpdateCurrentDotGraph();
            UpdateCurrentDotGraphLabel();
            FlushGraphIfNeedsOutput();
        }
        
        void Msg(std::string &&msg, const Pcp_NodeSet& nodes) {
            if (!TF_VERIFY(!indexStack.empty()) ||
                !TF_VERIFY(!indexStack.back().phases.empty())) {
                return;
            }
            
            WriteDebugMessage(msg);
            
            _Phase& currentPhase = indexStack.back().phases.back();
            
            if (currentPhase.nodesToHighlight != nodes) {
                FlushGraphIfNeedsOutput();                
                currentPhase.nodesToHighlight = nodes;
                UpdateCurrentDotGraph();
            }
            
            currentPhase.messages.push_back(std::move(msg));
            UpdateCurrentDotGraphLabel();
        }
            
        void WriteDebugMessage(const std::string& msg) const {
            const size_t indent = GetNumPhases();
            const size_t numSpacesPerIndent = 4;
            const std::string indentation(indent * numSpacesPerIndent, ' ');
            const std::string finalMsg =
                TfStringReplace(msg, "\n", "\n" + indentation);

            // Append output.
            outputBuffer.push_back(indentation + finalMsg + "\n");
        }
        
        void OutputGraph() const {
            if (!TfDebug::IsEnabled(PCP_PRIM_INDEX_GRAPHS)) {
                return;
            }
            
            if (!TF_VERIFY(!indexStack.empty())) {
                return;
            }
            
            // Figure out the next filename and open it for writing.
            const std::string filename =
                TfStringPrintf(
                    "pcp.%s.%06d.dot",
                    TfStringReplace(indexStack.front().path.GetName(),
                                    "/", "_").c_str(),
                    nextGraphFileIndex);
            
            std::ofstream f(
                filename.c_str(), std::ofstream::out | std::ofstream::trunc);
            if (!f) {
                TF_RUNTIME_ERROR("Unable to open %s to write graph",
                                 filename.c_str());
                return;
            }
            
            ++nextGraphFileIndex;
            
            // Write the graph and label out to the file.
            _WriteGraphHeader(f);
            
            const _IndexInfo& currentIndex = indexStack.back();
            f << "\tlabel = <" << currentIndex.dotGraphLabel << ">\n";
            f << "\tlabelloc = b\n";
            f << currentIndex.dotGraph;
            
            _WriteGraphFooter(f);
        }
            
        void FlushGraphIfNeedsOutput() {
            if (!indexStack.empty() && indexStack.back().needsOutput) {
                OutputGraph();
                
                // Clear dirtied flags from our phase and graph structures.
                indexStack.back().phases.back().messages.clear();
                indexStack.back().needsOutput = false;
            }
        }
        
        void UpdateCurrentDotGraph() {
            if (!TfDebug::IsEnabled(PCP_PRIM_INDEX_GRAPHS)) {
                return;
            }

            if (!TF_VERIFY(!indexStack.empty()) ||
                !TF_VERIFY(!indexStack.back().phases.empty())) {
                return;
            }

            _IndexInfo& currentIndex = indexStack.back();
            const _Phase& currentPhase = currentIndex.phases.back();

            std::stringstream ss;

            _WriteGraph(
                ss, 
                currentIndex.index->GetRootNode(),
                /* includeInheritOriginInfo = */ true,
                /* includeMaps = */ false, 
                currentPhase.nodesToHighlight);
    
            currentIndex.dotGraph = ss.str();
            currentIndex.needsOutput = true;
        }
        
        void UpdateCurrentDotGraphLabel() {

            if (!TfDebug::IsEnabled(PCP_PRIM_INDEX_GRAPHS)) {
                return;
            }

            if (!TF_VERIFY(!indexStack.empty()) ||
                !TF_VERIFY(!indexStack.back().phases.empty())) {
                return;
            }

            _IndexInfo& currentIndex = indexStack.back();
            const _Phase& currentPhase = currentIndex.phases.back();

            // Create a nicely formatted HTML label that contains the current
            // and queued phases.
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

            int numPhases = GetNumPhases();

            // Generate the left side of the label, which shows the current
            // phase and any associated messages.
            std::string infoAboutCurrentPhase = TfStringPrintf(
                "%d. %s\n", numPhases--, currentPhase.description.c_str());

            TF_FOR_ALL(msgIt, currentPhase.messages) {
                infoAboutCurrentPhase += "- " + *msgIt + "\n";
            }

            infoAboutCurrentPhase = 
                TfStringReplace(TfGetXmlEscapedString(infoAboutCurrentPhase),
                                "\n", "<br/>\n");

            // Generate the right side of the label, which shows the stack of
            // active phases.
            int numActivePhasesToShow = 5;

            std::string infoAboutPendingPhases;
            TF_REVERSE_FOR_ALL(graphIt, indexStack) {
                if (numActivePhasesToShow == 0) {
                    break;
                }

                TF_REVERSE_FOR_ALL(phaseIt, graphIt->phases) {
                    if (&*phaseIt != &currentPhase) {
                        infoAboutPendingPhases += 
                            TfStringPrintf("%d. %s\n", numPhases--,
                                           phaseIt->description.c_str());
                
                        if (--numActivePhasesToShow == 0) {
                            break;
                        }
                    }
                }
            }

            infoAboutPendingPhases = 
                TfStringReplace(TfGetXmlEscapedString(infoAboutPendingPhases),
                                "\n", "<br/>\n");

            currentIndex.dotGraphLabel = TfStringPrintf(
                tableFormat.c_str(), 
                infoAboutCurrentPhase.c_str(),
                infoAboutPendingPhases.c_str());
            currentIndex.needsOutput = true;
        }
        
        size_t GetNumPhases() const {
            size_t numPhases = 0;
            for (auto const &indexInfo: indexStack) {
                numPhases += indexInfo.phases.size();
            }
            return numPhases;
        }

        void FlushBufferedOutput() const {
            static std::mutex mutex;
            std::lock_guard<std::mutex> lock(mutex);
            // Issue TfDebug messages.
            for (auto const &msg: outputBuffer)
                TfDebug::Helper::Msg(msg);
        }

        std::vector<_IndexInfo> indexStack;
        mutable int nextGraphFileIndex = 0;
        mutable std::vector<std::string> outputBuffer;
    };
    
private:
    using _DebugInfoMap =
        tbb::concurrent_hash_map<PcpPrimIndex const *, _DebugInfo>;

    void _Erase(PcpPrimIndex const *index) {
        _debugInfo.erase(index);
    }

    _DebugInfo *_Get(PcpPrimIndex const *index) {
        _DebugInfoMap::accessor acc;
        _debugInfo.insert(acc, index);
        return &acc->second;
    }

    _DebugInfo const *_Get(PcpPrimIndex const *index) const {
        _DebugInfoMap::const_accessor acc;
        return _debugInfo.find(acc, index) ? &acc->second : nullptr;
    }

    _DebugInfoMap _debugInfo;
};

Pcp_IndexingOutputManager::Pcp_IndexingOutputManager()
{
}

void 
Pcp_IndexingOutputManager::PushIndex(PcpPrimIndex const *originatingIndex,
                                     PcpPrimIndex const *index,
                                     const PcpLayerStackSite& site)
{
    _DebugInfo *info = _Get(originatingIndex);
    info->FlushGraphIfNeedsOutput();
    info->indexStack.emplace_back(index, site.path);
    info->BeginPhase(TfStringPrintf("Computing prim index for %s",
                                    Pcp_FormatSite(site).c_str()));
}

void 
Pcp_IndexingOutputManager::PopIndex(PcpPrimIndex const *originatingIndex)
{
    _DebugInfo *info = _Get(originatingIndex);
    if (!TF_VERIFY(!info->indexStack.empty()) ||
        !TF_VERIFY(!info->indexStack.back().phases.empty())) {
        return;
    }

    _Phase& currentPhase = info->indexStack.back().phases.back();

    currentPhase.messages.push_back("DONE - " + currentPhase.description);
    info->UpdateCurrentDotGraph();
    info->UpdateCurrentDotGraphLabel();

    info->EndPhase();
    info->indexStack.pop_back();

    if (info->indexStack.empty()) {
        // Write all the buffered output.
        info->FlushBufferedOutput();
        _Erase(originatingIndex);
    }
}

void 
Pcp_IndexingOutputManager::BeginPhase(
    PcpPrimIndex const *originatingIndex,
    std::string &&msg,
    const PcpNodeRef& nodeForPhase)
{
    _Get(originatingIndex)->BeginPhase(std::move(msg), nodeForPhase);
}

void 
Pcp_IndexingOutputManager::EndPhase(PcpPrimIndex const *originatingIndex)
{
    _Get(originatingIndex)->EndPhase();
}

void 
Pcp_IndexingOutputManager::Update(
    PcpPrimIndex const *originatingIndex,
    const PcpNodeRef& updatedNode, std::string &&msg)
{
    _Get(originatingIndex)->Update(updatedNode, std::move(msg));
}

void 
Pcp_IndexingOutputManager::Msg(
    PcpPrimIndex const *originatingIndex,
    std::string &&msg, const Pcp_NodeSet& nodes)
{
    _Get(originatingIndex)->Msg(std::move(msg), nodes);
}

static TfStaticData<Pcp_IndexingOutputManager> _outputManager;

////////////////////////////////////////////////////////////

void
Pcp_PrimIndexingDebug::_PushIndex(const PcpLayerStackSite& site) const
{
    _outputManager->PushIndex(_originatingIndex, _index, site);
}

void
Pcp_PrimIndexingDebug::_PopIndex() const
{
    _outputManager->PopIndex(_originatingIndex);
}

Pcp_IndexingPhaseScope::Pcp_IndexingPhaseScope(
    PcpPrimIndex const *index, const PcpNodeRef& node, std::string &&msg)
    : _index(index)
{
    _outputManager->BeginPhase(_index, std::move(msg), node);
}

void
Pcp_IndexingPhaseScope::_EndScope() const
{
    _outputManager->EndPhase(_index);
}

void
Pcp_IndexingUpdate(PcpPrimIndex const *index,
                   const PcpNodeRef& node, std::string &&msg)
{
    _outputManager->Update(index, node, std::move(msg));
}

void
Pcp_IndexingMsg(PcpPrimIndex const *index,
                const PcpNodeRef& a1,
                char const *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    std::string msg = TfVStringPrintf(fmt, ap);
    va_end(ap);

    Pcp_NodeSet nodes { a1 };
    _outputManager->Msg(index, std::move(msg), nodes);
}

void
Pcp_IndexingMsg(PcpPrimIndex const *index,
                const PcpNodeRef& a1, const PcpNodeRef& a2,
                char const *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    std::string msg = TfVStringPrintf(fmt, ap);
    va_end(ap);

    Pcp_NodeSet nodes { a1, a2 };
    _outputManager->Msg(index, std::move(msg), nodes);
}

PXR_NAMESPACE_CLOSE_SCOPE
