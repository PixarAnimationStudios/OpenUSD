#include <UT/UT_Interrupt.h>
#include <UT/UT_ParallelUtil.h>
#include <UT/UT_ErrorManager.h>

#include <iostream>

#include "gusd/error.h"

#include "pxr/base/tf/stringUtils.h"


PXR_NAMESPACE_USING_DIRECTIVE


template <typename FN1, typename FN2>
void _TestBasicErrorFn(UT_ErrorSeverity sev,
                       const FN1& oneArgFn,
                       const FN2& twoArgFn)
{
    {
        UT_ErrorManager::Scope scope;
        oneArgFn("foo");

        TF_AXIOM(scope.getSeverity() == sev);
        TF_AXIOM(sev == UT_ERROR_NONE || GusdGetErrors() == "foo");
    }
    {
        UT_ErrorManager::Scope scope;
        twoArgFn("foo %s", "bar");
        
        TF_AXIOM(scope.getSeverity() == sev);
        TF_AXIOM(sev == UT_ERROR_NONE || GusdGetErrors() == "foo bar");
    }
}


const char*
_ConstructStringNeverReached()
{
    // Should never reach this point. This indicates that GUSD_GENERIC_ERR
    // is failing to elide its message-posting as expected.
    TF_AXIOM(false);

    return "invalid";
}


/// Test basic error posting helpers.
void _TestGusdBasicErrors()
{
    std::cout << "Testing basic error reporting" << std::endl;

    _TestBasicErrorFn(UT_ERROR_ABORT,
                      [](const char* msg)
                      { GUSD_ERR().Msg(msg); },
                      [](const char* fmt, const char* arg)
                      { GUSD_ERR().Msg(fmt, arg); });

    _TestBasicErrorFn(UT_ERROR_WARNING,
                      [](const char* msg)
                      { GUSD_WARN().Msg(msg); },
                      [](const char* fmt, const char* arg)
                      { GUSD_WARN().Msg(fmt, arg); });

    _TestBasicErrorFn(UT_ERROR_MESSAGE,
                      [](const char* msg)
                      { GUSD_MSG().Msg(msg); },
                      [](const char* fmt, const char* arg)
                      { GUSD_MSG().Msg(fmt, arg); });

    for(int i = 0; i < UT_NUM_ERROR_SEVERITIES; ++i) {
        UT_ErrorSeverity sev = static_cast<UT_ErrorSeverity>(i);
        _TestBasicErrorFn(sev,
                          [&](const char* msg)
                          { GUSD_GENERIC_ERR(sev).Msg(msg); },
                          [&](const char* fmt, const char* arg)
                          { GUSD_GENERIC_ERR(sev).Msg(fmt, arg); });
    }

    // GUSD_GENERIC_ERR with UT_ERROR_NONE should not end up
    // invoking any code that builds the error string.
    {
        UT_ErrorManager::Scope scope;
        GUSD_GENERIC_ERR(UT_ERROR_NONE).Msg(_ConstructStringNeverReached());
    }
}


void _TestGusdErrorTransport()
{
    std::cout << "Test GusdErrorTransport" << std::endl;

    // Basic parallel transport test.
    {
        UT_ErrorManager::Scope scope;

        GusdErrorTransport transport;

        auto fn = [&]() {
            UT_ErrorManager::Scope threadScope;

            // XXX: Sleep to trick tbb into thinking this is an expensive
            // task. Otherwise it might run single-threaded.
            sleep(1);
            GusdAutoErrorTransport autoTransport(transport);
            GUSD_ERR().Msg("error");
        };

        UTparallelInvoke(true, fn, fn);

        TF_AXIOM(scope.getErrorManager().getNumErrors() == 2);
        std::string msg = GusdGetErrors();
        TF_AXIOM(TfStringContains(msg, "error"));
    }
}


void _TestGusdTfErrorScope()
{
    std::cout << "Test GusdTfErrorScope" << std::endl;

    // Severity is user-configured. Test each severity level.
    for(int i = UT_ERROR_MESSAGE; i < UT_NUM_ERROR_SEVERITIES; ++i) {
        
        auto sev = static_cast<UT_ErrorSeverity>(i);

        UT_ErrorManager::Scope scope;
        {
            GusdTfErrorScope tfErrScope(sev);
            TF_CODING_ERROR("(coding error)");
            TF_RUNTIME_ERROR("(runtime error)");
        }
        TF_AXIOM(scope.getErrorManager().getNumErrors() == 2);
        TF_AXIOM(scope.getSeverity() == sev);
        TF_AXIOM(TfStringContains(GusdGetErrors(), "(coding error)"));
        TF_AXIOM(TfStringContains(GusdGetErrors(), "(runtime error)"));
    }

    // Setting severity of NONE means errors are ignored.
    {
        UT_ErrorManager::Scope scope;
        {
            GusdTfErrorScope tfErrScope(UT_ERROR_NONE);
            TF_CODING_ERROR("(coding error)");
            TF_RUNTIME_ERROR("(runtime error)");
        }
        TF_AXIOM(scope.getErrorManager().getNumErrors() == 0);
        TF_AXIOM(scope.getSeverity() == UT_ERROR_NONE);
    }

    // Test workaround for errors containing '<>' chars, which normally
    // won't display in node MMB menus due to HTML formatting.
    {
        UT_ErrorManager::Scope scope;
        {
            GusdTfErrorScope tfErrScope;
            TF_CODING_ERROR("<foo>");
        }
        TF_AXIOM(TfStringContains(GusdGetErrors(), "[foo]"));
    }
}


int main(int argc, char* argv[])
{
    static UT_Interrupt boss("testGusdError");
    boss.setEnabled(1,1);
    UTsetInterrupt(&boss);

    _TestGusdBasicErrors();
    _TestGusdErrorTransport();
    _TestGusdTfErrorScope();

    return 0;
}
