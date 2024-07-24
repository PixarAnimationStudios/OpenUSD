//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
