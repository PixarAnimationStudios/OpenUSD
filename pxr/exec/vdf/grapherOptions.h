//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_GRAPHER_OPTIONS_H
#define PXR_EXEC_VDF_GRAPHER_OPTIONS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/grapher.h"
#include "pxr/exec/vdf/object.h"

#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/stl.h"

#include <functional>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNode;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfGrapherOptions
/// 
/// This class can be used to configure aspects of VdfGrapher's output.
///
class VdfGrapherOptions 
{

public:

    /// The display styles for nodes.
    ///
    /// DisplayStyleFull: this is the default style and draws the full node.
    /// DisplayStyleNoLabels: this draws the node as a box with a name in it.
    /// DisplayStyleSummary: this draws the node as a small filled circle.
    ///
    enum DisplayStyle {
        DisplayStyleFull = 0,
        DisplayStyleNoLabels,
        DisplayStyleSummary
    };

    /// This typedef describes the function signature for callbacks used to
    /// filter nodes out of the graph.  Returns true if node should be in
    /// the graph, false if it should be left out.
    ///
    using NodeFilterCallback = std::function<
        bool (const VdfNode &nodeToFilter)>;

    /// This callback is used to determine what style a specific node should
    /// be rendered with regardless what was set via SetDisplayStyle().
    ///
    using NodeStyleCallback = std::function<
        DisplayStyle (
            const VdfNode &node,
            const VdfConnectionVector &drawnIn,
            const VdfConnectionVector &drawnOut)>;

    /// This struct is used to allow the grapher to graph a subset of the
    /// nodes.
    ///
    struct NodeLimit {
        NodeLimit(const VdfNode *n, int maxin, int maxout) :
            node(n),
            maxInDepth(maxin),
            maxOutDepth(maxout) 
        {}

        const VdfNode *node;
        int maxInDepth;
        int maxOutDepth;
    };

    typedef std::vector<NodeLimit> NodeLimitVector;

public:

    VDF_API
    VdfGrapherOptions();

    /// When \p drawMasks is \c true, the masks on the connections will 
    /// be drawn.
    /// 
    void SetDrawMasks(bool drawMasks)  {  _drawMasks = drawMasks; }

    /// When \p drawMasks is \c true, the affects masks on node outputs will
    /// be drawn.  Setting this to true implies SetPrintSingleOutputs() true
    /// as well.
    /// 
    void SetDrawAffectsMasks(bool drawMasks)  {  _drawAffectsMasks = drawMasks; }

    /// Returns whether or not masks will be draw on the connections.
    ///
    bool GetDrawMasks() const { return _drawMasks; }

    /// Returns whether or not masks will be draw on the connections.
    ///
    bool GetDrawAffectsMasks() const { return _drawAffectsMasks; }

    /// When \p enable is false, nodes containing only a single output, won't 
    /// render their full connector to reduce clutter.
    ///
    void SetPrintSingleOutputs(bool enable) {
        _printSingleOutputs = enable; }

    /// Returns true, if skipping single outputs is enabled.
    ///
    bool GetPrintSingleOutputs() const {
        return _printSingleOutputs; }

    /// Sets the desired size of the page output.  Setting the width and height
    /// to -1 will disable the page statement in the dot file altogther (which
    /// is useful when outputting as .tif file).
    ///
    void SetPageSize(double width, double height) { 
        _pageWidth = width;
        _pageHeight = height;
    }

    /// Returns the page height
    ///
    double GetPageHeight() const { return _pageHeight; }

    /// Returns the page width
    ///
    double GetPageWidth() const { return _pageWidth; }

    /// When \p uniqueIds is \c false, the graph will be printed without using 
    /// unique ids for node names and ports. 
    ///
    /// This will likely produce a graph that is not valid for graphing,
    /// but can be very useful for comparing output in a test, where we need ids
    /// to be exactly the same after each run. 
    ///
    void SetUniqueIds(bool uniqueIds) { _uniqueIds = uniqueIds; }

    /// Returns whether or not the graph should use unique ids.
    ///
    bool GetUniqueIds() const { return _uniqueIds; }

    /// When \p omit is set, unconnected specs will be omitted.
    ///
    void SetOmitUnconnectedSpecs(bool omit) { _omitUnconnectedSpecs = omit; }

    /// Returns whether or not the produced graph should include unconnected
    /// specs (ie. input and output ports).
    ///
    bool GetOmitUnconnectedSpecs() const { return _omitUnconnectedSpecs; }

    /// When \p drawColorizedConnectionsOnly is set, only connections that have
    /// a color set via SetColor() will be drawn.
    ///
    void SetDrawColorizedConnectionsOnly(bool drawColorizedConnectionsOnly) {
        _drawColorizedConnectionsOnly = drawColorizedConnectionsOnly;
    }

