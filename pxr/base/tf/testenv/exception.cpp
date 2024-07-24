//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
