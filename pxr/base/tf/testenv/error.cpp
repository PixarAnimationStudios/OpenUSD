//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"

#include "pxr/base/arch/functionLite.h"

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
    printf("Thread issuing error\n");
    TF_RUNTIME_ERROR("Cross-thread transfer test error");
    TF_AXIOM(!m.IsClean());
    m.TransportTo(*transport);
    TF_AXIOM(m.IsClean());
}

static bool
Test_TfErrorThreadTransport()
{
    TfErrorTransport transport;
    printf("Creating TfErrorMark\n");
    TfErrorMark m;
    printf("Launching thread\n");
    std::thread t([&transport]() { _ThreadTask(&transport); });
    TF_AXIOM(m.IsClean());
    t.join();
    printf("Thread completed, posting error.\n");
    TF_AXIOM(m.IsClean());
    transport.Post();
    TF_AXIOM(!m.IsClean());
    m.Clear();

    return true;
}

TF_ADD_REGTEST(TfErrorThreadTransport);