    /// Returns whether connections that have not a color set via SetColor() 
    /// should not be drawn.
    ///
    bool GetDrawColorizedConnectionsOnly() const {
        return _drawColorizedConnectionsOnly;
    }

    /// Adds \p node to the list of nodes to be graphed.
    ///
    /// If this list is empty, the entire graph will be printed.
    /// The parameters \p maxInDepth and \p maxOutDepth determine the 
    /// the depths of the traversal in both directions.
    /// 
    void AddNodeToGraph(const VdfNode &node, int maxInDepth, int maxOutDepth) {
        _nodesToGraph.push_back(NodeLimit(&node, maxInDepth, maxOutDepth));
    }

    /// Sets a \p color for \p object which can be a connection or node.
    ///
    /// Color must be in a format that is understood by dot.  Lowercase English
    /// color names usually work, (e.g. "red", "green", "blue").
    ///
    void SetColor(const VdfObjectPtr &object, const TfToken &color) {

        if (!color.IsEmpty())
            _objectColors[object] = color;
    }

    /// Returns the color for \p object or the empty TfToken if none was set.
    ///
    TfToken GetColor(const VdfObjectPtr &object) const {
        return TfMapLookupByValue(_objectColors, object, TfToken());
    }

    /// Sets an annotation \p text for \p object which gets rendered for the
    /// object.
    ///
    void SetAnnotation(const VdfObjectPtr &object, const std::string &text) {
        _objectAnnotations[object] = text;
    }

    /// Returns the annotation for \p object or the empty string if none was 
    /// set. 
    ///
    std::string GetAnnotation(const VdfObjectPtr &object) const {
        return TfMapLookupByValue(_objectAnnotations, object, std::string());
    }

    /// Returns the list of nodes that should be graphed.
    ///
    const NodeLimitVector &GetNodesToGraph() const { return _nodesToGraph; }

    /// Sets the callback used used to filter nodes out of the graph.
    ///
    /// If callback returns true the node should be in the graph,
    /// if false if it should be left out.
    ///
    void SetNodeFilterCallback(const NodeFilterCallback &callback) {
        _nodeFilterCallback = callback;
    }

    /// Returns the callback used to filter nodes out of the graph
    const NodeFilterCallback &GetNodeFilterCallback() const {
        return _nodeFilterCallback;
    }    

    /// Sets the callback used used to style nodes.  This style will override
    /// the default style set via SetDisplayStyle().
    ///
    void SetNodeStyleCallback(const NodeStyleCallback &callback) {
        _nodeStyleCallback = callback;
    }

    /// Returns the (optional) callback used to style nodes.
    ///
    const NodeStyleCallback &GetNodeStyleCallback() const {
        return _nodeStyleCallback;
    }    

    /// Filters nodes based on debug names, when used as a NodeFilterCallback.
    ///
    /// If any of the strings in \p nameList are a substring of the debug
    /// name of \p node, returns \p includeIfInNameList, including/excluding
    /// \p node from the graph.
    ///
    VDF_API
    static bool DebugNameFilter(
        const std::vector<std::string> &nameList,
        bool includeIfInNameList,
        const VdfNode &node );

    /// Sets the default display style for nodes.
    ///
    void SetDisplayStyle(DisplayStyle style) {
        _displayStyle = style;
    }

    /// Returns the default display style for a node.
    ///
    DisplayStyle GetDisplayStyle() const { return _displayStyle; }

private:

    // Draws the masks on the connections if true.
    bool _drawMasks;

    // Draws the affects masks on outputs if true.
    bool _drawAffectsMasks;

    // The width of the page
    double _pageWidth;

    // The height of the page
    double _pageHeight;

    // Determines whether or not unique IDs are used.
    bool _uniqueIds;

    // The subset of nodes to draw.  If this list is empty, everything 
    // in the network is drawn.
    NodeLimitVector _nodesToGraph;

    // The callback used to filter nodes out of the graph.  
    NodeFilterCallback _nodeFilterCallback;
    
    // The callback used to style nodes.
    NodeStyleCallback _nodeStyleCallback;

    // The display style for the nodes
    DisplayStyle _displayStyle;

    // If true, nodes that have a single output, will render that output.
    bool _printSingleOutputs;

    // Map of colored objects to color name.
    TfHashMap<VdfObjectPtr, TfToken, VdfObjectPtr::HashFunctor> _objectColors;
    
    // Map of annotated objects to annotation.
    TfHashMap<VdfObjectPtr, std::string, VdfObjectPtr::HashFunctor> _objectAnnotations;
    
    // If true, unconnected inputs/outputs will be omitted.
    bool _omitUnconnectedSpecs;
    
    // If true, draw connections with explicitly SetColor() only.
    bool _drawColorizedConnectionsOnly;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
