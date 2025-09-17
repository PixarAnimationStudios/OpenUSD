//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/dynamicTopologicalSorter.h"

PXR_NAMESPACE_USING_DIRECTIVE

// Use integer indices to represent the graph vertices.
typedef VdfDynamicTopologicalSorter<int> IntVertexSorter;

static void
testEmptyGraph()
{
    IntVertexSorter sorter;

    // Ensure that vertices that have never been added have an invalid
    // priority.
    TF_AXIOM(sorter.GetPriority(0) == IntVertexSorter::InvalidPriority);
    TF_AXIOM(sorter.GetPriority(1) == IntVertexSorter::InvalidPriority);

    // Try removing an edge that's not in the sorter.
    sorter.RemoveEdge(0, 1);
    TF_AXIOM(sorter.GetPriority(0) == IntVertexSorter::InvalidPriority);
    TF_AXIOM(sorter.GetPriority(1) == IntVertexSorter::InvalidPriority);

    // Try to remove a trivial loop that's not in the sorter.
    sorter.RemoveEdge(1, 1);
    TF_AXIOM(sorter.GetPriority(0) == IntVertexSorter::InvalidPriority);
    TF_AXIOM(sorter.GetPriority(1) == IntVertexSorter::InvalidPriority);

    // Clear an empty sorter.
    sorter.Clear();
}

static void
testSingleEdgeGraph()
{
    /* Construct a graph with a single edge.
     *
     *     0 -> 1
     *
     */

    IntVertexSorter sorter;

    sorter.AddEdge(0, 1);
    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(1));

    // Ensure that removing the last reference to vertices gives them
    // invalid priority.
    sorter.RemoveEdge(0, 1);
    TF_AXIOM(sorter.GetPriority(0) == IntVertexSorter::InvalidPriority);
    TF_AXIOM(sorter.GetPriority(1) == IntVertexSorter::InvalidPriority);
}

static void
testTree()
{
    /* Construct a tree with a root and two children.
     *
     *       -> 1
     *      /
     *     0
     *      \ 
     *       -> 3
     *
     */

    IntVertexSorter sorter;

    sorter.AddEdge(0, 1);
    sorter.AddEdge(0, 2);

    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(1));
    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(2));

    TF_AXIOM(sorter.GetPriority(1) != sorter.GetPriority(2));
}

static void
testTwoRoots()
{
    /* Construct a graph with two vertices that don't have incoming
     * edges and point to a third vertex.
     *
     *     0 --
     *         \
     *          v
     *          2
     *          ^
     *         /
     *     1 --
     *
     */
    IntVertexSorter sorter;

    sorter.AddEdge(0, 2);
    sorter.AddEdge(1, 2);

    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(2));
    TF_AXIOM(sorter.GetPriority(1) < sorter.GetPriority(2));

    TF_AXIOM(sorter.GetPriority(0) != sorter.GetPriority(1));
}

static void
testAcyclicDiamond()
{
    /* Construct a diamond-shaped acyclic graph with 4 vertices.
     *
     *        > 1
     *       /    \
     *      /      v
     *     0       3
     *      \      ^
     *       \    /
     *        > 2 
     *
     */

    IntVertexSorter sorter;

    sorter.AddEdge(0, 1);
    sorter.AddEdge(0, 2);
    sorter.AddEdge(1, 3);
    sorter.AddEdge(2, 3);

    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(1));
    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(2));

    TF_AXIOM(sorter.GetPriority(1) != sorter.GetPriority(2));

    TF_AXIOM(sorter.GetPriority(1) < sorter.GetPriority(3));
    TF_AXIOM(sorter.GetPriority(2) < sorter.GetPriority(3));
}

static void
testCycle()
{
    // We don't expect a reasonable order for cycles, just that
    // the program doesn't crash.

    IntVertexSorter sorter;

    sorter.AddEdge(0, 1);
    sorter.AddEdge(1, 0);

    sorter.GetPriority(0);
    sorter.GetPriority(1);
}

static void
testReorder()
{
    /* Construct a graph, then insert an edge that will require reordering.
     *
     *     2 -> 0 -> 1
     *
     */
    IntVertexSorter sorter;

    sorter.AddEdge(0, 1);
    sorter.AddEdge(2, 0);

    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(1));
    TF_AXIOM(sorter.GetPriority(2) < sorter.GetPriority(0));
}

