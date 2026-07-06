//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/diagnosticTrap.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/arch/functionLite.h"

#include <tbb/concurrent_vector.h>

#include <thread>

#define FILENAME   "error.cpp"

#include <string>
using std::string;
PXR_NAMESPACE_USING_DIRECTIVE

enum TfTestErrorCodes { SMALL, MEDIUM, LARGE };

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(SMALL);
    TF_ADD_ENUM_NAME(MEDIUM);
    TF_ADD_ENUM_NAME(LARGE);
}

enum UnRegisteredErrorCode { UNREGISTERED };

static bool
Test_TfError()
{

    TfErrorMark m;
    size_t lineNum;

    m.SetMark();
    TF_AXIOM(m.IsClean());

    m.SetMark();
    TF_ERROR(SMALL, "small error");
    lineNum = __LINE__ - 1;
    TF_AXIOM(!m.IsClean());

    TfErrorMark::Iterator i = m.GetBegin();
    TF_AXIOM(i == TfDiagnosticMgr::GetInstance().GetErrorBegin());
    TfError e = *i;
    TF_AXIOM(e.GetSourceFileName() == __ARCH_FILE__);
    TF_AXIOM(e.GetSourceLineNumber() == lineNum);
    TF_AXIOM(e.GetCommentary() == "small error");
    TF_AXIOM(e.GetErrorCode() == SMALL);
    TF_AXIOM(e.GetErrorCodeAsString() == "SMALL");
    TF_AXIOM(e.GetInfo<int>() == NULL);
    e.AugmentCommentary("augment");
    TF_AXIOM(e.GetCommentary() == "small error\naugment");
    i = TfDiagnosticMgr::GetInstance().EraseError(i);
    TF_AXIOM(i == TfDiagnosticMgr::GetInstance().GetErrorEnd());

    m.SetMark();
    TF_ERROR(1, MEDIUM, "medium error");
    TF_ERROR(2, LARGE, "large error");

    i = m.GetBegin();
    TF_AXIOM(i == TfDiagnosticMgr::GetInstance().GetErrorBegin());
    e = *i;
    TF_AXIOM(e.GetErrorCode() == MEDIUM);
    TF_AXIOM(*e.GetInfo<int>() == 1);

    ++i;
    TF_AXIOM(i != TfDiagnosticMgr::GetInstance().GetErrorEnd());
    e = *i;
    TF_AXIOM(e.GetErrorCode() == LARGE);
    TF_AXIOM(*e.GetInfo<int>() == 2);

    m.Clear();
    TF_AXIOM(m.IsClean());

    TF_VERIFY(m.IsClean());

    TF_AXIOM(TF_VERIFY(m.IsClean()));

    TF_CODING_ERROR("test error");

    // It should be the case that m is not clean.
    TF_AXIOM(TF_VERIFY(!m.IsClean()));

    // It should not be the case that m is clean.
    TF_AXIOM(!TF_VERIFY(m.IsClean()));

    TF_AXIOM(!TF_VERIFY(m.IsClean(), "With a %s", "message."));

    // Should issue a failed expect error.
    TF_VERIFY(m.IsClean());

    m.Clear();

    // Arbitrary info.
    std::string info("String containing arbitrary information.");

    // Issue a few different variations of errors.
    m.SetMark();

    string errString = "Error!";

    TF_CODING_ERROR("Coding error");
    TF_CODING_ERROR("Coding error %d", 1);
    TF_CODING_ERROR(errString);

    TF_RUNTIME_ERROR("Runtime error");
    TF_RUNTIME_ERROR("Runtime error %d", 1);
    TF_RUNTIME_ERROR(errString);

    TF_ERROR(SMALL, "const char *");
    TF_ERROR(SMALL, "const char *, %s", "...");
    TF_ERROR(SMALL, errString);

    TF_ERROR(info, MEDIUM, "const char *");
    TF_ERROR(info, MEDIUM, "const char *, %s", "...");
    TF_ERROR(info, MEDIUM, errString);

    TF_AXIOM(!m.IsClean());
    m.Clear();

    // Issue a few different warnings.
    string warningString = "Warning!";

    TF_WARN("const char *");
    TF_WARN("const char *, %s", "...");
    TF_WARN(warningString);

    TF_WARN(SMALL, "const char *");
    TF_WARN(SMALL, "const char *, %s", "...");
    TF_WARN(SMALL, warningString);

    TF_WARN(info, MEDIUM, "const char *");
    TF_WARN(info, MEDIUM, "const char *, %s", "...");
    TF_WARN(info, MEDIUM, warningString);

    // Issue a few different status messages.
    string statusString = "Status";

    TF_STATUS("const char *");
    TF_STATUS("const char *, %s", "...");
    TF_STATUS(statusString);

    TF_STATUS(SMALL, "const char *");
    TF_STATUS(SMALL, "const char *, %s", "...");
    TF_STATUS(SMALL, statusString);

    TF_STATUS(info, MEDIUM, "const char *");
    TF_STATUS(info, MEDIUM, "const char *, %s", "...");
    TF_STATUS(info, MEDIUM, statusString);

    return true;
}

