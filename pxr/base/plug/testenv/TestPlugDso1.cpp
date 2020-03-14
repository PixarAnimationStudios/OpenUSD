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

#include "pxr/pxr.h"
#include "pxr/base/plug/testPlugBase.h"

PXR_NAMESPACE_OPEN_SCOPE

class TestPlugDerived1 : public _TestPlugBase1 {
  public:
    typedef TestPlugDerived1 This;
    typedef TfRefPtr<This> RefPtr;
    typedef TfWeakPtr<This> Ptr;

    virtual ~TestPlugDerived1() {}

    virtual std::string GetTypeName() { return "TestPlugDerived1"; }

    static RefPtr New() {
        return TfCreateRefPtr(new This());
    }

  protected:
    TestPlugDerived1() {}
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TestPlugDerived1,
                   TfType::Bases<_TestPlugBase1> >()
        .SetFactory<_TestPlugFactory<TestPlugDerived1> >()
        ;
}

PXR_NAMESPACE_CLOSE_SCOPE