static void
testRemoveAndReorder()
{
    /* Construct a graph, remove an edge, then insert an edge that requires
     * reordering.
     *
     *     0 -> 1 -> 2 -> 3
     *
     * Remove (1, 2)
     *
     *     0 -> 1    2 -> 3
     *
     * Insert (3, 0)
     *
     *     2 -> 3 -> 0 -> 1
     *
     */
    IntVertexSorter sorter;

    sorter.AddEdge(0, 1);
    sorter.AddEdge(1, 2);
    sorter.AddEdge(2, 3);

    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(1));
    TF_AXIOM(sorter.GetPriority(1) < sorter.GetPriority(2));
    TF_AXIOM(sorter.GetPriority(2) < sorter.GetPriority(3));

    sorter.RemoveEdge(1, 2);

    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(1));
    TF_AXIOM(sorter.GetPriority(1) != sorter.GetPriority(2));
    TF_AXIOM(sorter.GetPriority(2) < sorter.GetPriority(3));

    sorter.AddEdge(3, 0);

    TF_AXIOM(sorter.GetPriority(2) < sorter.GetPriority(3));
    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(1));
    TF_AXIOM(sorter.GetPriority(3) < sorter.GetPriority(0));
}

static void
testClear()
{
    // Ensure that clearing the structure really erases any existing
    // priorities.
    IntVertexSorter sorter;

    sorter.AddEdge(0, 1);
    sorter.AddEdge(0, 2);

    TF_AXIOM(sorter.GetPriority(0) != IntVertexSorter::InvalidPriority);
    TF_AXIOM(sorter.GetPriority(1) != IntVertexSorter::InvalidPriority);
    TF_AXIOM(sorter.GetPriority(2) != IntVertexSorter::InvalidPriority);

    sorter.Clear();

    TF_AXIOM(sorter.GetPriority(0) == IntVertexSorter::InvalidPriority);
    TF_AXIOM(sorter.GetPriority(1) == IntVertexSorter::InvalidPriority);
    TF_AXIOM(sorter.GetPriority(2) == IntVertexSorter::InvalidPriority);
}

static void
testInsertDuplicateEdges()
{
    // Inserting duplicate edges is allowed, but the must be removed
    // and equal number of times.

    IntVertexSorter sorter;

    sorter.AddEdge(0, 1);
    sorter.AddEdge(0, 1);
    sorter.AddEdge(0, 1);

    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(1));

    sorter.RemoveEdge(0, 1);
    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(1));

    sorter.RemoveEdge(0, 1);
    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(1));

    sorter.RemoveEdge(0, 1);
    TF_AXIOM(sorter.GetPriority(0) == IntVertexSorter::InvalidPriority);
    TF_AXIOM(sorter.GetPriority(1) == IntVertexSorter::InvalidPriority);
}

static void
testRemoveInverseEdge()
{
    // Ensure that if we have an edge (a, b), attempting to remove (b, a)
    // does not erase any vertices.

    IntVertexSorter sorter;

    sorter.AddEdge(0, 1);
    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(1));

    sorter.RemoveEdge(1, 0);
    TF_AXIOM(sorter.GetPriority(0) != IntVertexSorter::InvalidPriority);
    TF_AXIOM(sorter.GetPriority(1) != IntVertexSorter::InvalidPriority);
    TF_AXIOM(sorter.GetPriority(0) < sorter.GetPriority(1));
}

static void
testDenseGraph()
{
    // Construct a graph with many more edges than vertices.
    // Connect 8 vertices, each with an edge to 8 other vertices.

    IntVertexSorter sorter;

    static const int N = 9;

    for (int i = 1; i < N; ++i) {
        for (int j = 1; j < N; ++j) {
            sorter.AddEdge(-i, j);
        }
    }

    for (int i = 1; i < N; ++i) {
        for (int j = 1; j < N; ++j) {
            int pi = sorter.GetPriority(-i);
            int pj = sorter.GetPriority(j);

            TF_AXIOM(pi != IntVertexSorter::InvalidPriority);
            TF_AXIOM(pj != IntVertexSorter::InvalidPriority);

            TF_VERIFY(pi < pj,
                      "Vertices (%d, %d) failed: %d < %d",
                      -i, j, pi, pj);
        }
    }
}

int
main(int argc, char *argv[])
{
    testEmptyGraph();
    testSingleEdgeGraph();
    testTree();
    testTwoRoots();
    testAcyclicDiamond();
    testCycle();
    testReorder();
    testRemoveAndReorder();
    testClear();
    testInsertDuplicateEdges();
    testRemoveInverseEdge();
    testDenseGraph();

    return 0;
}
