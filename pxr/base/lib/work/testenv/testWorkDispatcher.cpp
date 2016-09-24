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
#include "pxr/base/work/arenaDispatcher.h"
#include "pxr/base/work/dispatcher.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/poolAllocator.h"
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/arch/nap.h"

#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

#include <atomic>
#include <vector>
#include <iostream>
#include <fstream>


static const int numLevels =100; 
static const int numNodesPerLevel = 1000;
static const int maxFanIn = 3;
static const int maxSleepTime = 100;

// Returns a random integer in [m, n)
static int
GenRand(int m, int n) {
    return (int)(((float)rand() / (float)RAND_MAX) * n) + m;
}

// We're suppose to do some work that takes up some time.  The higher
// time is, the more time we should take up, and ideally it would be linear.
static void
DoWork(int time) {
    int b = 5;
    for (int i = 0; i < time; ++i) {
        b *= b;
    }
}


class Node
{
public:

    // Creates a node whose task is to wait sleepTime microseconds.
    Node(int index, int sleepTime) : _index(index), _sleepTime(sleepTime) {}

    // Returns the index of this node in the graph.
    int GetIndex() const { return _index; }

    // Returns a list of all the inputs to this node.
    const std::vector<const Node *> &GetInputs() const { return _inputs; }

    // Add an input to this node.
    void AddInput(const Node *node) { 
        _inputs.push_back(node); 
        const_cast<Node *>(node)->_outputs.push_back(this);
    }

    // Returns the list of all the outputs of this node.
    const std::vector<const Node *> &GetOutputs() const { return _outputs; }

    // Returns the number of microseconds this node is meant to sleep for.
    int GetSleepTime() const { return _sleepTime; }

    // Initializes the wait count to the number of inputs to this node.
    void InitWaitCount() { 
        _waitCount = _inputs.size();
    }

    // Decrements the wait count and returns true if the wait is now zero.
    bool DecrementWaitCount() const {
        return --_waitCount == 0;
    }

    // Custom new and delete operators for pool management.
    static void *operator new(size_t) {
        return _allocator.Alloc();
    }
    static void  operator delete (void *ptr) {
        return _allocator.Free(ptr);
    }


private:

    // The index of this node in the graph.
    int _index;

    // The amount of time (in microseconds).
    int _sleepTime;

    // All the nodes that must run before this node runs.
    std::vector<const Node *> _inputs;

    // All the nodes that this node feeds into.
    std::vector<const Node *> _outputs;

    // The number of inputs that are left to run before this node can run.
    mutable std::atomic<size_t> _waitCount;

    // Pool allocator so that this test program runs fast.
    static TfPoolAllocator _allocator;
};

// Create our pool allocator for Node.  We need this simply for speed.
#define SIZE_PER_OBJECT   sizeof(Node)
#define OBJECTS_PER_CHUNK 100000
TfPoolAllocator Node::_allocator(
                       SIZE_PER_OBJECT, SIZE_PER_OBJECT*OBJECTS_PER_CHUNK);


class Graph 
{
public:

    // Adds a node to the graph.
    //
    void AddNode(int sleepTime) {
        int index = _nodes.size();
        _nodes.push_back(new Node(index, sleepTime));
    }

    // Method called to do work on a node that can add dependencies  as
    // additional work.
    //
    template <typename DispatcherType>
    void CallbackDynamic(Node *node, DispatcherType *dispatcher)
    {
        DoWork(node->GetSleepTime());

        // Now that the node is done, loop over all its outputs and decrement
        // their counts.  If they can start, add them as available work.
        TF_FOR_ALL(output, node->GetOutputs()) {
            if ((*output)->DecrementWaitCount()) {
                dispatcher->Run(&Graph::CallbackDynamic<DispatcherType>,
                    this, const_cast<Node *>(*output), dispatcher);
            }
        }

        _numNodesRun++;
    }

    // Method called to do work on a node form a fixed dispatcher.
    //
    void CallbackFixed(Node *node)
    {
        DoWork(node->GetSleepTime());
        _numNodesRun++;
    }

    // Returns the number of nodes in the graph.
    //
    size_t GetSize() const { return _nodes.size(); }


    // Get the vector of jobs that can start right away.
    //
    void GetInitialJobsForDynamic(std::vector<Node *> *jobs) {

        TF_FOR_ALL(node, _nodes) {
            // Nodes that can run right away are nodes with zero inputs.
            if ( (*node)->GetInputs().size() == 0 ) {
                jobs->push_back(*node);
            }
            (*node)->InitWaitCount();
        }

        _numNodesRun = 0;
    }

    void GetInitialJobsForFixed(std::vector<Node *> *jobs) {

        TF_FOR_ALL(node, _nodes) {
            jobs->push_back(*node);
            (*node)->InitWaitCount();
        }

        _numNodesRun = 0;
    }

    // Returns the node at index and level.
    Node *GetNode(int index, int level) {
        return _nodes[index + numNodesPerLevel * level];
    }

    // Returns the number of nodes that have run. Since the last call to
    // InitializeWork().
    int GetNumNodesRun() const {
        return _numNodesRun;
    }

    // Writes out the graph in human readable format.
    void Save(const char *filename) const {

        std::ofstream os(filename);

        // The first line is the total number of nodes.
        os << _nodes.size() << std::endl;
        TF_FOR_ALL(node, _nodes) {
            // Each additional line is the amount of sleep followed by the
            // number of inputs followed by the input index.
            os << (*node)->GetSleepTime() << " ";
            os << (*node)->GetInputs().size() << " ";
            TF_FOR_ALL(input, (*node)->GetInputs()) {
                os << (*input)->GetIndex() << " ";
            }
            os << std::endl;
        }

    }

