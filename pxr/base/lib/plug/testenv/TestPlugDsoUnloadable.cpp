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
#include "pxr/base/plug/testPlugBase.h"
#include "pxr/base/tf/tf.h"

// This plugin depends on an undefined external function
// and so will be unloadable.
extern int TestPlugDoSomethingUndefined();
static int something = TestPlugDoSomethingUndefined();

class TestPlugUnloadable : public _TestPlugBase1 {
  public:
    typedef TestPlugUnloadable This;
    typedef TfRefPtr<This> RefPtr;
    typedef TfWeakPtr<This> Ptr;

    virtual ~TestPlugUnloadable() {}

    virtual std::string GetTypeName() { return "TestPlugUnloadable"; }

    static RefPtr New() {
        return TfCreateRefPtr(new This());
    }

  protected:
    TestPlugUnloadable() {}
};

TF_REGISTRY_FUNCTION(TfType)
{
    // Ensure that the static something gets "used" to avoid compiler
    // warnings.
    TF_UNUSED(something);

    TfType::Define<TestPlugUnloadable,
                   TfType::Bases<_TestPlugBase1> >()
        .SetFactory<_TestPlugFactory<TestPlugUnloadable> >()
        ;
}
