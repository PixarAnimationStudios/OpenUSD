//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/uncompilationTarget.h"
#include "pxr/pxr.h"

#include "pxr/exec/exec/uncompilationRuleSet.h"
#include "pxr/exec/exec/uncompilationTable.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/collector.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/exec/esf/editReason.h"
#include "pxr/exec/esf/journal.h"

#include <iostream>
#include <set>
#include <tuple>

PXR_NAMESPACE_USING_DIRECTIVE;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (input1)
);

#define ASSERT_EQ(expr, expected)                                              \
    [&] {                                                                      \
        auto&& expr_ = expr;                                                   \
        if (expr_ != expected) {                                               \
            TF_FATAL_ERROR(                                                    \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'",        \
                TfStringify(expected).c_str(),                                 \
                TfStringify(expr_).c_str());                                   \
        }                                                                      \
    }()

PXR_NAMESPACE_OPEN_SCOPE

// For testing only, we support comparison of rule sets by sorting their
// contained rules and removing all duplicates prior to comparison. These
// operators must be defined in the pxr namespace.

static bool operator==(
    const Exec_NodeUncompilationTarget &a,
    const Exec_NodeUncompilationTarget &b)
{
    return a.GetNodeId() == b.GetNodeId();
}

static bool operator==(
    const Exec_InputUncompilationTarget &a,
    const Exec_InputUncompilationTarget &b)
{
    return a.IsValid() && b.IsValid() &&
        *a.GetNodeId() == *b.GetNodeId() &&
        *a.GetInputName() == *b.GetInputName();
}

static bool operator==(
    const Exec_UncompilationRule &a,
    const Exec_UncompilationRule &b)
{
    return std::tie(a.target, a.reasons) == std::tie(b.target, b.reasons);
}

static bool operator<(
    const Exec_NodeUncompilationTarget &a,
    const Exec_NodeUncompilationTarget &b)
{
    return a.GetNodeId() < b.GetNodeId();
}

static bool operator<(
    const Exec_InputUncompilationTarget &a,
    const Exec_InputUncompilationTarget &b)
{
    return a.IsValid() && b.IsValid() &&
        std::tie(*a.GetNodeId(), *a.GetInputName()) <
        std::tie(*b.GetNodeId(), *b.GetInputName());
}

static bool operator<(
    const Exec_UncompilationRule &a,
    const Exec_UncompilationRule &b)
{
    return std::tie(a.target, a.reasons) < std::tie(b.target, b.reasons);
}

static bool operator==(
    const Exec_UncompilationRuleSet &a,
    const Exec_UncompilationRuleSet &b)
{
    std::set<Exec_UncompilationRule> setA{a.begin(), a.end()};
    std::set<Exec_UncompilationRule> setB{b.begin(), b.end()};
    return setA == setB;
}

static bool operator!=(
    const Exec_UncompilationRuleSet &a,
    const Exec_UncompilationRuleSet &b)
{
    return !(a == b);
}

static
std::ostream &operator<<(std::ostream &out, const Exec_UncompilationRuleSet& r)
{
    return out << r.GetDescription();
}

PXR_NAMESPACE_CLOSE_SCOPE

// Test that Exec_UncompilationRuleSet::erase is correct.
static void
TestUncompilationRuleSetErase()
{
    // Initialize a rule set.
    Exec_UncompilationRuleSet ruleSet {
        {
            Exec_NodeUncompilationTarget(0),
            EsfEditReason::ResyncedObject
        },
        {
            Exec_NodeUncompilationTarget(1),
            EsfEditReason::ResyncedObject
        },
        {
            Exec_NodeUncompilationTarget(2),
            EsfEditReason::ResyncedObject
        },
        {
            Exec_InputUncompilationTarget(0, _tokens->input1),
            EsfEditReason::ChangedPropertyList
        },
        {
            Exec_InputUncompilationTarget(1, _tokens->input1),
            EsfEditReason::ChangedPropertyList
        },
        {
            Exec_InputUncompilationTarget(2, _tokens->input1),
            EsfEditReason::ChangedPropertyList
        },
    };

    // Erase elements that have nodeId == 2. (This also ensures we test the
    // corner case of erasing the last element.)
    struct _Visitor {
        bool operator()(const Exec_NodeUncompilationTarget &target) const {
            return target.GetNodeId() == 2;
        }

        bool operator()(const Exec_InputUncompilationTarget &target) const {
            return target.IsValid() && *target.GetNodeId() == 2;
        }
    };

    Exec_UncompilationRuleSet::iterator it = ruleSet.begin();
    while (it != ruleSet.end()) {
        if (std::visit(_Visitor(), it->target)) {
            it = ruleSet.erase(it);
            continue;
        }
        ++it;
    }

    // Verify resulting rule set
    Exec_UncompilationRuleSet expected{
        {
            Exec_NodeUncompilationTarget(0),
            EsfEditReason::ResyncedObject
        },
        {
            Exec_NodeUncompilationTarget(1),
            EsfEditReason::ResyncedObject
        },
        {
            Exec_InputUncompilationTarget(0, _tokens->input1),
            EsfEditReason::ChangedPropertyList
        },
        {
            Exec_InputUncompilationTarget(1, _tokens->input1),
            EsfEditReason::ChangedPropertyList
        },
    };
    ASSERT_EQ(ruleSet, expected);
}

// Test that we add uncompilation rules for each journal entry. If separate
// journals add rules for the same path, those rules get inserted into the
// same rule set.
//
static void
TestUncompilationTableInsertAndFind()
{

    Exec_UncompilationTable table;
    {
        // Node 0 sensitive to resyncs on /A and /B.
        EsfJournal journal;
        journal.Add(SdfPath("/A"), EsfEditReason::ResyncedObject);
        journal.Add(SdfPath("/B"), EsfEditReason::ResyncedObject);
        table.AddRulesForNode(0, journal);
    }
    {
        // Node 1 sensitive to resyncs on /B and /C.
        EsfJournal journal;
        journal.Add(SdfPath("/B"), EsfEditReason::ResyncedObject);
        journal.Add(SdfPath("/C"), EsfEditReason::ResyncedObject);
        table.AddRulesForNode(1, journal);
    }
    {
        // Input "input1" on node 0 sensitive to /A ChangedPropertyList
        EsfJournal journal;
        journal.Add(SdfPath("/A"), EsfEditReason::ChangedPropertyList);
        table.AddRulesForInput(0, _tokens->input1, journal);
    }

    // Verify the contents of the table.
    {
        // Check hook set for /A.
        Exec_UncompilationTable::Entry entryA = table.Find(SdfPath("/A"));
        TF_AXIOM(entryA.path == SdfPath("/A"));
        TF_AXIOM(entryA.ruleSet);
        Exec_UncompilationRuleSet expected{
            {
                Exec_NodeUncompilationTarget(0),
                EsfEditReason::ResyncedObject
            },
            {
                Exec_InputUncompilationTarget(0, _tokens->input1),
                EsfEditReason::ChangedPropertyList
            },
        };
        ASSERT_EQ(*entryA.ruleSet, expected);
    }
    {
        // Check hook set for /B.
        Exec_UncompilationTable::Entry entryB = table.Find(SdfPath("/B"));
        TF_AXIOM(entryB.path == SdfPath("/B"));
        TF_AXIOM(entryB.ruleSet);
        Exec_UncompilationRuleSet expected{
            {
                Exec_NodeUncompilationTarget(0),
                EsfEditReason::ResyncedObject
            },
            {
                Exec_NodeUncompilationTarget(1),
                EsfEditReason::ResyncedObject
            },
        };
        ASSERT_EQ(*entryB.ruleSet, expected);
    }
    {
        // Check hook set for /C.
        Exec_UncompilationTable::Entry entryC = table.Find(SdfPath("/C"));
        TF_AXIOM(entryC.path == SdfPath("/C"));
        TF_AXIOM(entryC.ruleSet);
        Exec_UncompilationRuleSet expected{
            {
                Exec_NodeUncompilationTarget(1),
                EsfEditReason::ResyncedObject
            },
        };
        ASSERT_EQ(*entryC.ruleSet, expected);
    }
    {
        // Check hook set for /D. (It should not exist)
        Exec_UncompilationTable::Entry entryD = table.Find(SdfPath("/D"));
        TF_AXIOM(entryD.path == SdfPath("/D"));
        TF_AXIOM(!entryD.ruleSet);
    }
}

// Test that UpdateForRecursiveResync removes the correct rule sets from
// the uncompilation table, and that unrelated rule sets are not removed.
//
static void
TestUncompilationTableUpdateForRecursiveResync()
{

    const SdfPath parent("/Parent");
    const SdfPath child1("/Parent/Child1");
    const SdfPath child1Attr("/Parent/Child1.attr");
    const SdfPath child2("/Parent/Child2");
    const SdfPath other("/Other");
    const SdfPath otherChild("/Other/Child");

    Exec_UncompilationTable table;
    
    /// Node \p nodeId sensitive to resync on \p path.
    auto insertRules = [&table](const SdfPath &path, VdfId nodeId)
    {
        EsfJournal journal;
        journal.Add(path, EsfEditReason::ResyncedObject);
        table.AddRulesForNode(nodeId, journal);
    };
    insertRules(parent, 0);
    insertRules(child1, 1);
    insertRules(child1Attr, 2);
    insertRules(child2, 3);
    insertRules(other, 4);
    insertRules(otherChild, 5);

    // Handle a recursive resync on /Parent.
    const std::vector<Exec_UncompilationTable::Entry> removedEntries =
        table.UpdateForRecursiveResync(parent);

    // Expected to have removed entries for /Parent, and all descendants.
    auto verifyRemovedEntry = [&removedEntries](
        size_t entryIndex,
        const SdfPath &path,
        VdfId nodeId)
    {
        ASSERT_EQ(removedEntries[entryIndex].path, path);
        Exec_UncompilationRuleSet expected{
            {
                Exec_NodeUncompilationTarget(nodeId),
                EsfEditReason::ResyncedObject
            },
        };
        TF_AXIOM(removedEntries[entryIndex].ruleSet != nullptr);
        ASSERT_EQ(*removedEntries[entryIndex].ruleSet, expected);
    };
    ASSERT_EQ(removedEntries.size(), 4);
    verifyRemovedEntry(0, parent, 0);
    verifyRemovedEntry(1, child1, 1);
    verifyRemovedEntry(2, child1Attr, 2);
    verifyRemovedEntry(3, child2, 3);

    // Searching the table for any of the removed paths should return null
    // rule sets.
    ASSERT_EQ(table.Find(parent).ruleSet, nullptr);
    ASSERT_EQ(table.Find(child1).ruleSet, nullptr);
    ASSERT_EQ(table.Find(child1Attr).ruleSet, nullptr);
    ASSERT_EQ(table.Find(child2).ruleSet, nullptr);

    // Rule sets remain in the table for /Other and /Other/Child.
    Exec_UncompilationTable::Entry entry = table.Find(other);
    Exec_UncompilationRuleSet expectedRuleSetOther{
        {
            Exec_NodeUncompilationTarget(4),
            EsfEditReason::ResyncedObject
        },
    };
    TF_AXIOM(entry);
    ASSERT_EQ(entry.path, other);
    ASSERT_EQ(*entry.ruleSet, expectedRuleSetOther);

    entry = table.Find(otherChild);
    Exec_UncompilationRuleSet expectedRuleSetOtherChild{
        {
            Exec_NodeUncompilationTarget(5),
            EsfEditReason::ResyncedObject
        },
    };
    TF_AXIOM(entry);
    ASSERT_EQ(entry.path, otherChild);
    ASSERT_EQ(*entry.ruleSet, expectedRuleSetOtherChild);
}

// Tests that we can add rules to the uncompilation table concurrently
// from many threads.
//
static void
TestConcurrency()
{

    TRACE_FUNCTION();

    constexpr size_t NUM_PATHS =  5'000;
    constexpr size_t NUM_NODES = 10'000;
    std::cout << "Starting concurrency test ("
        << NUM_PATHS << " paths, "
        << NUM_NODES << " nodes)..." << std::endl;
    
    // Build up a large journal with NUM_PATHS entries, each for ResyncedObject
    // on a unique path.
    std::cout << "Generating journal..." << std::endl;
    EsfJournal journal;
    auto uniquePath = [](int i) {
        return SdfPath(TfStringPrintf("/Prim_%d", i));
    };
    for (int i = 0; i < (int)NUM_PATHS; i++) {
        journal.Add(uniquePath(i), EsfEditReason::ResyncedObject);
    }

    // Simulate the compilation of NUM_NODES VdfNodes. Each task i in
    // [0, NUM_NODES) bills the journal to a node with id = i.
    Exec_UncompilationTable table;
    std::cout << "Starting tasks..." << std::endl;
    {
        TRACE_SCOPE("Adding rules");
        WorkParallelForN(NUM_NODES, [&](size_t nodeIdBegin, size_t nodeIdEnd) {
            for (size_t nodeId = nodeIdBegin; nodeId < nodeIdEnd; ++nodeId) {
                table.AddRulesForNode(nodeId, journal);
            }
        });
    }
    std::cout << "Finished tasks" << std::endl;

    // Verify the final state of the uncompilation table. We should have the
    // same rule set for each unique path.
    std::cout << "Verifying table..." << std::endl;
    Exec_UncompilationRuleSet expectedRuleSet;
    for (size_t i = 0; i < NUM_NODES; ++i) {
        expectedRuleSet.emplace_back(
            Exec_NodeUncompilationTarget(i),
            EsfEditReason::ResyncedObject);
    }
    {
        TRACE_SCOPE("Verifying table");
        WorkParallelForN(NUM_PATHS, [&](size_t pathIdBegin, size_t pathIdEnd) {
            for (size_t pathId = pathIdBegin; pathId < pathIdEnd; ++pathId) {
                const SdfPath path = uniquePath(pathId);
                Exec_UncompilationTable::Entry entry = table.Find(path);
                TF_AXIOM(entry);
                ASSERT_EQ(entry.path, path);
                ASSERT_EQ(*entry.ruleSet, expectedRuleSet);
            }
        });
    }

    std::cout << "Done!" << std::endl;
}

int main()
{
    WorkSetMaximumConcurrencyLimit();

    TestUncompilationTableInsertAndFind();
    TestUncompilationRuleSetErase();
    TestUncompilationTableUpdateForRecursiveResync();

    TraceCollector::GetInstance().SetEnabled(true);
    TestConcurrency();
    TraceReporter::GetGlobalReporter()->Report(std::cout);
}
