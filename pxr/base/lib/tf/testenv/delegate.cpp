//
// Copyright 2017 Pixar
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

#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/regTest.h"

#include <iostream>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class TestDelegate : public TfDiagnosticMgr::Delegate {
    public:
        TestDelegate(const std::string& ident) : _ident(ident) {}
        virtual ~TestDelegate() {}

        virtual void IssueError(const TfError &err) override {
            std::cout << "[" << _ident << "]: Error issued\n";
        }

        virtual void IssueFatalError(const TfCallContext &context,
                                     const std::string &msg) override {
            std::cout << "[" << _ident << "]: Fatal error issued\n";
        }

        virtual void IssueStatus(const TfStatus &status) override {
            std::cout << "[" << _ident << "]: Status issued\n";
        }

        virtual void IssueWarning(const TfWarning &warning) override {
            std::cout << "[" << _ident << "]: Warning issued\n";
        }

    private:
        std::string _ident;
};

// Create an RAII style wrapper using AddDelegate/RemoveDelegate
class TestDelegateWrapper {
    public:
        TestDelegateWrapper(TestDelegate* testDelegate) 
            : _testDelegate(testDelegate) {
            TfDiagnosticMgr::GetInstance().AddDelegate(_testDelegate);
        }

        ~TestDelegateWrapper() {
            TfDiagnosticMgr::GetInstance().RemoveDelegate(_testDelegate);
        }

    private:
        TestDelegate* _testDelegate;
};

static bool
Test_TfDelegateAddRemove() {
    TestDelegate testDelegate("delegate_1");
    TestDelegateWrapper testDelegateWrapper(&testDelegate);
    TF_STATUS("."); 
    TF_WARN(".");
    TF_FATAL_ERROR(".");

    // Add a second delegate
    {
        TestDelegate testDelegate2 ("delegate_2");
        TestDelegateWrapper testDelegateWrapper2(&testDelegate2);
        TF_STATUS("."); 
        TF_WARN(".");
        TF_FATAL_ERROR(".");
    }
    
    // Second delegate is gone bc its out of scope now
    TF_STATUS(".");
    TF_WARN(".");
    TF_FATAL_ERROR(".");

    return true;
}

TF_ADD_REGTEST(TfDelegateAddRemove);

PXR_NAMESPACE_CLOSE_SCOPE