TF_ADD_REGTEST(TfError);


static void
_ThreadTask(TfErrorTransport *transport)
{
    TfErrorMark m;
    TF_RUNTIME_ERROR("Cross-thread transfer test error");
    TF_AXIOM(!m.IsClean());
    m.TransportTo(*transport);
    TF_AXIOM(m.IsClean());
}

static bool
Test_TfErrorThreadTransport()
{
    TfErrorTransport transport;
    TfErrorMark m;
    std::thread t([&transport]() { _ThreadTask(&transport); });
    TF_AXIOM(m.IsClean());
    t.join();
    TF_AXIOM(m.IsClean());
    transport.Post();
    TF_AXIOM(!m.IsClean());
    m.Clear();
    return true;
}

TF_ADD_REGTEST(TfErrorThreadTransport);


static bool
Test_TfDiagnosticTrap()
{
    // Run all tests in a separate thread to escape the test framework's
    // top-level TfErrorMark, which would otherwise prevent errors from being
    // reported and thus trapped.
    bool result = false;
    std::thread t([&result]() {

        // Basic capture of warnings, statuses, and errors.
        {
            TfDiagnosticTrap trap;
            TF_AXIOM(!trap.HasErrors());
            TF_AXIOM(!trap.HasWarnings());
            TF_AXIOM(!trap.HasStatuses());

            TF_WARN("test warning");
            TF_AXIOM(trap.HasWarnings());
            TF_AXIOM(trap.GetWarnings().size() == 1);
            TF_AXIOM(trap.GetWarnings()[0].GetCommentary() == "test warning");
            TF_AXIOM(!trap.HasErrors());
            TF_AXIOM(!trap.HasStatuses());

            TF_STATUS("test status");
            TF_AXIOM(trap.HasStatuses());
            TF_AXIOM(trap.GetStatuses().size() == 1);
            TF_AXIOM(trap.GetStatuses()[0].GetCommentary() == "test status");

            TF_RUNTIME_ERROR("test error");
            TF_AXIOM(trap.HasErrors());
            TF_AXIOM(trap.GetErrors().size() == 1);
            TF_AXIOM(trap.GetErrors()[0].GetCommentary() == "test error");

            trap.Clear();
            TF_AXIOM(!trap.HasWarnings());
            TF_AXIOM(!trap.HasStatuses());
            TF_AXIOM(!trap.HasErrors());
        }

        // Selective clearing.
        {
            TfDiagnosticTrap trap;
            TF_WARN("warning");
            TF_STATUS("status");
            TF_RUNTIME_ERROR("error");
            TF_AXIOM(trap.HasWarnings());
            TF_AXIOM(trap.HasStatuses());
            TF_AXIOM(trap.HasErrors());

            trap.ClearWarnings();
            TF_AXIOM(!trap.HasWarnings());
            TF_AXIOM(trap.HasStatuses());
            TF_AXIOM(trap.HasErrors());

            trap.ClearStatuses();
            TF_AXIOM(!trap.HasStatuses());
            TF_AXIOM(trap.HasErrors());

            trap.ClearErrors();
            TF_AXIOM(!trap.HasErrors());
        }

        // Cleared diagnostics are not re-posted; uncleared ones are.
        {
            TfDiagnosticTrap outer;
            {
                TfDiagnosticTrap inner;
                TF_WARN("should be cleared");
                inner.ClearWarnings();
                TF_WARN("should be reposted");
                // inner destructs here, re-posts "should be reposted"
            }
            TF_AXIOM(outer.HasWarnings());
            TF_AXIOM(outer.GetWarnings().size() == 1);
            TF_AXIOM(outer.GetWarnings()[0].GetCommentary() ==
                     "should be reposted");
            outer.Clear();
        }

        // Nesting - inner trap gates, outer sees only what escapes.
        {
            TfDiagnosticTrap outer;
            {
                TfDiagnosticTrap inner;
                TF_WARN("inner warning");
                TF_STATUS("inner status");
                TF_AXIOM(inner.HasWarnings());
                TF_AXIOM(inner.HasStatuses());
                TF_AXIOM(!outer.HasWarnings());
                TF_AXIOM(!outer.HasStatuses());
                inner.Clear();
                // inner destructs here, nothing re-posted
            }
            TF_AXIOM(!outer.HasWarnings());
            TF_AXIOM(!outer.HasStatuses());
            outer.Clear();
        }

        // Ordering - interleaved diagnostics re-post in original order.
        {
            TfDiagnosticTrap outer;
            {
                TfDiagnosticTrap inner;
                TF_WARN("first");
                TF_STATUS("second");
                TF_WARN("third");
            }
            std::vector<std::string> order;
            outer.ForEach(TfOverloads {
                [&order](TfWarning const &w) {
                    order.push_back("W:" + w.GetCommentary());
                },
                [&order](TfStatus const &s) {
                    order.push_back("S:" + s.GetCommentary());
                },
                [&order](TfError const &e) {
                    order.push_back("E:" + e.GetCommentary());
                }
            });
            TF_AXIOM(order.size() == 3);
            TF_AXIOM(order[0] == "W:first");
            TF_AXIOM(order[1] == "S:second");
            TF_AXIOM(order[2] == "W:third");
            outer.Clear();
        }

        // Dismiss() explicitly re-posts and deactivates.
        {
            TfDiagnosticTrap outer;
            {
                TfDiagnosticTrap inner;
                TF_WARN("warning");
                inner.Dismiss();
                TF_AXIOM(!inner.HasWarnings());
                // destructor is a no-op
            }
            TF_AXIOM(outer.HasWarnings());
            TF_AXIOM(outer.GetWarnings()[0].GetCommentary() == "warning");
            outer.Clear();
        }

        // TfErrorMark interaction - errors with active mark are not trapped.
        {
            TfDiagnosticTrap trap;
            TfErrorMark mark;
            TF_RUNTIME_ERROR("error under mark");
            TF_AXIOM(!mark.IsClean());
            TF_AXIOM(!trap.HasErrors());
            mark.Clear();
        }

        // TfErrorMark interaction - trapped errors reposted under an active
        // mark are caught by the mark.
        {
            TfDiagnosticTrap trap;
            TF_RUNTIME_ERROR("trapped error");
            TfErrorMark mark;
            TF_AXIOM(mark.IsClean());
            TF_AXIOM(trap.HasErrors());
            trap.Dismiss();
            TF_AXIOM(!mark.IsClean());
            TF_AXIOM(!trap.HasErrors());
            mark.Clear();
        }

        // ForEach with type-specific callable -- only matching types visited.
        {
            TfDiagnosticTrap trap;
            TF_WARN("warning");
            TF_STATUS("status");
            TF_RUNTIME_ERROR("error");

            std::vector<std::string> visited;
            trap.ForEach(TfOverloads {
                [&visited](TfWarning const &w) {
                    visited.push_back("W:" + w.GetCommentary());
                },
                [&visited](TfStatus const &s) {
                    visited.push_back("S:" + s.GetCommentary());
                }
                // errors not handled -- silently skipped
            });
            TF_AXIOM(visited.size() == 2);
            TF_AXIOM(visited[0] == "W:warning");
            TF_AXIOM(visited[1] == "S:status");

            // ForEach with TfDiagnosticBase -- all types visited.
            std::vector<std::string> all;
            trap.ForEach([&all](TfDiagnosticBase const &d) {
                all.push_back(d.GetCommentary());
            });
            TF_AXIOM(all.size() == 3);
            TF_AXIOM(all[0] == "warning");
            TF_AXIOM(all[1] == "status");
            TF_AXIOM(all[2] == "error");

            trap.Clear();
        }

        // EraseMatching with type-specific predicate -- only matching types
        // erased.
        {
            TfDiagnosticTrap trap;
            TF_WARN("keep this warning");
            TF_WARN("erase this warning");
            TF_STATUS("keep this status");

            TF_AXIOM(trap.EraseMatching([](TfWarning const &w) {
                return TfStringContains(w.GetCommentary(), "erase");
            }) == 1);

            TF_AXIOM(trap.HasWarnings());
            TF_AXIOM(trap.GetWarnings().size() == 1);
            TF_AXIOM(trap.GetWarnings()[0].GetCommentary() ==
                     "keep this warning");
            TF_AXIOM(trap.HasStatuses());
            TF_AXIOM(trap.GetStatuses()[0].GetCommentary() ==
                     "keep this status");
            trap.Clear();
        }

        // EraseMatching with TfDiagnosticBase -- matches across all types.
        {
            TfDiagnosticTrap trap;
            TF_WARN("erase me");
            TF_STATUS("erase me");
            TF_WARN("keep me");

            TF_AXIOM(trap.EraseMatching([](TfDiagnosticBase const &d) {
                return d.GetCommentary() == "erase me";
            }) == 2);

            TF_AXIOM(trap.HasWarnings());
            TF_AXIOM(trap.GetWarnings().size() == 1);
            TF_AXIOM(trap.GetWarnings()[0].GetCommentary() == "keep me");
            TF_AXIOM(!trap.HasStatuses());
            trap.Clear();
        }

        // EraseMatching preserves order of remaining diagnostics.
        {
            TfDiagnosticTrap outer;
            {
                TfDiagnosticTrap inner;
                TF_WARN("first");
                TF_STATUS("second");
                TF_WARN("third");
                TF_STATUS("fourth");

                inner.EraseMatching([](TfWarning const &w) {
                    return w.GetCommentary() == "first";
                });

                // "first" erased, rest re-post to outer in order
            }

            std::vector<std::pair<char, std::string>> order;
            outer.ForEach(TfOverloads {
                [&order](TfWarning const &w) {
                    order.push_back({'W', w.GetCommentary()});
                },
                [&order](TfStatus const &s) {
                    order.push_back({'S', s.GetCommentary()});
                }
            });
            TF_AXIOM(order.size() == 3);
            TF_AXIOM((order[0] == std::pair<char,std::string>{'S', "second"}));
            TF_AXIOM((order[1] == std::pair<char,std::string>{'W', "third"}));
            TF_AXIOM((order[2] == std::pair<char,std::string>{'S', "fourth"}));
            outer.Clear();
        }

        // EraseMatching safe during ForEach iteration.
        {
            TfDiagnosticTrap trap;
            TF_WARN("keep");
            TF_WARN("erase");
            TF_WARN("keep");

            trap.ForEach([&trap](TfWarning const &w) {
                if (w.GetCommentary() == "erase") {
                    trap.EraseMatching([](TfWarning const &w) {
                        return w.GetCommentary() == "erase";
                    });
                }
            });

            TF_AXIOM(trap.GetWarnings().size() == 2);
            TF_AXIOM(trap.GetWarnings()[0].GetCommentary() == "keep");
            TF_AXIOM(trap.GetWarnings()[1].GetCommentary() == "keep");
            trap.Clear();
        }
        
        // HasAnyMatching -- type-specific predicate.
        {
            TfDiagnosticTrap trap;
            TF_WARN("deprecated feature");
            TF_WARN("unrelated warning");
            TF_STATUS("a status");

            TF_AXIOM(trap.HasAnyMatching([](TfWarning const &w) {
                return TfStringContains(w.GetCommentary(), "deprecated");
            }));
            TF_AXIOM(!trap.HasAnyMatching([](TfWarning const &w) {
                return TfStringContains(w.GetCommentary(), "nonexistent");
            }));
            // Predicate not invocable with TfStatus -- considered not matching.
            TF_AXIOM(!trap.HasAnyMatching([](TfError const &) {
                return true;
            }));
            trap.Clear();
        }

        // HasAnyMatching -- TfDiagnosticBase predicate matches all types.
        {
            TfDiagnosticTrap trap;
            TF_WARN("warning");
            TF_STATUS("status");

            TF_AXIOM(trap.HasAnyMatching([](TfDiagnosticBase const &d) {
                return d.GetCommentary() == "status";
            }));
            TF_AXIOM(!trap.HasAnyMatching([](TfDiagnosticBase const &d) {
                return d.GetCommentary() == "nonexistent";
            }));
            trap.Clear();
        }

        // HasAnyMatching -- empty trap.
        {
            TfDiagnosticTrap trap;
            TF_AXIOM(!trap.HasAnyMatching([](TfDiagnosticBase const &) {
                return true;
            }));
        }

        // HasAllMatching -- all match, some don't.
        {
            TfDiagnosticTrap trap;
            TF_WARN("deprecated a");
            TF_WARN("deprecated b");

            TF_AXIOM(trap.HasAllMatching([](TfWarning const &w) {
                return TfStringContains(w.GetCommentary(), "deprecated");
            }));

            TF_WARN("unrelated");
            TF_AXIOM(!trap.HasAllMatching([](TfWarning const &w) {
                return TfStringContains(w.GetCommentary(), "deprecated");
            }));
            trap.Clear();
        }

        // HasAllMatching -- vacuous truth: empty trap and no invocable type.
        {
            TfDiagnosticTrap trap;
            // Empty trap.
            TF_AXIOM(trap.HasAllMatching([](TfDiagnosticBase const &) {
                return true;
            }));

            TF_WARN("warning");
            TF_STATUS("status");
            // No errors present -- predicate not invocable with any captured
            // type.
            TF_AXIOM(trap.HasAllMatching([](TfError const &) {
                return false;
            }));
            trap.Clear();
        }

        // CountMatching -- type-specific and base predicate.
        {
            TfDiagnosticTrap trap;
            TF_WARN("deprecated a");
            TF_WARN("deprecated b");
            TF_WARN("unrelated");
            TF_STATUS("deprecated status");

            // Type-specific -- only counts warnings.
            TF_AXIOM(trap.CountMatching([](TfWarning const &w) {
                return TfStringContains(w.GetCommentary(), "deprecated");
            }) == 2);

            // Base predicate -- counts across all types.
            TF_AXIOM(trap.CountMatching([](TfDiagnosticBase const &d) {
                return TfStringContains(d.GetCommentary(), "deprecated");
            }) == 3);

            // No invocable type -- counts nothing.
            TF_AXIOM(trap.CountMatching([](TfError const &) {
                return true;
            }) == 0);

            trap.Clear();
        }

        // EraseMatching -- sanity check that it still works correctly
        // after iterator-based reimplementation.
        {
            TfDiagnosticTrap trap;
            TF_WARN("keep");
            TF_STATUS("erase");
            TF_WARN("keep");

            trap.EraseMatching([](TfStatus const &) { return true; });

            TF_AXIOM(!trap.HasStatuses());
            TF_AXIOM(trap.GetWarnings().size() == 2);
            TF_AXIOM(trap.GetWarnings()[0].GetCommentary() == "keep");
            TF_AXIOM(trap.GetWarnings()[1].GetCommentary() == "keep");
            trap.Clear();
        }
        
        result = true;
    });
    t.join();
    return result;
}

