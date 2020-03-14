//
// Copyright 2019 Pixar
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
#include "pxr/base/tf/anyUniquePtr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/regTest.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace
{
    struct TestCounter
    {
        ~TestCounter() {
            ++destructorCount;
        }

        static int destructorCount;
    };

    int TestCounter::destructorCount = 0;
}

static bool
Test_TfAnyUniquePtr()
{
    // Construct using trivial, default-constructed held value
    {
        TfAnyUniquePtr p = TfAnyUniquePtr::New<int>();
        TF_AXIOM(p.Get() != nullptr);
        TF_AXIOM(*static_cast<int const *>(p.Get()) == 0);
    }

    // Construct using trivial, copy-constructed held value
    {
        TfAnyUniquePtr p = TfAnyUniquePtr::New(1);
        TF_AXIOM(p.Get() != nullptr);
        TF_AXIOM(*static_cast<int const *>(p.Get()) == 1);
    }

    // Move construct
    {
        TfAnyUniquePtr p = TfAnyUniquePtr::New<int>(2);
        TfAnyUniquePtr p2 = std::move(p);
        TF_AXIOM(*static_cast<int const *>(p2.Get()) == 2);
    }

    // Move assign
    {
        TfAnyUniquePtr p = TfAnyUniquePtr::New<int>();
        p = TfAnyUniquePtr::New(3);
        TF_AXIOM(*static_cast<int const *>(p.Get()) == 3);
    }

    // Non-trivial, default-constructed held type
    {
        TfAnyUniquePtr p = TfAnyUniquePtr::New<std::string>();
        TF_AXIOM(p.Get() != nullptr);
        TF_AXIOM(*static_cast<std::string const *>(p.Get()) == "");
    }

    // Non-trivial, copy-constructed held type
    {
        std::string s = "Testing";
        TfAnyUniquePtr p = TfAnyUniquePtr::New(s);
        TF_AXIOM(p.Get() != nullptr);
        TF_AXIOM(*static_cast<std::string const *>(p.Get()) == "Testing");
    }

    // Check that the destructor is run as expected
    TF_AXIOM(TestCounter::destructorCount == 0);
    {
        TfAnyUniquePtr p = TfAnyUniquePtr::New<TestCounter>();
    }
    TF_AXIOM(TestCounter::destructorCount == 1);
    {
        TestCounter c;
        TfAnyUniquePtr p = TfAnyUniquePtr::New(c);
    }
    TF_AXIOM(TestCounter::destructorCount == 3);

    return true;
}

TF_ADD_REGTEST(TfAnyUniquePtr);
