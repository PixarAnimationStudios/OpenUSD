//
// Copyright 2021 Pixar
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
#include "pxr/base/tf/exception.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/regTest.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

class Tf_TestException : public TfBaseException
{
public:
    using TfBaseException::TfBaseException;
    virtual ~Tf_TestException();
};

Tf_TestException::~Tf_TestException()
{
}

static bool
Test_TfException()
{
    try {
        TF_THROW(Tf_TestException, "test exception 1");
    }
    catch (TfBaseException const &exc) {
        TF_AXIOM(std::string(exc.what()) == std::string("test exception 1"));
        TF_AXIOM(exc.GetThrowContext());
    }
    catch (...) {
        TF_FATAL_ERROR("Expected exception was not thrown");
    }

    try {
        TF_THROW(Tf_TestException, TfSkipCallerFrames(2), "test exception 2");
    }
    catch (TfBaseException const &exc) {
        TF_AXIOM(std::string(exc.what()) == std::string("test exception 2"));
        TF_AXIOM(exc.GetThrowContext());
    }
    catch (...) {
        TF_FATAL_ERROR("Expected exception was not thrown");
    }

    return true;
}

TF_ADD_REGTEST(TfException);