TF_ADD_REGTEST(TfDiagnosticTrap);

static bool
Test_TfDiagnosticTransport()
{
    bool result = false;
    std::thread t([&result]() {

        // Basic transport - trap is left clean and active after Transport().
        {
            TfDiagnosticTrap outer;
            {
                TfDiagnosticTrap inner;
                TF_WARN("warning");
                TF_STATUS("status");

                TfDiagnosticTransport transport = inner.Transport();
                TF_AXIOM(inner.IsClean());
                TF_AXIOM(!transport.IsEmpty());

                // Trap still active - further diagnostics are captured.
                TF_WARN("after transport");
                TF_AXIOM(inner.HasWarnings());
                inner.Clear();

                // Post lands in outer.
                transport.Post();
                TF_AXIOM(transport.IsEmpty());
            }
            TF_AXIOM(outer.HasWarnings());
            TF_AXIOM(outer.HasStatuses());
            TF_AXIOM(outer.GetWarnings().size() == 1);
            TF_AXIOM(outer.GetWarnings()[0].GetCommentary() == "warning");
            TF_AXIOM(outer.GetStatuses()[0].GetCommentary() == "status");
            outer.Clear();
        }

        // Empty transport - Post() is a safe no-op.
        {
            TfDiagnosticTrap outer;
            TfDiagnosticTransport transport;
            TF_AXIOM(transport.IsEmpty());
            transport.Post();
            TF_AXIOM(!outer.HasWarnings());
            TF_AXIOM(!outer.HasStatuses());
            outer.Clear();
        }

        // Cross-thread ordering - interleaved order is preserved across the
        // transport boundary.
        {
            tbb::concurrent_vector<TfDiagnosticTransport> transports;

            std::thread child([&transports]() {
                TfDiagnosticTrap trap;
                TF_WARN("first");
                TF_STATUS("second");
                TF_WARN("third");
                if (!trap.IsClean()) {
                    transports.push_back(trap.Transport());
                }
            });
            child.join();

            TfDiagnosticTrap outer;
            for (auto &transport : transports) {
                transport.Post();
            }

            std::vector<std::string> order;
            outer.ForEach(TfOverloads {
                [&order](TfWarning const &w) {
                    order.push_back("W:" + w.GetCommentary());
                },
                [&order](TfStatus const &s) {
                    order.push_back("S:" + s.GetCommentary());
                },
                [&order](TfError const &e) {
                    order.push_back("E:" + e.GetCommentary());
                }
            });
            TF_AXIOM(order.size() == 3);
            TF_AXIOM(order[0] == "W:first");
            TF_AXIOM(order[1] == "S:second");
            TF_AXIOM(order[2] == "W:third");
            outer.Clear();
        }

        // Multiple child threads - each accumulates independently, parent posts
        // all transports after joining.
        {
            tbb::concurrent_vector<TfDiagnosticTransport> transports;

            auto childTask = [&transports](std::string const &name) {
                TfDiagnosticTrap trap;
                TF_WARN("%s", name.c_str());
                if (!trap.IsClean()) {
                    transports.push_back(trap.Transport());
                }
            };

            std::thread child1([&]() { childTask("child1"); });
            std::thread child2([&]() { childTask("child2"); });
            std::thread child3([&]() { childTask("child3"); });
            child1.join();
            child2.join();
            child3.join();

            TfDiagnosticTrap outer;
            for (auto &transport : transports) {
                transport.Post();
            }

            TF_AXIOM(outer.HasWarnings());
            TF_AXIOM(outer.GetWarnings().size() == 3);
            outer.Clear();
        }

        // Nested traps in child - inner clears, outer transports remainder.
        {
            tbb::concurrent_vector<TfDiagnosticTransport> transports;

            std::thread child([&transports]() {
                TfDiagnosticTrap outer;
                {
                    TfDiagnosticTrap inner;
                    TF_WARN("should be cleared");
                    inner.ClearWarnings();
                    TF_WARN("should survive");
                }
                // inner re-posted "should survive" to outer
                if (!outer.IsClean()) {
                    transports.push_back(outer.Transport());
                }
            });
            child.join();

            TfDiagnosticTrap outer;
            for (auto &transport : transports) {
                transport.Post();
            }
            TF_AXIOM(outer.HasWarnings());
            TF_AXIOM(outer.GetWarnings().size() == 1);
            TF_AXIOM(outer.GetWarnings()[0].GetCommentary() ==
                     "should survive");
            outer.Clear();
        }


        result = true;
    });
    t.join();

    return result;
}

