//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_WRAP_CONTEXT_H
#define PXR_BASE_TF_PY_WRAP_CONTEXT_H

#include "pxr/pxr.h"

#include "pxr/base/tf/singleton.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// This is used internally by the Tf python wrapping infrastructure.

class Tf_PyWrapContextManager {
    Tf_PyWrapContextManager(const Tf_PyWrapContextManager&) = delete;
    Tf_PyWrapContextManager&
    operator=(const Tf_PyWrapContextManager&) = delete;

  public:

    typedef Tf_PyWrapContextManager This;
    
    static This &GetInstance() {
        return TfSingleton<This>::GetInstance();
    }

    std::string GetCurrentContext() const {
        return _contextStack.empty() ? std::string() : _contextStack.back();
    }

    void PushContext(std::string const &ctx) {
        _contextStack.push_back(ctx);
    }

    void PopContext() {
        _contextStack.pop_back();
    }

  private:

    Tf_PyWrapContextManager();

    friend class TfSingleton<This>;

    std::vector<std::string> _contextStack;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_WRAP_CONTEXT_H