    // Loads a graph from file.
    void Load(const char *filename) {

        std::ifstream is(filename);

        // Number of nodes
        int numNodes;
        is >> numNodes;

        _nodes.clear();
        _nodes.reserve(numNodes);

        std::vector<int> numInputs;
        std::vector< std::vector<int> > inputs;

        inputs.resize(numNodes);

        for (int i = 0; i < numNodes; ++i) {

            int sleepTime;
            int numIns;

            is >> sleepTime;
            is >> numIns;

            AddNode(sleepTime);
            numInputs.push_back(numIns);

            for (int j = 0; j < numIns; ++j) {
                int input;
                is >> input;
                inputs[i].push_back(input);
            }
        }

        // Add all the inputs.
        for (int i = 0; i < numNodes; ++i) {
            for (int j = 0; j < numInputs[i]; ++j) {
                _nodes[i]->AddInput( _nodes[inputs[i][j]] );
            }
        }

    }

private:

    // The vector of all the nodes in this graph.
    std::vector<Node *> _nodes;

    // The number of nodes run.
    std::atomic<int> _numNodesRun;

};


static Graph *
GenerateRandomGraph() 
{
    srand(time(0));

    Graph *graph = new Graph();

    // Create the required number of nodes.
    for (int i = 0; i < numLevels * numNodesPerLevel; ++i) {
        int sleepTime =  GenRand(0, maxSleepTime);
        graph->AddNode(sleepTime);
    }

    // Generate the inputs for all nodes in levels > 0.
    // The rule is that nodes can only have as inputs nodes that are in
    // level less than itself.  All nodes in level 0 have no inputs.
    //

    for (int level = 1; level < numLevels; ++level) {
        for (int i = 0; i < numNodesPerLevel; ++i) {

            Node *node = graph->GetNode(i, level);

            // Now for node i in level, determine the number of inputs.
            int numInputs = GenRand(1, maxFanIn);

            for (int input = 0; input < numInputs; ++input) {

                // Get a random level less than level
                int randLevel = GenRand(0, level);
                // Get a random node within that level
                int randIndex = GenRand(0, numNodesPerLevel);
                node->AddInput(graph->GetNode(randIndex, randLevel));
            }
        }
    }


    return graph;

}


static Graph *
LoadGraph(const char *filename)
{
    Graph *graph  = new Graph;
    graph->Load(filename);
    return graph;
}


template <typename DispatcherType>
static bool
_TestDispatcher(Graph *graph)
{
    TfStopwatch timer;

    DispatcherType workDispatcher;

    std::cout << "\tInitializing graph" << std::endl;

    std::vector<Node *> jobs;

    graph->GetInitialJobsForDynamic(&jobs);

    timer.Reset();
    timer.Start();

    TF_FOR_ALL(i, jobs) {
        workDispatcher.Run(
            &Graph::CallbackDynamic<DispatcherType>,
            graph, *i, &workDispatcher);
    }

    workDispatcher.Wait();

    timer.Stop();

    if (graph->GetNumNodesRun() != numNodesPerLevel * numLevels) {
        std::cerr << "\tERROR: expected to run " << 
            numNodesPerLevel * numLevels << " but we only ran " << 
            graph->GetNumNodesRun() << std::endl;
        return false;
    }
    std::cout << "\tDone: in " << timer.GetMilliseconds() << " ms" << std::endl;
    return true;
}

template <typename DispatcherType>
static bool
_DelayedGraphTask(Graph *graph)
{
    std::cout << "\tSleeping..." << std::endl;
    ArchSleep(2);
    return _TestDispatcher<DispatcherType>(graph);
}

template <typename DispatcherType>
static bool
_TestDispatcherCancellation(Graph *graph)
{
    // Calling Cancel() on a dispatcher should only affect tasks that
    // it has directly been given to run. If those tasks use their
    // own dispatchers to run other tasks, those tasks should *not*
    // be cancelled.
    //
    // We use sleep here and in the task to ensure the task begins
    // running before the call to Cancel() occurs. Otherwise, the
    // task will never have a chance to start, which would make this
    // test useless.
    DispatcherType parentDispatcher;

    parentDispatcher.Run(&_DelayedGraphTask<DispatcherType>, graph);
    ArchSleep(1);
    std::cout << "\tCancelling..." << std::endl;
    parentDispatcher.Cancel();
    parentDispatcher.Wait();
    
    return graph->GetNumNodesRun() == numNodesPerLevel * numLevels;
}

int
main(int argc, char **argv)
{
    boost::scoped_ptr<Graph> graph;

    if (argc < 2) {
        std::cout << "Generating random graph" << std::endl;
        graph.reset(GenerateRandomGraph());
        graph->Save("graph.txt");
    } else {
        std::cout << "Loading " << argv[1] << std::endl;
        graph.reset(LoadGraph(argv[1]));
    }

    if (not graph) {
        std::cerr << "Error getting a graph" << std::endl;
        return 1;
    }

    // Test the general dispatcher.
    {
        std::cout << "Using the general dispatcher" << std::endl;
        if (not _TestDispatcher<WorkDispatcher>(graph.get())) {
            return 1;
        }

        if (not _TestDispatcherCancellation<WorkDispatcher>(graph.get())) {
            return 1;
        }
    }

    // Test the arena dispatcher.
    {
        std::cout << "Using the arena dispatcher" << std::endl;
        if (not _TestDispatcher<WorkArenaDispatcher>(graph.get())) {
            return 1;
        }

        if (not _TestDispatcherCancellation<WorkArenaDispatcher>(graph.get())) {
            return 1;
        }
    }

    return 0;

}
