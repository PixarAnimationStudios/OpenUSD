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
#ifndef TF_PYWRAPCONTEXT_H
#define TF_PYWRAPCONTEXT_H

#include "pxr/base/tf/singleton.h"

#include <boost/noncopyable.hpp>

#include <string>
#include <vector>

// This is used internally by the Tf python wrapping infrastructure.

class Tf_PyWrapContextManager : public boost::noncopyable {

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

TF_API_TEMPLATE_CLASS(TfSingleton<Tf_EnumRegistry>);

#endif // TF_PYWRAPCONTEXT_H
