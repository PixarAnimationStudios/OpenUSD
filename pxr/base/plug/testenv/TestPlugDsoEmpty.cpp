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

// This plugin is coded correctly, but will have an empty plugInfo.json
class TestPlugEmpty : public _TestPlugBase3 {
  public:
    typedef TestPlugEmpty This;
    typedef TfRefPtr<This> RefPtr;
    typedef TfWeakPtr<This> Ptr;

    virtual ~TestPlugEmpty() {}

    virtual std::string GetTypeName() { return "TestPlugEmpty"; }

    static RefPtr New() {
        return TfCreateRefPtr(new This());
    }

  protected:
    TestPlugEmpty() {}
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TestPlugEmpty,
                   TfType::Bases<_TestPlugBase3> >()
        .SetFactory<_TestPlugFactory<TestPlugEmpty> >()
        ;
}

PXR_NAMESPACE_CLOSE_SCOPE
