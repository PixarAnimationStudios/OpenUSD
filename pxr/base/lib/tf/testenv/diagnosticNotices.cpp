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
/// \file tf/test/testTfDiagnosticNotices.cpp

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnosticNotice.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/warning.h"
#include "pxr/base/tf/errorMark.h"

#define FILENAME   "diagnosticNotices.cpp"

#include <iostream>
#include <fstream>
#include <string>

using std::string;
PXR_NAMESPACE_USING_DIRECTIVE

enum TfTestDiagnosticCodes { SMALL, MEDIUM, LARGE };
enum UnRegisteredErrorCode { UNREGISTERED };

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(SMALL);
    TF_ADD_ENUM_NAME(MEDIUM);
    TF_ADD_ENUM_NAME(LARGE);

    TF_ADD_ENUM_NAME(UNREGISTERED);
}

class TestDiagnosticListener: public TfWeakBase
{
public:
    TestDiagnosticListener(const char *outFileName):
        _numErrors(0),
        _numWarnings(0),
        _numStatuses(0)
    {
        _outFile.open(outFileName);

        TfWeakPtr<TestDiagnosticListener> me(this);
        TfNotice::Register(me, &TestDiagnosticListener::HandleError);
        TfNotice::Register(me, &TestDiagnosticListener::HandleWarning);
        TfNotice::Register(me, &TestDiagnosticListener::HandleStatus);
        TfNotice::Register(me, &TestDiagnosticListener::HandleFatalError);
    }

    ~TestDiagnosticListener()
    {
        _outFile.close();
    }

    void HandleError(const TfDiagnosticNotice::IssuedError &n)
    {
        _numErrors++;

        const TfError &error = n.GetError();
        _PrintDiagnostic(error.GetErrorCode(), error.GetContext(),
            error.GetCommentary());
    }

    void HandleWarning(const TfDiagnosticNotice::IssuedWarning &n)
    {
        _numWarnings++;

        const TfWarning &warning = n.GetWarning();
        _PrintDiagnostic(warning.GetDiagnosticCode(), warning.GetContext(),
            warning.GetCommentary());
    }

    void HandleStatus(const TfDiagnosticNotice::IssuedStatus &n)
    {
        _numStatuses++;

        const TfStatus &status = n.GetStatus();
        _PrintDiagnostic(status.GetDiagnosticCode(), status.GetContext(),
            status.GetCommentary());
    }

    void HandleFatalError(const TfDiagnosticNotice::IssuedFatalError &n)
    {
        _PrintDiagnostic(TF_DIAGNOSTIC_FATAL_ERROR_TYPE, n.GetContext(),
            n.GetMessage());
    }

    size_t GetNumErrors() const { return _numErrors; }

    size_t GetNumWarnings() const { return _numWarnings; }

    size_t GetNumStatuses() const { return _numStatuses; }

private:
    std::ofstream _outFile;
    size_t _numErrors, _numWarnings, _numStatuses;

    void _PrintDiagnostic(const TfEnum &code, const TfCallContext &context,
        const std::string& msg)
    {
        string codeName = TfEnum::GetDisplayName(code);
        if (codeName.empty())
            codeName = ArchGetDemangled(code.GetType());

        _outFile << TfStringPrintf("%s: in %s at line %zu of %s -- %s",
            codeName.c_str(),
            context.GetFunction(),
            context.GetLine(),
            context.GetFile(),
            msg.c_str()) << std::endl;
    }
};

static bool
Test_TfDiagnosticNotices()
{
    std::cout << "Verifying TfDiagnosticNotice send/receive behavior."
              << std::endl;

    TestDiagnosticListener listener("output.txt");

    TfErrorMark m;
    m.SetMark();
    TF_AXIOM(m.IsClean());

    // TfDiagnosticInfo containing arbitrary info.
    TfDiagnosticInfo info(std::string("String containing arbitrary information."));

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

    // Assert that 12 errors got issued.
    TF_AXIOM(std::distance(m.GetBegin(), m.GetEnd()) == 12);

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

    TF_STATUS(UNREGISTERED, "Status message with an unregistered error code!");

    std::cout << "Number of errors received: " << listener.GetNumErrors()
              << std::endl
              << "Number of warnings received: " << listener.GetNumWarnings()
              << std::endl
              << "Number of status messages received: "
              << listener.GetNumStatuses() << std::endl;

    // No error notices issued, since they were handled by the mark above.
    TF_AXIOM(listener.GetNumErrors() == 0);
    TF_AXIOM(listener.GetNumWarnings() ==  9);
    TF_AXIOM(listener.GetNumStatuses() == 10);

    return true;
}

static bool
Test_TfDiagnosticNotice_Fatal()
{
    std::cout << "Verifying IssuedFatalError notice behavior." << std::endl;

    TestDiagnosticListener listener("output_fatal.txt");

    TF_FATAL_ERROR("Testing notice IssuedFatalError.");

    return true;
}

TF_ADD_REGTEST(TfDiagnosticNotices);
TF_ADD_REGTEST(TfDiagnosticNotice_Fatal);