TF_ADD_REGTEST(TfDiagnosticTransport);

// Test hooks -- defined in diagnosticMgr.cpp, not in any public header.
PXR_NAMESPACE_OPEN_SCOPE

TF_API std::vector<std::string> Tf_GetPendingErrorsLogTextForTesting();
TF_API std::vector<std::string> Tf_GetTrappedDiagnosticsLogTextForTesting();

TF_API std::vector<std::string>
Tf_GetTransportLogTextForTesting(TfErrorTransport const &);
TF_API std::vector<std::string>
Tf_GetTransportLogTextForTesting(TfDiagnosticTransport const &);

PXR_NAMESPACE_CLOSE_SCOPE

static bool
Test_TfErrorAndDiagnosticTransportLogText()
{
    // All cases run on a dedicated thread to avoid interference from the test
    // framework's top-level TfErrorMark and to get a clean thread-local state.
    bool result = false;
    std::thread t([&result]() {

        // Helper: return true if any entry in 'log' contains 'substr'.
        auto logContains = [](const std::vector<std::string> &log,
                              const std::string &substr) {
            for (const auto &entry : log) {
                if (TfStringContains(entry, substr)) {
                    return true;
                }
            }
            return false;
        };

        // Helpers for each log text.
        auto hasErrLog = [&logContains](std::string const &substr) {
            return logContains(
                Tf_GetPendingErrorsLogTextForTesting(), substr);
        };

        auto hasTrapLog = [&logContains](std::string const &substr) {
            return logContains(
                Tf_GetTrappedDiagnosticsLogTextForTesting(), substr);
        };

        auto hasTransportLog = [&logContains](auto const &transport,
                                              std::string const &substr) {
            return logContains(
                Tf_GetTransportLogTextForTesting(transport), substr);
        };

        ////////////////////////////////////////////////////////////////
        // TfErrorMark::Transport()

        // Transport() immediately rebuilds the primary pending-errors log,
        // removing the transported entries.  They live in a separate transient
        // Arch key for the lifetime of the TfErrorTransport.
        {
            TfErrorMark m;
            TF_RUNTIME_ERROR("flight error");
            TF_AXIOM(hasErrLog("flight error"));

            TfErrorTransport transport = m.Transport();
            TF_AXIOM(m.IsClean());
            // Immediate rebuild: transported entry no longer in primary log.
            TF_AXIOM(!hasErrLog("flight error"));

            // Transported entry should be in its own transient log.
            TF_AXIOM(hasTransportLog(transport, "flight error"));

            // Post() re-adds "flight error" to this thread's error list and
            // primary log; the transient Arch key is removed.
            transport.Post();
            TF_AXIOM(hasErrLog("flight error"));
            TF_AXIOM(!hasTransportLog(transport, "flight error"));

            // Clear() should remove the error entriely.
            m.Clear();
            TF_AXIOM(!hasErrLog("flight error"));
            TF_AXIOM(!hasTransportLog(transport, "flight error"));
        }

        // Dropping a transport without Post() just removes its transient Arch
        // key; the primary log was already rebuilt at Transport() time.
        {
            TfErrorMark m;
            TF_RUNTIME_ERROR("dropped error");
            TF_AXIOM(hasErrLog("dropped error"));
            {
                TfErrorTransport transport = m.Transport();
                TF_AXIOM(m.IsClean());
                // Primary log already clean.
                TF_AXIOM(!hasErrLog("dropped error"));
                // transport destroyed here -- transient Arch key removed.
            }
            TF_AXIOM(!hasErrLog("dropped error"));
            m.Clear();
        }

        // Multiple concurrent transports are independent: each removes its
        // errors from the primary log immediately and owns a unique Arch key.
        {
            TfErrorMark m;
            TF_RUNTIME_ERROR("multi error");

            TfErrorTransport t1 = m.Transport();
            // "multi error" immediately removed from primary log.
            TF_AXIOM(!hasErrLog("multi error"));
            TF_AXIOM(hasTransportLog(t1, "multi error"));

            TF_RUNTIME_ERROR("second error");
            TfErrorTransport t2 = m.Transport();
            // "second error" immediately removed.
            TF_AXIOM(!hasErrLog("second error"));
            TF_AXIOM(hasTransportLog(t2, "second error"));
            TF_AXIOM(!hasTransportLog(t1, "second error"));
            TF_AXIOM(m.IsClean());

            // Drop t1 -- its transient key removed; t2's key and primary log
            // state are unaffected.
            t1 = TfErrorTransport{};
            TF_AXIOM(!hasTransportLog(t1, "multi error"));
            TF_AXIOM(!hasErrLog("multi error"));
            TF_AXIOM(hasTransportLog(t2, "second error"));

            // Drop t2 -- its transient key removed; primary log still clean.
            t2 = TfErrorTransport{};
            TF_AXIOM(!hasErrLog("second error"));
            TF_AXIOM(!hasTransportLog(t2, "second error"));
            m.Clear();
        }

        // Transport() cleans the primary log even when the mark is later
        // destroyed before the transport is posted or dropped.
        {
            TfErrorTransport transport;
            {
                TfErrorMark m;
                TF_RUNTIME_ERROR("quiescent error");
                transport = m.Transport();
                // Primary log already clean at Transport() time.
                TF_AXIOM(!hasErrLog("quiescent error"));
                TF_AXIOM(hasTransportLog(transport, "quiescent error"));
            }
            // m destroyed; transport still in flight -- primary log clean.
            TF_AXIOM(!hasErrLog("quiescent error"));
            TF_AXIOM(hasTransportLog(transport, "quiescent error"));

            // Dropping the transport removes the transient Arch key.
            transport = TfErrorTransport{};
            TF_AXIOM(!hasErrLog("quiescent error"));
            TF_AXIOM(!hasTransportLog(transport, "quiescent error"));
        }

        ////////////////////////////////////////////////////////////////
        // TfDiagnosticTrap::Transport()

        // Transport() immediately rebuilds the primary trapped-diagnostics log,
        // removing the transported entries.
        {
            TfDiagnosticTrap trap;
            TF_WARN("trap flight warning");
            TF_AXIOM(hasTrapLog("trap flight warning"));

            TfDiagnosticTransport transport = trap.Transport();
            TF_AXIOM(trap.IsClean());
            // Immediate rebuild: transported entry no longer in primary log.
            TF_AXIOM(!hasTrapLog("trap flight warning"));
            TF_AXIOM(hasTransportLog(transport, "trap flight warning"));

            // Drop the transport -- transient Arch key removed.
            transport = TfDiagnosticTransport{};
            TF_AXIOM(!hasTrapLog("trap flight warning"));
            TF_AXIOM(!hasTransportLog(transport, "trap flight warning"));
            trap.Clear();
        }

        // _PopTrap always rebuilds; after Transport() the primary log is
        // already clean, so new diagnostics issued after Transport() and before
        // the trap destructs are correctly removed by _PopTrap.
        {
            TfDiagnosticTransport transport;
            {
                TfDiagnosticTrap trap;
                TF_WARN("d1");
                TF_AXIOM(hasTrapLog("d1"));

                // Transport d1 -- primary log immediately rebuilt.
                transport = trap.Transport();
                TF_AXIOM(trap.IsClean());
                TF_AXIOM(!hasTrapLog("d1"));
                TF_AXIOM(hasTransportLog(transport, "d1"));

                // New diagnostic while transport is in flight.
                TF_WARN("d2");
                TF_AXIOM(hasTrapLog("d2"));
                TF_AXIOM(!hasTransportLog(transport, "d2"));
                // d1 is in the transient key, not the primary log.
                TF_AXIOM(!hasTrapLog("d1"));
                TF_AXIOM(hasTransportLog(transport, "d1"));

                // trap destructs: _PopTrap rebuilds from trap._logStart,
                // removing d2 (which is dispatched to delegates, no outer trap).
            }
            // Primary log clean; d1 is in transient key.
            TF_AXIOM(!hasTrapLog("d1"));
            TF_AXIOM(!hasTrapLog("d2"));
            TF_AXIOM(hasTransportLog(transport, "d1"));

            // Drop the transport -- transient key removed.
            transport = TfDiagnosticTransport{};
            TF_AXIOM(!hasTrapLog("d1"));
            TF_AXIOM(!hasTransportLog(transport, "d1"));
        }

        // _OnContentsChanged always rebuilds; after Transport() the primary log
        // is already clean, so clearing a trap while a transport is in flight
        // correctly removes any subsequent entries without disturbing the
        // transient key.
        {
            TfDiagnosticTransport transport;
            {
                TfDiagnosticTrap trap;
                TF_WARN("ct1");
                TF_AXIOM(hasTrapLog("ct1"));

                // Transport ct1 -- primary log immediately rebuilt.
                transport = trap.Transport();
                TF_AXIOM(trap.IsClean());
                TF_AXIOM(!hasTrapLog("ct1"));
                TF_AXIOM(hasTransportLog(transport, "ct1"));

                // Issue a second warning, then clear it.
                // _OnContentsChanged immediately rebuilds the primary log.
                TF_WARN("ct2");
                TF_AXIOM(hasTrapLog("ct2"));
                trap.Clear();
                TF_AXIOM(trap.IsClean());
                TF_AXIOM(!hasTrapLog("ct2"));
                // ct1 is in the transient key, not the primary log.
                TF_AXIOM(!hasTrapLog("ct1"));
                TF_AXIOM(hasTransportLog(transport, "ct1"));

                // trap destructs cleanly (container empty, nothing to re-post).
            }
            // Transport still in flight; transient key still registered.
            TF_AXIOM(!hasTrapLog("ct1"));
            TF_AXIOM(hasTransportLog(transport, "ct1"));
                
            // Drop the transport -- transient Arch key removed.
            transport = TfDiagnosticTransport{};
            TF_AXIOM(!hasTrapLog("ct1"));
            TF_AXIOM(!hasTransportLog(transport, "ct1"));
        }

        // Cross-thread drop: dropping a TfDiagnosticTransport on a different
        // thread removes only its transient Arch key.  The source thread's
        // primary log was already rebuilt at Transport() time and is unaffected.
        {
            TfDiagnosticTrap trap;
            TF_WARN("cross w1");
            TF_AXIOM(hasTrapLog("cross w1"));

            TfDiagnosticTransport transport = trap.Transport();
            TF_AXIOM(trap.IsClean());
            // Primary log rebuilt immediately.
            TF_AXIOM(!hasTrapLog("cross w1"));
            TF_AXIOM(hasTransportLog(transport, "cross w1"));

            // Drop on a child thread -- removes transient Arch key cross-thread.
            std::thread child([t = std::move(transport)]() {});
            child.join();

            transport = TfDiagnosticTransport{};
            
            // Primary log is unchanged (was already clean).
            TF_AXIOM(!hasTrapLog("cross w1"));
            TF_AXIOM(!hasTransportLog(transport, "cross w1"));

            // New diagnostic appended correctly -- no stale state to correct.
            TF_WARN("cross w2");
            TF_AXIOM(trap.HasWarnings());
            TF_AXIOM(trap.GetWarnings().size() == 1);
            TF_AXIOM(trap.GetWarnings()[0].GetCommentary() == "cross w2");
            TF_AXIOM(hasTrapLog("cross w2"));
            trap.Clear();
        }

        // Nested traps: Transport() of inner entries immediately rebuilds the
        // primary log, preserving outer entries and correctly re-computing
        // _logStart for all traps on the stack.
        {
            TfDiagnosticTrap outer;
            TF_WARN("outer w1");
            TF_AXIOM(hasTrapLog("outer w1"));
            {
                TfDiagnosticTrap inner;
                TF_WARN("inner w1");
                TF_AXIOM(hasTrapLog("inner w1"));

                TfDiagnosticTransport transport = inner.Transport();
                TF_AXIOM(inner.IsClean());
                // Primary log rebuilt: outer w1 present, inner w1 removed.
                TF_AXIOM(hasTrapLog("outer w1"));
                TF_AXIOM(!hasTrapLog("inner w1"));
                TF_AXIOM(!hasTransportLog(transport, "outer w1"));
                TF_AXIOM(hasTransportLog(transport, "inner w1"));

                // Drop on child thread -- removes transient key only.
                std::thread child([t = std::move(transport)]() {});
                child.join();

                transport = TfDiagnosticTransport{};

                TF_AXIOM(hasTrapLog("outer w1"));
                TF_AXIOM(!hasTrapLog("inner w1"));
                TF_AXIOM(!hasTransportLog(transport, "outer w1"));
                TF_AXIOM(!hasTransportLog(transport, "inner w1"));

                // Post into inner -- _logStart was already resynced at
                // Transport() time; append is correct.
                TF_WARN("inner w2");
                TF_AXIOM(inner.HasWarnings());
                TF_AXIOM(inner.GetWarnings().size() == 1);
                TF_AXIOM(inner.GetWarnings()[0].GetCommentary() == "inner w2");
                TF_AXIOM(hasTrapLog("outer w1"));
                TF_AXIOM(hasTrapLog("inner w2"));
                inner.Clear();
            }
            // inner re-posts nothing (was cleared).
            TF_AXIOM(outer.HasWarnings());
            TF_AXIOM(outer.GetWarnings().size() == 1);
            TF_AXIOM(outer.GetWarnings()[0].GetCommentary() == "outer w1");
            outer.Clear();
        }

        result = true;
    });
    t.join();
    return result;
}

TF_ADD_REGTEST(TfErrorAndDiagnosticTransportLogText);

